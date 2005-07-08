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
}

Rts2DevClientCameraExec::~Rts2DevClientCameraExec (void)
{
  if (script)
    delete script;
}

void
Rts2DevClientCameraExec::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SET_TARGET:
      currentTarget = (Target *) event->getArg ();
      break;
    case EVENT_OBSERVE:
      char scriptBuf[MAX_COMMAND_LENGTH];
      if (!currentTarget)
	break;
      if (script)
	delete script;
      currentTarget->getScript (connection->getName (), scriptBuf);
      script = new Rts2Script (scriptBuf, connection->getName ());
      exposureEnabled = 1;
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_SCRIPT_STARTED));
      nextCommand ();
      break;
    }
  Rts2DevClientCameraImage::postEvent (event);
}

void
Rts2DevClientCameraExec::nextCommand ()
{
  Rts2Command *nextComd;
  char new_device[DEVICE_NAME_SIZE];
  int ret;
  if (!script)			// waiting for script..
    return;
  ret = script->nextCommand (connection->getMaster (), &nextComd, new_device);
  if (ret < 0)
    {
      delete script;
      script = NULL;
      if (exposureEnabled)
	{
	  connection->getMaster ()->
	    postEvent (new Rts2Event (EVENT_LAST_READOUT));
	  exposureEnabled = 0;
	}
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_SCRIPT_ENDED));
      return;
    }
  if (!strcmp (new_device, connection->getName ()))
    {
      connection->queCommand (nextComd);
    }
  // else change control to other device...somehow
}

Rts2Image *
Rts2DevClientCameraExec::createImage (const struct timeval *expStart)
{
  if (currentTarget)
    return new Rts2ImageDb (1, currentTarget->getTargetID (), this,
			    currentTarget->getObsId (), expStart,
			    currentTarget->getNextImgId ());
  return new Rts2Image ("img.fits", expStart);
}

void
Rts2DevClientCameraExec::processImage (Rts2Image * image)
{
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
Rts2DevClientCameraExec::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("img_chip"))
    {
      if ((state->value & (CAM_MASK_EXPOSE | CAM_MASK_DATA)) ==
	  (CAM_NOEXPOSURE | CAM_DATA))
	{
	  if (script && script->isLastCommand ())
	    {
	      connection->getMaster ()->
		postEvent (new Rts2Event (EVENT_LAST_READOUT));
	      exposureEnabled = 0;
	    }
	}
      if ((state->value & (CAM_MASK_EXPOSE | CAM_MASK_READING)) ==
	  (CAM_NOEXPOSURE | CAM_NOTREADING))
	{
	  nextCommand ();
	}
    }
  Rts2DevClientCameraImage::stateChanged (state);
}

Rts2DevClientTelescopeExec::Rts2DevClientTelescopeExec (Rts2Conn * in_connection):Rts2DevClientTelescopeImage
  (in_connection)
{
  currentTarget = NULL;
}

void
Rts2DevClientTelescopeExec::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SET_TARGET:
      struct ln_equ_posn coord;
      currentTarget = (Target *) event->getArg ();
      if (currentTarget)
	{
	  int ret;
	  currentTarget->beforeMove ();
	  getEqu (&coord);
	  ret = currentTarget->startObservation (&coord);
	  if (ret == OBS_DONT_MOVE)
	    {
	      connection->getMaster ()->
		postEvent (new Rts2Event (EVENT_OBSERVE));
	    }
	  else
	    {
	      connection->
		queCommand (new
			    Rts2CommandMove (connection->getMaster (), this,
					     coord.ra, coord.dec));
	    }
	}
      break;
    }
  Rts2DevClientTelescopeImage::postEvent (event);
}

void
Rts2DevClientTelescopeExec::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("telescope") && currentTarget)
    {
      if ((state->value & TEL_MASK_MOVING) == TEL_OBSERVING
	  || (state->value & TEL_MASK_MOVING) == TEL_PARKING)
	{
	  connection->getMaster ()->postEvent (new Rts2Event (EVENT_OBSERVE));
	}
    }
  Rts2DevClientTelescopeImage::stateChanged (state);
}
