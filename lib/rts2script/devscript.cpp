/*
 * Client for script devices.
 * Copyright (C) 2004-2007 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "rts2script/devscript.h"
#include "rts2script/execcli.h"

using namespace rts2script;

DevScript::DevScript (rts2core::Connection * in_script_connection) : script ()
{
	scriptKillCommand = NULL;
	scriptKillcallScriptEnds = false;

	currentTarget = NULL;
	nextComd = NULL;
	cmd_device[0] = '\0';
	nextTarget = NULL;
	killTarget = NULL;
	waitScript = NO_WAIT;
	dont_execute_for = -1;
	dont_execute_for_obsid = -1;
	scriptLoopCount = 0;
	scriptCount = 0;
	lastTargetObsID = -1;
	script_connection = in_script_connection;
}

DevScript::~DevScript (void)
{
	// cannot call deleteScript there, as unsetWait is pure virtual
}

void DevScript::startTarget (bool callScriptEnds)
{
	// currentTarget should be nulled when script ends in
	// deleteScript
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "DevScript::startTarget this " << this
		<< " currentTarget " << (currentTarget ? currentTarget->getTargetID () : 0)
		<< " nextTarget " << (nextTarget ? nextTarget->getTargetID () : 0)
		<< sendLog;
	#endif						 /* DEBUG_EXTRA */
	if (currentTarget == NULL)
	{
		if (nextTarget == NULL)
			return;
		currentTarget = nextTarget;
		nextTarget = NULL;
	}

	if (lastTargetObsID == currentTarget->getObsTargetID ())
		scriptLoopCount++;
	else
		scriptLoopCount = 0;

	scriptCount++;

	counted_ptr <Script> sc (new Script (scriptLoopCount, script_connection->getMaster ()));
	sc->setTarget (script_connection->getName (), currentTarget);
	setScript (sc);

	clearFailedCount ();

	if (callScriptEnds)
		queCommandFromScript (new rts2core::CommandScriptEnds (script_connection->getMaster ()));

	scriptBegin ();

	delete nextComd;
	nextComd = NULL;

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "DevScript::startTarget currentTarget " <<
		currentTarget->getTargetID () << " OBS ID " << currentTarget->
		getObsTargetID () << sendLog;
	#endif

	script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_SCRIPT_STARTED));
}

