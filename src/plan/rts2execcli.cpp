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
  nextComd = NULL;
  script = NULL;
  blockMove = 0;
  getObserveStart = 0;
  imgCount = 0;
  waitAcq = NO_WAIT;
}

Rts2DevClientCameraExec::~Rts2DevClientCameraExec (void)
{
  deleteScript ();
}

void
Rts2DevClientCameraExec::postEvent (Rts2Event * event)
{
  int acqEnd;
  switch (event->getType ())
    {
    case EVENT_KILL_ALL:
      currentTarget = NULL;
      // stop actual observation..
      waiting = NOT_WAITING;
      blockMove = 0;
      unblockWait ();
      waitAcq = NO_WAIT;
      if (script)
	{
	  delete script;
	  script = NULL;
	}
      isExposing = 0;
      break;
    case EVENT_SET_TARGET:
      currentTarget = (Target *) event->getArg ();
      getObserveStart = 0;
      break;
    case EVENT_LAST_READOUT:
    case EVENT_SCRIPT_ENDED:
      if (!event->getArg () || (getObserveStart && script))
	break;
      // we get new target..handle that same way as EVENT_OBSERVE command,
      // telescope will not move
      currentTarget = (Target *) event->getArg ();
    case EVENT_OBSERVE:
      if (script)		// we are still observing..we will be called after last command finished
	{
	  getObserveStart = 1;
	  break;
	}
      startTarget ();
      if ((connection->
	   getState (0) & (CAM_MASK_EXPOSE | CAM_MASK_DATA |
			   CAM_MASK_READING)) ==
	  (CAM_NOEXPOSURE & CAM_NODATA & CAM_NOTREADING))
	{
	  nextCommand ();
	}
      // otherwise we post command after end of camera readout
      getObserveStart = 0;
      break;
    case EVENT_CLEAR_WAIT:
      waiting = NOT_WAITING;
      // in case we have some command pending..send it
      nextCommand ();
      break;
    case EVENT_MOVE_QUESTION:
      if (blockMove)
	{
	  *(int *) event->getArg () = *(int *) event->getArg () + 1;
	}
      break;
    case EVENT_OK_ASTROMETRY:
    case EVENT_NOT_ASTROMETRY:
      if (script)
	{
	  script->postEvent (new Rts2Event (event));
	  if (waiting == WAIT_MOVE)
	    // get a change to process updates..
	    nextCommand ();
	}
      break;
    case EVENT_ACQUSITION_END:
      if (waitAcq != WAIT_SLAVE)
	break;
      waitAcq = NO_WAIT;
      acqEnd = *(int *) event->getArg ();
      switch (acqEnd)
	{
	case NEXT_COMMAND_PRECISION_OK:
	  nextCommand ();
	  break;
	case -5:		// failed with script deletion..
	case NEXT_COMMAND_PRECISION_FAILED:
	  deleteScript ();
	  break;
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
  script = new Rts2Script (scriptBuf, connection);
  exposureCount = 1;
  connection->getMaster ()->postEvent (new Rts2Event (EVENT_SCRIPT_STARTED));
}

int
Rts2DevClientCameraExec::nextPreparedCommand ()
{
  int ret;
  if (nextComd)
    return 0;
  ret = script->nextCommand (this, &nextComd, cmd_device);
  switch (ret)
    {
    case NEXT_COMMAND_WAITING:
      waiting = WAIT_MOVE;
      break;
    case NEXT_COMMAND_CHECK_WAIT:
      unblockWait ();
      if (waiting == NOT_WAITING)
	waiting = WAIT_MOVE;
      break;
    case NEXT_COMMAND_RESYNC:
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_TEL_SCRIPT_RESYNC));
      if (waiting == NOT_WAITING)
	waiting = WAIT_MOVE;
      break;
    case NEXT_COMMAND_PRECISION_OK:
    case NEXT_COMMAND_PRECISION_FAILED:
      waiting = NOT_WAITING;	// don't wait for mount move - it will not happen
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_ACQUSITION_END, (void *) &ret));
      if (ret == NEXT_COMMAND_PRECISION_OK)
	{
	  // there wouldn't be a recursion, as Rts2ScriptElement->nextCommand
	  // should never return NEXT_COMMAND_PRECISION_OK
	  ret = nextPreparedCommand ();
	}
      else
	{
	  ret = NEXT_COMMAND_END_SCRIPT;
	}
      break;
    case NEXT_COMMAND_WAIT_ACQUSITION:
      waitAcq = WAIT_SLAVE;
      break;
    case NEXT_COMMAND_ACQUSITION_IMAGE:
      waitAcq = WAIT_MASTER;
      break;
    }
  return ret;
}

