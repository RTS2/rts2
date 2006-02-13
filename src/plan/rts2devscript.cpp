#include "rts2devscript.h"
#include "rts2execcli.h"

Rts2DevScript::Rts2DevScript (Rts2Conn * in_script_connection):Rts2Object ()
{
  currentTarget = NULL;
  nextComd = NULL;
  nextScript = NULL;
  nextTarget = NULL;
  script = NULL;
  blockMove = 0;
  getObserveStart = NO_START;
  waitScript = NO_WAIT;
  dont_execute_for = -1;
  script_connection = in_script_connection;
}

Rts2DevScript::~Rts2DevScript (void)
{
  // cannot call deleteScript there, as unsetWait is pure virtual
}

void
Rts2DevScript::startTarget ()
{
  // currentTarget should be nulled when script ends in
  // deleteScript
  if (!currentTarget)
    {
      if (!nextTarget)
	return;
      script = nextScript;
      currentTarget = nextTarget;
      nextScript = NULL;
      nextTarget = NULL;
    }
  else
    {
      script =
	new Rts2Script (script_connection->getMaster (),
			script_connection->getName (), currentTarget);
    }
  clearFailedCount ();
  queCommandFromScript (new
			Rts2CommandScriptEnds (script_connection->
					       getMaster ()));
  script_connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_SCRIPT_STARTED));
}

void
Rts2DevScript::postEvent (Rts2Event * event)
{
  int sig;
  int acqEnd;
  Rts2Script *tmp_script;
  AcquireQuery *ac;
  switch (event->getType ())
    {
    case EVENT_KILL_ALL:
      currentTarget = NULL;
      if (nextScript)
	{
	  tmp_script = nextScript;
	  nextScript = NULL;
	  delete tmp_script;
	}
      nextTarget = NULL;
      // stop actual observation..
      blockMove = 0;
      unsetWait ();
      waitScript = NO_WAIT;
      if (script)
	{
	  tmp_script = script;
	  script = NULL;
	  delete tmp_script;
	}
      delete nextComd;
      nextComd = NULL;
      break;
    case EVENT_SET_TARGET:
      setNextTarget ((Target *) event->getArg ());
      getObserveStart = NO_START;
      break;
    case EVENT_LAST_READOUT:
    case EVENT_SCRIPT_ENDED:
      if (!event->getArg () || (getObserveStart == START_NEXT && script))
	break;
      // we get new target..handle that same way as EVENT_OBSERVE command,
      // telescope will not move
      setNextTarget ((Target *) event->getArg ());
      // currentTarget is defined - tested in if (!event->getArg () || ..
    case EVENT_OBSERVE:
      if (script && getObserveStart != START_CURRENT)	// we are still observing..we will be called after last command finished
	{
	  getObserveStart = START_NEXT;
	  break;
	}
      startTarget ();
      if ((script_connection->
	   getState (0) & (CAM_MASK_EXPOSE | CAM_MASK_DATA |
			   CAM_MASK_READING)) ==
	  (CAM_NOEXPOSURE & CAM_NODATA & CAM_NOTREADING))
	{
	  nextCommand ();
	}
      // otherwise we post command after end of camera readout
      getObserveStart = NO_START;
      break;
    case EVENT_CLEAR_WAIT:
      clearWait ();
      // in case we have some command pending..send it
      if ((script_connection->
	   getState (0) & (CAM_MASK_EXPOSE | CAM_MASK_DATA |
			   CAM_MASK_READING)) ==
	  (CAM_NOEXPOSURE & CAM_NODATA & CAM_NOTREADING))
	{
	  nextCommand ();
	}
      // otherwise, exposureEnd/readoutEnd will query new command
      break;
    case EVENT_MOVE_QUESTION:
      if (blockMove)
	{
	  *(int *) event->getArg () = *(int *) event->getArg () + 1;
	}
      break;
    case EVENT_OK_ASTROMETRY:
    case EVENT_NOT_ASTROMETRY:
    case EVENT_STAR_DATA:
      if (script)
	{
	  script->postEvent (new Rts2Event (event));
	  if (isWaitMove ())
	    // get a change to process updates..
	    nextCommand ();
	}
      break;
    case EVENT_MIRROR_FINISH:
      if (script && waitScript == WAIT_MIRROR)
	{
	  script->postEvent (new Rts2Event (event));
	  waitScript = NO_WAIT;
	  nextCommand ();
	}
      break;
    case EVENT_ACQUSITION_END:
      if (waitScript != WAIT_SLAVE)
	break;
      waitScript = NO_WAIT;
      syslog (LOG_DEBUG, "Rts2DevScript::postEvent EVENT_ACQUSITION_END %s",
	      script_connection->getName ());
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
    case EVENT_ACQUIRE_QUERY:
      ac = (AcquireQuery *) event->getArg ();
      if (currentTarget
	  && ac->tar_id == currentTarget->getObsTargetID ()
	  && currentTarget->isAcquired ())
	{
	  // target that was already acquired will not be acquired again
	  ac->count = 0;
	  break;
	}
    case EVENT_SIGNAL_QUERY:
      if (script)
	script->postEvent (new Rts2Event (event));
      if (nextScript)
	nextScript->postEvent (new Rts2Event (event));
      break;
    case EVENT_SIGNAL:
      if (!script)
	break;
      sig = *(int *) event->getArg ();
      script->postEvent (new Rts2Event (EVENT_SIGNAL, (void *) &sig));
      if (sig == -1)
	{
	  waitScript = NO_WAIT;
	  nextCommand ();
	}
      break;
    case EVENT_TEL_SEARCH_END:
    case EVENT_TEL_SEARCH_STOP:
    case EVENT_TEL_SEARCH_SUCCESS:
      if (waitScript != WAIT_SEARCH)
	break;
      waitScript = NO_WAIT;
      if (script)
	script->postEvent (new Rts2Event (event));
      // give script chance to finish searching (e.g. move filter back
      // to 0, then delete script)
      nextCommand ();
      break;
    }
  Rts2Object::postEvent (event);
}

