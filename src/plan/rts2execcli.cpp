#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <limits.h>

#include "rts2execcli.h"
#include "../writers/rts2imagedb.h"
#include "../utilsdb/target.h"
#include "../utils/rts2command.h"

Rts2DevClientCameraExec::Rts2DevClientCameraExec (Rts2Conn * in_connection):Rts2DevClientCameraImage
  (in_connection)
{
  currentTarget = NULL;
  script = NULL;
  blockMove = 0;
  getObserveStart = 0;
  imgCount = 0;
}

Rts2DevClientCameraExec::~Rts2DevClientCameraExec (void)
{
  deleteScript ();
}

void
Rts2DevClientCameraExec::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_KILL_ALL:
      currentTarget = NULL;
      // stop actual observation..
      deleteScript ();
      break;
    case EVENT_SET_TARGET:
      currentTarget = (Target *) event->getArg ();
      getObserveStart = 0;
      break;
    case EVENT_OBSERVE:
      if (script)		// we are still observing..we will be called after last command finished
	{
	  getObserveStart = 1;
	  break;
	}
      startTarget ();
      getObserveStart = 0;
      break;
    case EVENT_MOVE_QUESTION:
      if (blockMove)
	{
	  connection->getMaster ()->
	    postEvent (new Rts2Event (EVENT_DONT_MOVE));
	}
      break;
    }
  Rts2DevClientCameraImage::postEvent (event);
}

void
Rts2DevClientCameraExec::startTarget ()
{
  char scriptBuf[MAX_COMMAND_LENGTH];
  // currentTarget should be nulled when script ends in
  // deleteScript
  if (!currentTarget)
    return;
  currentTarget->getScript (connection->getName (), scriptBuf);
  script = new Rts2Script (scriptBuf, connection->getName ());
  exposureCount = 1;
  connection->getMaster ()->postEvent (new Rts2Event (EVENT_SCRIPT_STARTED));
  if ((connection->
       getState (0) & (CAM_MASK_EXPOSE | CAM_MASK_DATA | CAM_MASK_READING)) ==
      (CAM_NOEXPOSURE & CAM_NODATA & CAM_NOTREADING))
    {
      nextCommand ();
    }
  // otherwise we post command after end of camera readout
}

void
Rts2DevClientCameraExec::nextCommand ()
{
  Rts2Command *nextComd;
  char new_device[DEVICE_NAME_SIZE];
  int ret;
  if (!script)			// waiting for script..
    {
      return;
    }
  ret =
    script->nextCommand (connection->getMaster (), this, &nextComd,
			 new_device);
  if (ret < 0)
    {
      deleteScript ();
      // we don't get new command..delete us and look if there is new
      // target..
      if (!getObserveStart)
	{
	  return;
	}
      getObserveStart = 0;
      startTarget ();
      if (!script)
	{
	  return;
	}
      ret =
	script->nextCommand (connection->getMaster (), this, &nextComd,
			     new_device);
      // we don't have any next command:(
      if (ret < 0)
	{
	  deleteScript ();
	  return;
	}
    }
  blockMove = 1;		// as we run a script..
  if (!strcmp (new_device, connection->getName ()))
    {
      connection->queCommand (nextComd);
    }
  // else change control to other device...somehow (post event)
}

Rts2Image *
Rts2DevClientCameraExec::createImage (const struct timeval *expStart)
{
  imgCount++;
  if (currentTarget)
    return new Rts2ImageDb (currentTarget->getEpoch (),
			    currentTarget->getTargetID (), this,
			    currentTarget->getObsId (), expStart,
			    currentTarget->getNextImgId ());
  syslog (LOG_ERR,
	  "Rts2DevClientCameraExec::createImage creating no-target image");
  Rts2Image *image;
  char *name;
  asprintf (&name, "!/tmp/%s_%i.fits", connection->getName (), imgCount);
  image = new Rts2Image (name, expStart);
  free (name);
  return image;
}

void
Rts2DevClientCameraExec::processImage (Rts2Image * image)
{
  // if unknow type, don't process image..
  if (image->getType () != IMGTYPE_OBJECT)
    return;

  // find image processor with lowest que number..
  int lovestValue = INT_MAX;
  Rts2Conn *minConn = NULL;
  for (int i = 0; i < MAX_CONN; i++)
    {
      Rts2Value *que_size;
      Rts2Conn *conn;
      conn = connection->getMaster ()->connections[i];
      if (conn)
	{
	  que_size = conn->getValue ("que_size");
	  if (que_size)
	    {
	      if (que_size->getValueInteger () >= 0
		  && que_size->getValueInteger () < lovestValue)
		{
		  minConn = conn;
		  lovestValue = que_size->getValueInteger ();
		}
	    }
	}
    }
  if (!minConn)
    return;
  minConn->
    queCommand (new Rts2CommandQueImage (connection->getMaster (), image));
}

void
Rts2DevClientCameraExec::exposureStarted ()
{
  // we control observations..
  if (script)
    {
      blockMove = 1;
    }
  Rts2DevClientCameraImage::exposureStarted ();
}

void
Rts2DevClientCameraExec::exposureEnd ()
{
  Rts2DevClientCameraImage::exposureEnd ();
  if (!script || (script && script->isLastCommand ()))
    {
      blockMove = 0;
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_LAST_READOUT));
      currentTarget = NULL;
    }
}

void
Rts2DevClientCameraExec::readoutEnd ()
{
  nextCommand ();
  // we don't want camera to react to that..
}

void
Rts2DevClientCameraExec::deleteScript ()
{
  blockMove = 0;
  if (script)
    {
      delete script;
      script = NULL;
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_SCRIPT_ENDED));
    }
}

Rts2DevClientTelescopeExec::Rts2DevClientTelescopeExec (Rts2Conn * in_connection):Rts2DevClientTelescopeImage
  (in_connection)
{
  currentTarget = NULL;
  blockMove = 0;
}

void
Rts2DevClientTelescopeExec::postEvent (Rts2Event * event)
{
  struct ln_equ_posn coord;
  switch (event->getType ())
    {
    case EVENT_SET_TARGET:
      currentTarget = (Target *) event->getArg ();
      break;
    case EVENT_SLEW_TO_TARGET:
      if (currentTarget)
	{
	  int ret;
	  currentTarget->beforeMove ();
	  getEqu (&coord);
	  ret = currentTarget->startObservation (&coord);
	  if (ret == OBS_DONT_MOVE)
	    {
	      connection->getMaster ()->
		postEvent (new
			   Rts2Event (EVENT_OBSERVE, (void *) currentTarget));
	    }
	  else
	    {
	      blockMove = 1;
	      connection->
		queCommand (new
			    Rts2CommandMove (connection->getMaster (), this,
					     coord.ra, coord.dec));
	    }
	}
      break;
    case EVENT_MOVE_QUESTION:
      if (blockMove)
	{
	  connection->getMaster ()->
	    postEvent (new Rts2Event (EVENT_DONT_MOVE));
	}
      break;
    }
  Rts2DevClientTelescopeImage::postEvent (event);
}

void
Rts2DevClientTelescopeExec::moveEnd ()
{
  connection->getMaster ()->postEvent (new Rts2Event (EVENT_OBSERVE));
  blockMove = 0;
  Rts2DevClientTelescopeImage::moveEnd ();
}

void
Rts2DevClientTelescopeExec::moveFailed (int status)
{
  Rts2DevClientTelescopeImage::moveFailed (status);
  blockMove = 0;
  // move failed because we get priority..
  connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_MOVE_FAILED, (void *) &status));
}