void DevScript::postEvent (rts2core::Event * event)
{
	int sig;
	int acqEnd;
	AcquireQuery *ac;
	switch (event->getType ())
	{
		case EVENT_STOP_TARGET:
			if (currentTarget)
			{
				dont_execute_for = currentTarget->getTargetID ();
				dont_execute_for_obsid = currentTarget->getObsId ();
			}
		case EVENT_KILL_ALL:
		case EVENT_SET_TARGET_KILL:
		case EVENT_SET_TARGET_KILL_NOT_CLEAR:
			currentTarget = NULL;
			nextTarget = NULL;
		#ifdef DEBUG_EXTRA
			logStream(MESSAGE_DEBUG) << script_connection->getName () << " EVENT_KILL_ALL" << sendLog;
		#endif					 /* DEBUG_EXTRA */
			// stop actual observation..
			unsetWait ();
			waitScript = NO_WAIT;
			if (script.get ())
				script.get ()->notActive ();
			script.null ();
			delete nextComd;
			nextComd = NULL;
			if (event->getType () != EVENT_STOP_TARGET)
			{
				// null dont_execute_for
				dont_execute_for = -1;
				dont_execute_for_obsid = -1;
			}
			switch (event->getType ())
			{
				case EVENT_STOP_TARGET:
				case EVENT_SET_TARGET_KILL:
					scriptKillCommand = new rts2core::CommandKillAll (script_connection->getMaster ());
					scriptKillCommand->setOriginator (script_connection);
					scriptKillcallScriptEnds = true;
					killTarget = (Rts2Target *) event->getArg ();
					queCommandFromScript (scriptKillCommand);
					break;
				case EVENT_SET_TARGET_KILL_NOT_CLEAR:
					scriptKillCommand = new rts2core::CommandKillAllWithoutScriptEnds (script_connection->getMaster ());
					scriptKillCommand->setOriginator (script_connection);
					scriptKillcallScriptEnds = false;
					killTarget = (Rts2Target *) event->getArg ();
					queCommandFromScript (scriptKillCommand);
					break;
			}
			break;
		case EVENT_STOP_OBSERVATION:
			deleteScript ();
			break;
		case EVENT_SET_TARGET:
		case EVENT_SET_TARGET_NOT_CLEAR:
			setNextTarget ((Rts2Target *) event->getArg ());
			if (currentTarget)
				break;
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "EVENT_SET_TARGET " << this << " "
				<< (nextTarget ? nextTarget->getTargetID () : 0)
				<< sendLog;
		#endif					 /* DEBUG_EXTRA */
			// we don't have target..so let's observe us
			startTarget (event->getType () != EVENT_SET_TARGET_NOT_CLEAR);
			nextCommand ();
			break;
		case EVENT_MOVE_OK:
		case EVENT_CORRECTING_OK:
			break;
		case EVENT_SCRIPT_ENDED:
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "EVENT_SCRIPT_ENDED DevScript currentTarget " << currentTarget << " nextTarget " << nextTarget << sendLog;
		#endif					 /* DEBUG_EXTRA */
			if (event->getArg ())
				// we get new target..handle that same way as EVENT_OBSERVE command,
				// telescope will not move
				setNextTarget ((Rts2Target *) event->getArg ());
			// we still have something to do
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "EVENT_SCRIPT_ENDED " << currentTarget << " next " << nextTarget << " arg " << event->getArg () << sendLog;
		#endif					 /* DEBUG_EXTRA */
			if (currentTarget)
				break;
			// currentTarget is defined - tested in if (event->getArg ())
		case EVENT_OBSERVE:
			// we can start script before we get EVENT_OBSERVE - don't start us in such case
			if (currentTarget)
			{
				// only check if we have something to do
				nextCommand ();
				break;
			}
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "EVENT_OBSERVE " << this << " current " << currentTarget << " next " << nextTarget << sendLog;
		#endif					 /* DEBUG_EXTRA */
			startTarget ();
			nextCommand ();
			break;
		case EVENT_CLEAR_WAIT:
			clearWait ();
			nextCommand ();
			// if we are still exposing, exposureEnd/readoutEnd will query new command
			break;
		case EVENT_SCRIPT_RUNNING_QUESTION:
			if (script.get ())
				(*((int *) event->getArg ()))++;
			break;
		case EVENT_OK_ASTROMETRY:
		case EVENT_NOT_ASTROMETRY:
		case EVENT_STAR_DATA:
			if (script.get ())
			{
				script->postEvent (new rts2core::Event (event));
				if (isWaitMove ())
					// get a chance to process updates..
					nextCommand ();
			}
			break;
		case EVENT_MIRROR_FINISH:
			if (script.get () && waitScript == WAIT_MIRROR)
			{
				script->postEvent (new rts2core::Event (event));
				waitScript = NO_WAIT;
				nextCommand ();
			}
			break;
		case EVENT_ACQUSITION_END:
			if (waitScript != WAIT_SLAVE)
				break;
			waitScript = NO_WAIT;
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) <<
				"DevScript::postEvent EVENT_ACQUSITION_END " <<
				script_connection->getName () << sendLog;
		#endif
			acqEnd = *(int *) event->getArg ();
			switch (acqEnd)
			{
				case NEXT_COMMAND_PRECISION_OK:
					nextCommand ();
					break;
				case -5:		 // failed with script deletion..
					logStream (MESSAGE_DEBUG)
						<< "DevScript::postEvent EVENT_ACQUSITION_END -5 "
						<< script_connection->getName () << sendLog;
					break;
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
			// we have to wait for decesion from next target
			if (nextTarget && ac->tar_id == nextTarget->getObsTargetID ())
			{
				if (nextTarget->isAcquired ())
					ac->count = 0;
				else
					ac->count++;
				break;
			}
			// that's intentional, as acqueryQuery should be send to all
			// scripts for processing
		case EVENT_SIGNAL_QUERY:
			if (script.get ())
				script->postEvent (new rts2core::Event (event));
			break;
		case EVENT_SIGNAL:
			if (script.get () == 0)
				break;
			sig = *(int *) event->getArg ();
			script->postEvent (new rts2core::Event (EVENT_SIGNAL, (void *) &sig));
			if (sig == -1)
			{
				waitScript = NO_WAIT;
				nextCommand ();
			}
			break;
	}
}

void DevScript::stateChanged (rts2core::ServerState *state)
{
	if ((state->getValue () & DEVICE_ERROR_MASK) && state->maskValueChanged (DEVICE_ERROR_MASK) && script.get ())
		script->errorReported (state->getValue (), state->getOldValue ());
}

void DevScript::scriptBegin ()
{
	rts2core::DevClient *cli = script_connection->getOtherDevClient ();
	if (cli == NULL)
	{
		return;
	}
	if (script.get () == NULL)
	{
		queCommandFromScript (new rts2core::CommandChangeValue (cli, std::string ("SCRIPREP"), '=', 0));
		queCommandFromScript (new rts2core::CommandChangeValue (cli, std::string ("SCRIPT"), '=', std::string ("")));
	}
	else
	{
		queCommandFromScript (new rts2core::CommandChangeValue (cli, std::string ("SCRIPREP"), '=', scriptLoopCount));
		queCommandFromScript (new rts2core::CommandChangeValue (cli, std::string ("SCRIPT"), '=', script->getWholeScript ()));
	}
}