void
Rts2DevScript::deleteScript ()
{
  Rts2Script *tmp_script;
  clearBlockMove ();
  unsetWait ();
  if (waitScript == WAIT_MASTER)
    {
      int acqRet;
      // should not happen
      acqRet = -5;
      syslog (LOG_DEBUG,
	      "Rts2DevScript::deleteScript sending EVENT_ACQUSITION_END");
      script_connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_ACQUSITION_END, (void *) &acqRet));
    }
  waitScript = NO_WAIT;
  if (script)
    {
      if (currentTarget && script->getExecutedCount () == 0)
	{
	  dont_execute_for = currentTarget->getTargetID ();
	  if (nextTarget && nextTarget->getTargetID () == dont_execute_for)
	    {
	      tmp_script = nextScript;
	      nextScript = NULL;
	      delete tmp_script;
	      nextTarget = NULL;
	    }
	}
      if (getFailedCount () > 0)
	// don't execute us for current target..
	dont_execute_for = currentTarget->getTargetID ();
      tmp_script = script;
      script = NULL;
      delete tmp_script;
      currentTarget = NULL;
      script_connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_SCRIPT_ENDED));
    }
}

void
Rts2DevScript::searchSucess ()
{
  script_connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_TEL_SEARCH_SUCCESS));
}

void
Rts2DevScript::setNextTarget (Target * in_target)
{
  Rts2Script *tmp_script;
  if (nextTarget)
    {
      // we have to free our memory..
      tmp_script = nextScript;
      nextScript = NULL;
      delete tmp_script;
    }
  nextTarget = in_target;
  if (nextTarget->getTargetID () == dont_execute_for)
    {
      nextTarget = NULL;
    }
  else
    {
      nextScript =
	new Rts2Script (script_connection->getMaster (),
			script_connection->getName (), nextTarget);
    }
}

int
Rts2DevScript::nextPreparedCommand ()
{
  int ret;
  if (nextComd)
    return 0;
  ret = getNextCommand ();
  switch (ret)
    {
    case NEXT_COMMAND_WAITING:
      setWaitMove ();
      break;
    case NEXT_COMMAND_CHECK_WAIT:
      unblockWait ();
      setWaitMove ();
      break;
    case NEXT_COMMAND_RESYNC:
      script_connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_TEL_SCRIPT_RESYNC));
      setWaitMove ();
      break;
    case NEXT_COMMAND_PRECISION_OK:
    case NEXT_COMMAND_PRECISION_FAILED:
      clearWait ();		// don't wait for mount move - it will not happen
      waitScript = NO_WAIT;
      syslog (LOG_DEBUG,
	      "Rts2DevScript::nextPreparedCommand sending EVENT_ACQUSITION_END %s %i",
	      script_connection->getName (), ret);
      script_connection->getMaster ()->
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
      // let's wait for that..
      waitScript = WAIT_SLAVE;
      break;
    case NEXT_COMMAND_ACQUSITION_IMAGE:
      currentTarget->acqusitionStart ();
      script_connection->getMaster ()->
	postEvent (new Rts2Event (EVENT_ACQUIRE_START));
      waitScript = WAIT_MASTER;
      break;
    case NEXT_COMMAND_WAIT_SIGNAL:
      waitScript = WAIT_SIGNAL;
      break;
    case NEXT_COMMAND_WAIT_MIRROR:
      waitScript = WAIT_MIRROR;
      break;
    case NEXT_COMMAND_WAIT_SEARCH:
      waitScript = WAIT_SEARCH;
      break;
    }
  return ret;
}

int
Rts2DevScript::haveNextCommand ()
{
  int ret;

  if (!script || waitScript == WAIT_SLAVE)	// waiting for script or acqusition
    {
      return 0;
    }
  ret = nextPreparedCommand ();
  if (ret < 0)
    {
      deleteScript ();
      // we don't get new command..delete us and look if there is new
      // target..
      if (getObserveStart == NO_START)
	{
	  return 0;
	}
      getObserveStart = NO_START;
      startTarget ();
      if (!script)
	{
	  return 0;
	}
      ret = nextPreparedCommand ();
      // we don't have any next command:(
      if (ret < 0)
	{
	  deleteScript ();
	  return 0;
	}
    }
  if (isWaitMove () || waitScript == WAIT_SLAVE || waitScript == WAIT_SIGNAL
      || waitScript == WAIT_MIRROR || waitScript == WAIT_SEARCH
      || nextComd == NULL)
    return 0;
  if (!strcmp (cmd_device, "TX"))	// some telescope command..
    {
      script_connection->getMaster ()->
	postEvent (new
		   Rts2Event (EVENT_TEL_SCRIPT_CHANGE, (void *) nextComd));
      // postEvent have to create copy (in case we will serve more devices) .. so we have to delete command
      delete nextComd;
      nextComd = NULL;
      setWaitMove ();
      blockMove = 1;
      return 0;
    }
  if (strcmp (cmd_device, script_connection->getName ()))
    {
      syslog (LOG_ERR,
	      "Rts2DevScript::haveNextCommand cmd_device %s ret %i conn %s",
	      cmd_device, ret, script_connection->getName ());
      return 0;
    }
  return 1;
}