void
Rts2DevClientCameraExec::nextCommand ()
{
  int ret;

  if (!script || waitAcq == WAIT_SLAVE)	// waiting for script or acqusition
    {
      return;
    }
  ret = nextPreparedCommand ();
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
      ret = nextPreparedCommand ();
      // we don't have any next command:(
      if (ret < 0)
	{
	  deleteScript ();
	  return;
	}
    }
  if (waiting == WAIT_MOVE || waitAcq == WAIT_SLAVE)
    return;
  if (!strcmp (cmd_device, connection->getName ()))
    {
      if (nextComd->getCommandCond () == NO_EXPOSURE)
	{
	  if (isExposing || (connection->
			     getState (0) & (CAM_MASK_EXPOSE | CAM_MASK_DATA |
					     CAM_MASK_READING)) !=
	      (CAM_NOEXPOSURE & CAM_NODATA & CAM_NOTREADING))
	    {
	      return;		// after current exposure ends..
	    }
	  isExposing = 1;
	}
      connection->queCommand (nextComd);
      nextComd = NULL;		// after command execute, it will be deleted
    }
  if (!strcmp (cmd_device, "TX"))	// some telescope command..
    {
      connection->getMaster ()->
	postEvent (new
		   Rts2Event (EVENT_TEL_SCRIPT_CHANGE, (void *) nextComd));
      // postEvent have to create copy (in case we will serve more devices) .. so we have to delete command
      delete nextComd;
      nextComd = NULL;
      waiting = WAIT_MOVE;
    }
  blockMove = 1;		// as we run a script..
  if (currentTarget)
    currentTarget->startObservation ();
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
  int ret;
  // try processing in script..
  if (script)
    {
      ret = script->processImage (image);
      if (!ret)
	return;
    }
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
  image->saveImage ();
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
      currentTarget = NULL;
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_LAST_READOUT));
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
Rts2DevClientCameraExec::readoutEnd ()
{
  nextCommand ();
  // we don't want camera to react to that..
}

void
Rts2DevClientCameraExec::deleteScript ()
{
  blockMove = 0;
  unblockWait ();
  if (waitAcq == WAIT_MASTER)
    {
      int acqRet;
      // should not happen
      acqRet = -5;
      connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_ACQUSITION_END, (void *) &acqRet));
    }
  waitAcq = NO_WAIT;
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
  cmdChng = NULL;
  blockMove = 0;
}

void
Rts2DevClientTelescopeExec::postEvent (Rts2Event * event)
{
  int ret;
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
	      connection->getMaster ()->
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
    case OBS_ALREADY_STARTED:
      connection->
	queCommand (new
		    Rts2CommandMove (connection->getMaster (), this,
				     coord.ra, coord.dec));
      break;
    }
  return ret;
}

void
Rts2DevClientTelescopeExec::checkInterChange ()
{
  int waitNum = 0;
  connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_QUERY_WAIT, (void *) &waitNum));
  if (waitNum == 0)
    connection->getMaster ()->postEvent (new Rts2Event (EVENT_ENTER_WAIT));
}

void
Rts2DevClientTelescopeExec::moveEnd ()
{
  connection->getMaster ()->postEvent (new Rts2Event (EVENT_MOVE_OK));
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
  connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_MOVE_FAILED, (void *) &status));
}