void DevScript::idle ()
{
	if (script.get ())
	{
		int ret = script->idle ();
		if (ret == NEXT_COMMAND_NEXT)
		{
			nextCommand ();
		}
	}
}

void DevScript::deleteScript ()
{
	unsetWait ();
	if (waitScript == WAIT_MASTER)
	{
		int acqRet;
		// should not happen
		acqRet = -5;
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG)
			<< "DevScript::deleteScript sending EVENT_ACQUSITION_END" <<
			sendLog;
		#endif
		script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_ACQUSITION_END, (void *) &acqRet));
	}
	// make sure we don't left any garbage for start of observation
	waitScript = NO_WAIT;
	delete nextComd;
	nextComd = NULL;
	if (script.get ())
	{
		if (currentTarget)
		{
			lastTargetObsID = currentTarget->getObsTargetID ();
			if (script->getExecutedCount () == 0)
			{
				if (currentTarget->getTargetID () >= 0)
				{
					logStream (MESSAGE_WARNING) << "disabling target " << currentTarget->getTargetID () << " on device " << script_connection->getName () << ", because its execution failed. This probably signal wrong script assigned to target - please check the target script" << sendLog;
					dont_execute_for = currentTarget->getTargetID ();
					dont_execute_for_obsid = currentTarget->getObsId ();
					if (nextTarget && nextTarget->getTargetID () == dont_execute_for)
					{
						nextTarget = NULL;
					}
				}
				else
				{
					logStream (MESSAGE_ERROR) << "execution of script for virtual target failed" << sendLog;
				}
			}
		}
		if (getFailedCount () > 0)
		{
			// don't execute us for current target..
			logStream (MESSAGE_WARNING) << "disabling target " << currentTarget->getTargetID ()
				<< " on device " << script_connection->getName () << ", because previous execution was with error" << sendLog;
			dont_execute_for = currentTarget->getTargetID ();
			dont_execute_for_obsid = currentTarget->getObsId ();
		}
		if (script.get ())
			script.get ()->notActive ();
		script.null ();
		currentTarget = NULL;
		// that can result in call to startTarget and
		// therefore nextCommand, which will set nextComd - so we
		// don't want to touch it after that
		script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_SCRIPT_ENDED));
	}
}

void DevScript::setNextTarget (Rts2Target * in_target)
{
	if (in_target->getTargetID () == dont_execute_for && in_target->getObsId () == dont_execute_for_obsid)
	{
	  	logStream (MESSAGE_WARNING) << "device " << script_connection->getName () << " ignores next target (" << in_target->getTargetID () << "," << dont_execute_for 
			<< "), as it should not be execute" << sendLog;
		nextTarget = NULL;
	}
	else
	{
	  	// assume the script will be fixed..
		nextTarget = in_target;
		dont_execute_for = -1;
		dont_execute_for_obsid = -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "setNextTarget " << nextTarget << sendLog;
	#endif						 /* DEBUG_EXTRA */
}

int DevScript::nextPreparedCommand ()
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
			script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_TEL_SCRIPT_RESYNC));
			setWaitMove ();
			break;
		case NEXT_COMMAND_PRECISION_OK:
		case NEXT_COMMAND_PRECISION_FAILED:
			clearWait ();		 // don't wait for mount move - it will not happen
			waitScript = NO_WAIT;
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG)
				<< "DevScript::nextPreparedCommand sending EVENT_ACQUSITION_END "
				<< script_connection->getName () << " " << ret << sendLog;
		#endif
			script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_ACQUSITION_END, (void *) &ret));
			if (ret == NEXT_COMMAND_PRECISION_OK)
			{
				// there wouldn't be a recursion, as Element->nextCommand
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
			script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_ACQUIRE_START));
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

int DevScript::haveNextCommand (rts2core::DevClient *devClient)
{
	int ret;
	// waiting for script or acqusition
	if (script.get () == 0 || waitScript == WAIT_SLAVE)
	{
		return 0;
	}
	ret = nextPreparedCommand ();
	// only end when we do not have any commands in que
	if (ret < 0 && script_connection->queEmptyForOriginator (devClient) && canEndScript ())
	{
	  	logStream (MESSAGE_DEBUG) << "ending script on connection " << script_connection->getName () << sendLog;
		deleteScript ();
		startTarget ();
		if (script.get () == 0)
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
								 // some telescope command..
	if (!strcmp (cmd_device, "TX"))
	{
		script_connection->getMaster ()->postEvent (new rts2core::Event (EVENT_TEL_SCRIPT_CHANGE, (void *) nextComd));
		// postEvent have to create copy (in case we will serve more devices) .. so we have to delete command
		delete nextComd;
		nextComd = NULL;
		setWaitMove ();
		return 0;
	}
	return 1;
}
