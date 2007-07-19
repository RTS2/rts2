#include <limits.h>
#include <iostream>

#include "rts2execcli.h"
#include "../writers/rts2image.h"
#include "../utilsdb/target.h"
#include "../utils/rts2command.h"

Rts2DevClientCameraExec::Rts2DevClientCameraExec (Rts2Conn * in_connection):Rts2DevClientCameraImage
  (in_connection),
Rts2DevScript (in_connection)
{
  queCurrentImage = false;
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
}

int
Rts2DevClientCameraExec::getNextCommand ()
{
  return getScript ()->nextCommand (*this, &nextComd, cmd_device);
}

void
Rts2DevClientCameraExec::clearBlockMove ()
{
  // we will be cleared when exposure ends..
  if (getIsExposing ())
    return;
  Rts2DevScript::clearBlockMove ();
}

void
Rts2DevClientCameraExec::nextCommand ()
{
  int ret;
  ret = haveNextCommand ();
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Rts2DevClientCameraExec::nextComd " << ret <<
    sendLog;
#endif /* DEBUG_EXTRA */
  if (!ret)
    return;

  // send command to other device
  if (strcmp (getName (), cmd_device))
    {
      Rts2Conn *cmdConn = getMaster ()->getConnection (cmd_device);
      if (!cmdConn)
	{
	  logStream (MESSAGE_ERROR) << "Unknow device : " << cmd_device <<
	    sendLog;
	  return;
	}
      // WHILE_EXPOSING
      if (nextComd->getCommandCond () == WHILE_EXPOSING)
	{
	  if (!getIsExposing ())
	    {
	      logStream (MESSAGE_WARNING) <<
		"Tried to execute 'while exposing' command without exposure, executing it now"
		<< sendLog;
	    }
	  else if (isIdle ())
	    {
	      return;
	    }
	}
      logStream (MESSAGE_DEBUG) << "Executing WHILE_EXPOSING command" <<
	sendLog;

      cmdConn->queCommand (nextComd);
      nextComd = NULL;
      return;
    }

  if (nextComd->getCommandCond () == NO_EXPOSURE_NO_MOVE
      || nextComd->getCommandCond () == NO_EXPOSURE_MOVE)
    {
#ifdef DEBUG_EXTRA
      logStream (MESSAGE_DEBUG) <<
	"Rts2DevClientCameraExec::nextComd isExposing: " << getIsExposing ()
	<< " isIdle: " << isIdle () << sendLog;
#endif /* DEBUG_EXTRA */
      if (getIsExposing () || !isIdle ())
	{
	  return;		// after current exposure ends..
	}
      // don't execute command which need stable mount before
      // move was executed
      if (currentTarget && nextComd->getCommandCond () == NO_EXPOSURE_NO_MOVE)
	{
#ifdef DEBUG_EXTRA
	  logStream (MESSAGE_DEBUG) <<
	    "Rts2DevClientCameraExec::nextComd wasMoved: " << currentTarget->
	    wasMoved () << sendLog;
#endif /* DEBUG_EXTRA */
	  if (!currentTarget->wasMoved ())
	    {
	      return;
	    }
	}
      setIsExposing (true);
    }
  // when we would like to execute that while exposing
  if (nextComd->getCommandCond () == WHILE_EXPOSING)
    {
      if (!getIsExposing ())
	{
	  logStream (MESSAGE_WARNING) <<
	    "Tried to execute 'while exposing' command without exposure, executing it now"
	    << sendLog;
	}
      else if (isIdle ())
	{
	  return;
	}
      logStream (MESSAGE_DEBUG) << "Executing WHILE_EXPOSING command" <<
	sendLog;
    }
  queCommand (nextComd);
  nextComd = NULL;		// after command execute, it will be deleted
  blockMove = 1;		// as we run a script..
}

void
Rts2DevClientCameraExec::queImage (Rts2Image * image)
{
  // if unknow type, don't process image..
  if ((image->getShutter () != SHUT_OPENED
       && image->getShutter () != SHUT_SYNCHRO)
      || image->getImageType () == IMGTYPE_FLAT)
    return;

  // find image processor with lowest que number..
  Rts2Conn *minConn = getMaster ()->getMinConn ("que_size");
  if (!minConn)
    return;
  image->saveImage ();
  minConn->queCommand (new Rts2CommandQueImage (getMaster (), image));
}

imageProceRes Rts2DevClientCameraExec::processImage (Rts2Image * image)
{
  int
    ret;
  // try processing in script..
  if (getScript () && !queCurrentImage)
    {
      ret = getScript ()->processImage (image);
      if (ret > 0)
	{
	  return IMAGE_KEEP_COPY;
	}
      else if (ret == 0)
	{
	  return IMAGE_DO_BASIC_PROCESSING;
	}
      // otherwise que image processing
    }
  else
    {
      queCurrentImage = false;
    }
  queImage (image);
  return IMAGE_DO_BASIC_PROCESSING;
}

