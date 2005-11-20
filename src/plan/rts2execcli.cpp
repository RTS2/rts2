#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <limits.h>

#include "rts2execcli.h"
#include "../writers/rts2imagedb.h"
#include "../utilsdb/target.h"
#include "../utils/rts2command.h"

Rts2DevClientCameraExec::Rts2DevClientCameraExec (Rts2Conn * in_connection):Rts2DevClientCameraImage
  (in_connection),
Rts2DevScript (in_connection)
{
  imgCount = 0;
}

Rts2DevClientCameraExec::~Rts2DevClientCameraExec ()
{
  deleteScript ();
}

void
Rts2DevClientCameraExec::postEvent (Rts2Event * event)
{
  Rts2Image *image;
  switch (event->getType ())
    {
    case EVENT_QUE_IMAGE:
      image = (Rts2Image *) event->getArg ();
      if (!strcmp (image->getCameraName (), getName ()))
	queImage (image);
      break;
    }
  Rts2DevScript::postEvent (new Rts2Event (event));
  Rts2DevClientCameraImage::postEvent (event);
}

void
Rts2DevClientCameraExec::startTarget ()
{
  Rts2DevScript::startTarget ();
  // should be deleted..
  exposureCount = 1;
}

int
Rts2DevClientCameraExec::getNextCommand ()
{
  return script->nextCommand (*this, &nextComd, cmd_device);
}

void
Rts2DevClientCameraExec::nextCommand ()
{
  int ret;
  ret = haveNextCommand ();
  if (!ret)
    return;

  if (nextComd->getCommandCond () == NO_EXPOSURE)
    {
      if (isExposing || !isIdle ())
	{
	  return;		// after current exposure ends..
	}
      isExposing = 1;
    }
  connection->queCommand (nextComd);
  nextComd = NULL;		// after command execute, it will be deleted
  blockMove = 1;		// as we run a script..
  if (currentTarget)
    currentTarget->startObservation ();
}

Rts2Image *
Rts2DevClientCameraExec::createImage (const struct timeval *expStart)
{
  imgCount++;
  if (currentTarget)
    return new Rts2ImageDb (currentTarget, this, expStart);
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
Rts2DevClientCameraExec::queImage (Rts2Image * image)
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
      conn = getMaster ()->connections[i];
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
  image->saveImage ();
  minConn->queCommand (new Rts2CommandQueImage (getMaster (), image));
}

void
Rts2DevClientCameraExec::processImage (Rts2Image * image)
{
  int ret;
  // try processing in script..
  if (script)
    {
      ret = script->processImage (image);
      if (!ret)
	{
	  return;
	}
    }
  queImage (image);
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
      getMaster ()->postEvent (new Rts2Event (EVENT_LAST_READOUT));
    }
  else
    {
      nextComd = NULL;
      nextCommand ();
    }
}

void
Rts2DevClientCameraExec::exposureFailed (int status)
{
  // in case of an error..
  blockMove = 0;
  Rts2DevClientCameraImage::exposureFailed (status);
}

void
Rts2DevClientCameraExec::filterOK ()
{
  nextCommand ();
}

void
Rts2DevClientCameraExec::filterFailed (int status)
{
  deleteScript ();
  Rts2DevClientCameraImage::filterFailed ();
}

void
Rts2DevClientCameraExec::settingsOK ()
{
  nextCommand ();
}

void
Rts2DevClientCameraExec::settingsFailed (int status)
{
  deleteScript ();
  Rts2DevClientCameraImage::settingsFailed (status);
}


void
Rts2DevClientCameraExec::readoutEnd ()
{
  nextCommand ();
  // we don't want camera to react to that..
}

Rts2DevClientTelescopeExec::Rts2DevClientTelescopeExec (Rts2Conn * in_connection):Rts2DevClientTelescopeImage
  (in_connection)
{
  currentTarget = NULL;
  cmdChng = NULL;
  blockMove = 0;
  fixedOffset.ra = 0;
  fixedOffset.dec = 0;
}