void
Rts2DevClientCameraExec::exposureStarted ()
{
  // we control observations..
  if (getScript ())
    {
      blockMove = 1;
    }
  Rts2DevClientCameraImage::exposureStarted ();
  if (nextComd && nextComd->getCommandCond () == WHILE_EXPOSING)
    nextCommand ();
}

void
Rts2DevClientCameraExec::exposureEnd ()
{
  blockMove = 0;
  if (!getScript ()
      || (getScript () && !nextComd && getScript ()->isLastCommand ()))
    {
      deleteScript ();
      // EVENT_LAST_READOUT will start new script, when it's possible
      getMaster ()->postEvent (new Rts2Event (EVENT_LAST_READOUT));
      // created image is last in script - will be qued, not processed
      queCurrentImage = true;
    }
  else
    {
      // delete nextComd;
      // nextComd = NULL;
      nextCommand ();
    }
  // send readout after we deal with next command - which can be filter move
  Rts2DevClientCameraImage::exposureEnd ();
}

void
Rts2DevClientCameraExec::exposureFailed (int status)
{
  // in case of an error..
  blockMove = 0;
  queCurrentImage = false;
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
  nextCommand ();
  Rts2DevClientCameraImage::filterFailed (status);
}

void
Rts2DevClientCameraExec::settingsOK ()
{
  nextCommand ();
  Rts2DevClientCameraImage::settingsOK ();
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
      clearWait ();
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
	      fixedOffset.ra = 0;
	      fixedOffset.dec = 0;
	    case OBS_MOVE_FIXED:
	      queCommand (new Rts2CommandScriptEnds (getMaster ()));
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
	  queCommand (cmdChng);
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
    case EVENT_CORRECTING_OK:
    case EVENT_MOVE_FAILED:
      break;
    case EVENT_MOVE_QUESTION:
      if (blockMove)
	{
	  ((Rts2ValueInteger *) event->getArg ())->inc ();
	}
      break;
    case EVENT_ADD_FIXED_OFFSET:
      offset = (ln_equ_posn *) event->getArg ();
      // ra hold offset in HA - that increase on west
      // but we get offset in RA, which increase on east
      fixedOffset.ra += offset->ra;
      fixedOffset.dec += offset->dec;
      break;
    case EVENT_ACQUSITION_END:
      break;
    case EVENT_TEL_START_GUIDING:
      gp = (GuidingParams *) event->getArg ();
      queCommand (new
		  Rts2CommandStartGuide (getMaster (), gp->dir, gp->dist));
      break;
    case EVENT_TEL_STOP_GUIDING:
      queCommand (new Rts2CommandStopGuideAll (getMaster ()));
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
      currentTarget->moveStarted ();
      queCommand (new
		  Rts2CommandMove (getMaster (), this, coord.ra, coord.dec));
      break;
    case OBS_MOVE_UNMODELLED:
      currentTarget->moveStarted ();
      queCommand (new
		  Rts2CommandMoveUnmodelled (getMaster (), this,
					     coord.ra, coord.dec));
      break;
    case OBS_MOVE_FIXED:
      currentTarget->moveStarted ();
      logStream (MESSAGE_DEBUG)
	<< "Rts2DevClientTelescopeExec::syncTarget ha "
	<< coord.ra << " dec " << coord.dec
	<< " oha " << fixedOffset.ra << " odec " << fixedOffset.
	dec << sendLog;
      // we are ofsetting in HA, but offset is in RA - hence -
      queCommand (new
		  Rts2CommandMoveFixed (getMaster (), this,
					coord.ra - fixedOffset.ra,
					coord.dec + fixedOffset.dec));
      break;
    case OBS_ALREADY_STARTED:
      currentTarget->moveStarted ();
      if (fixedOffset.ra != 0 || fixedOffset.dec != 0)
	{
#ifdef DEBUG_EXTRA
	  logStream (MESSAGE_DEBUG)
	    << "Rts2DevClientTelescopeExec::syncTarget resync offsets: ra "
	    << fixedOffset.ra << " dec " << fixedOffset.dec << sendLog;
#endif
	  queCommand (new
		      Rts2CommandChange (this, fixedOffset.ra,
					 fixedOffset.dec));
	  fixedOffset.ra = 0;
	  fixedOffset.dec = 0;
	  break;
	}
      queCommand (new
		  Rts2CommandResyncMove (getMaster (), this,
					 coord.ra, coord.dec));
      break;
    case OBS_DONT_MOVE:
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
  if (currentTarget)
    currentTarget->moveEnded ();
  if (moveWasCorrecting)
    getMaster ()->postEvent (new Rts2Event (EVENT_CORRECTING_OK));
  else
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
  if (currentTarget && currentTarget->moveWasStarted ())
    currentTarget->moveFailed ();
  blockMove = 0;
  Rts2DevClientTelescopeImage::moveFailed (status);
  // move failed, either because of priority change, or because device failure
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

Rts2DevClientMirrorExec::Rts2DevClientMirrorExec (Rts2Conn * in_connection):Rts2DevClientMirror
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
	  queCommand (new Rts2CommandMirror (this, se->getMirrorPos ()));
	  se->takeJob ();
	}
      break;
    }
  Rts2DevClientMirror::postEvent (event);
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