void
Rts2DevClientTelescopeExec::postEvent (Rts2Event * event)
{
  int ret;
  struct ln_equ_posn *offset;
  GuidingParams *gp;
  Rts2ScriptElementSearch *searchEl;
  switch (event->getType ())
    {
    case EVENT_KILL_ALL:
      waiting = NOT_WAITING;
      break;
    case EVENT_SET_TARGET:
      currentTarget = (Target *) event->getArg ();
      break;
    case EVENT_SLEW_TO_TARGET:
      if (currentTarget)
	{
	  currentTarget->beforeMove ();
	  ret = syncTarget ();
	  switch (ret)
	    {
	    case OBS_DONT_MOVE:
	      getMaster ()->
		postEvent (new
			   Rts2Event (EVENT_OBSERVE, (void *) currentTarget));
	      break;
	    case OBS_MOVE:
	    case OBS_ALREADY_STARTED:
	      blockMove = 1;
	      break;
	    }
	}
      break;
    case EVENT_TEL_SCRIPT_RESYNC:
      cmdChng = NULL;
      checkInterChange ();
      break;
    case EVENT_TEL_SCRIPT_CHANGE:
      cmdChng =
	new Rts2CommandChange ((Rts2CommandChange *) event->getArg (), this);
      checkInterChange ();
      break;
    case EVENT_TEL_SEARCH_START:
      searchEl = (Rts2ScriptElementSearch *) event->getArg ();
      queCommand (new
		  Rts2CommandSearch (this, searchEl->getSearchRadius (),
				     searchEl->getSearchSpeed ()));
      searchEl->getJob ();
      break;
    case EVENT_TEL_SEARCH_SUCCESS:
      queCommand (new Rts2CommandSearchStop (this));
      break;
    case EVENT_ENTER_WAIT:
      if (cmdChng)
	{
	  connection->queCommand (cmdChng);
	  cmdChng = NULL;
	}
      else
	{
	  ret = syncTarget ();
	  if (ret == OBS_DONT_MOVE)
	    {
	      postEvent (new Rts2Event (EVENT_MOVE_OK));
	    }
	}
      break;
    case EVENT_MOVE_OK:
    case EVENT_MOVE_FAILED:
      break;
    case EVENT_MOVE_QUESTION:
      if (blockMove)
	{
	  *(int *) event->getArg () = *(int *) event->getArg () + 1;
	}
      break;
    case EVENT_ADD_FIXED_OFFSET:
      offset = (ln_equ_posn *) event->getArg ();
      fixedOffset.ra += offset->ra;
      fixedOffset.dec += offset->dec;
      break;
    case EVENT_ACQUSITION_END:
      fixedOffset.ra = 0;
      fixedOffset.dec = 0;
      break;
    case EVENT_TEL_START_GUIDING:
      gp = (GuidingParams *) event->getArg ();
      connection->
	queCommand (new
		    Rts2CommandStartGuide (getMaster (), gp->dir, gp->dist));
      break;
    case EVENT_TEL_STOP_GUIDING:
      connection->queCommand (new Rts2CommandStopGuideAll (getMaster ()));
      break;
    }
  Rts2DevClientTelescopeImage::postEvent (event);
}

int
Rts2DevClientTelescopeExec::syncTarget ()
{
  struct ln_equ_posn coord;
  int ret;
  if (!currentTarget)
    return -1;
  getEqu (&coord);
  ret = currentTarget->startSlew (&coord);
  switch (ret)
    {
    case OBS_MOVE:
      connection->
	queCommand (new
		    Rts2CommandMove (getMaster (), this,
				     coord.ra, coord.dec));
      break;
    case OBS_MOVE_FIXED:
      connection->
	queCommand (new
		    Rts2CommandMoveFixed (getMaster (), this,
					  coord.ra + fixedOffset.ra,
					  coord.dec + fixedOffset.dec));
      break;
    case OBS_ALREADY_STARTED:
      syslog (LOG_DEBUG,
	      "Rts2DevClientTelescopeExec::syncTarget resync offsets: ra %lf dec %lf",
	      fixedOffset.ra, fixedOffset.dec);
      connection->
	queCommand (new
		    Rts2CommandResyncMove (getMaster (), this,
					   coord.ra + fixedOffset.ra,
					   coord.dec + fixedOffset.dec));
      break;
    }
  return ret;
}

void
Rts2DevClientTelescopeExec::checkInterChange ()
{
  int waitNum = 0;
  getMaster ()->
    postEvent (new Rts2Event (EVENT_QUERY_WAIT, (void *) &waitNum));
  if (waitNum == 0)
    getMaster ()->postEvent (new Rts2Event (EVENT_ENTER_WAIT));
}

void
Rts2DevClientTelescopeExec::moveEnd ()
{
  getMaster ()->postEvent (new Rts2Event (EVENT_MOVE_OK));
  blockMove = 0;
  Rts2DevClientTelescopeImage::moveEnd ();
}

void
Rts2DevClientTelescopeExec::moveFailed (int status)
{
  if (status == DEVDEM_E_IGNORE)
    {
      moveEnd ();
      return;
    }
  blockMove = 0;
  Rts2DevClientTelescopeImage::moveFailed (status);
  // move failed because we get priority..
  getMaster ()->
    postEvent (new Rts2Event (EVENT_MOVE_FAILED, (void *) &status));
}

void
Rts2DevClientTelescopeExec::searchEnd ()
{
  Rts2DevClientTelescopeImage::searchEnd ();
  getMaster ()->postEvent (new Rts2Event (EVENT_TEL_SEARCH_END));
}

void
Rts2DevClientTelescopeExec::searchFailed (int status)
{
  Rts2DevClientTelescopeImage::searchFailed (status);
  getMaster ()->postEvent (new Rts2Event (EVENT_TEL_SEARCH_END));
}

Rts2DevClientMirrorExec::Rts2DevClientMirrorExec (Rts2Conn * in_connection):Rts2DevClientMirrorImage
  (in_connection)
{
}

void
Rts2DevClientMirrorExec::postEvent (Rts2Event * event)
{
  Rts2ScriptElementMirror *se;
  switch (event->getType ())
    {
    case EVENT_MIRROR_SET:
      se = (Rts2ScriptElementMirror *) event->getArg ();
      if (se->isMirrorName (connection->getName ()))
	{
	  connection->
	    queCommand (new Rts2CommandMirror (this, se->getMirrorPos ()));
	  se->takeJob ();
	}
      break;
    }
  Rts2DevClientMirrorImage::postEvent (event);
}

void
Rts2DevClientMirrorExec::mirrorA ()
{
  getMaster ()->postEvent (new Rts2Event (EVENT_MIRROR_FINISH));
}

void
Rts2DevClientMirrorExec::mirrorB ()
{
  getMaster ()->postEvent (new Rts2Event (EVENT_MIRROR_FINISH));
}

void
Rts2DevClientMirrorExec::moveFailed (int status)
{
  getMaster ()->postEvent (new Rts2Event (EVENT_MIRROR_FINISH));
}
