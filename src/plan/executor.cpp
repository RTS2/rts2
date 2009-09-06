/*
 * Executor body.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/rts2valuearray.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"
#include "rts2execcli.h"
#include "rts2execclidb.h"
#include "rts2devcliphot.h"

#define OPT_IGNORE_DAY    OPT_LOCAL + 100
#define OPT_DONT_DARK     OPT_LOCAL + 101

namespace rts2plan
{

class Executor:public Rts2DeviceDb
{
	private:
		Target * currentTarget;
		std::list <Target *> nextTargets;

		void clearNextTargets ();

		void queDarks ();
		void queFlats ();
		void beforeChange ();
		void doSwitch ();
		void switchTarget ();

		int setNext (int nextId);
		int queueTarget (int nextId);
		int setNow (int nextId);
		int setGrb (int grbId);
		int setShower ();

								 // -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
		Rts2ValueInteger *scriptCount;
		int waitState;
		std::vector < Target * >targetsQue;
		struct ln_lnlat_posn *observer;

		double grb_sep_limit;
		double grb_min_sep;

		Rts2ValueInteger *acqusitionOk;
		Rts2ValueInteger *acqusitionFailed;

		int setNow (Target * newTarget);

		void processTarget (Target * in_target);
		void updateScriptCount ();

		Rts2ValueInteger *current_id;
		Rts2ValueInteger *current_id_sel;
		Rts2ValueString *current_name;
		Rts2ValueString *current_type;
		Rts2ValueInteger *current_obsid;

		Rts2ValueBool *autoLoop;

		Rts2ValueInteger *next_id;
		Rts2ValueString *next_name;

		Rts2ValueBool *doDarks;
		Rts2ValueBool *ignoreDay;

		rts2core::IntegerArray *next_ids;
		rts2core::StringArray *next_names;

		Rts2ValueInteger *img_id;

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

		virtual int setValue (Rts2Value *oldValue, Rts2Value *newValue);
	public:
		Executor (int argc, char **argv);
		virtual ~ Executor (void);
		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);

		virtual void postEvent (Rts2Event * event);

		virtual void deviceReady (Rts2Conn * conn);

		virtual int info ();

		virtual int changeMasterState (int new_state);

		int end ()
		{
			maskState (EXEC_MASK_END, EXEC_END);
			return 0;
		}

		int stop ();

		virtual int commandAuthorized (Rts2Conn * conn);
};

};

using namespace rts2plan;


Executor::Executor (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_EXECUTOR, "EXEC")
{
	currentTarget = NULL;
	createValue (scriptCount, "script_count", "number of running scripts",
		false);
	scriptCount->setValueInteger (-1);

	grb_sep_limit = -1;
	grb_min_sep = 0;

	waitState = 0;

	createValue (acqusitionOk, "acqusition_ok",
		"number of acqusitions completed sucesfully", false);
	acqusitionOk->setValueInteger (0);

	createValue (acqusitionFailed, "acqusition_failed",
		"number of acqusitions which failed", false);
	acqusitionFailed->setValueInteger (0);

	createValue (current_id, "current", "ID of current target", false);
	createValue (current_id_sel, "current_sel",
		"ID of currently selected target", false);
	createValue (current_name, "current_name", "name of current target", false);
	createValue (current_type, "current_type", "type of current target", false);
	createValue (current_obsid, "obsid", "ID of observation", false);

	createValue (autoLoop, "auto_loop", "if enabled, observation will loop on its own after current script ends", false);
	autoLoop->setValueBool (true);

	createValue (next_id, "next", "ID of next target", false);
	createValue (next_name, "next_name", "name of next target", false);
	createValue (next_ids, "next_ids", "IDs of next target(s)", false);
	createValue (next_names, "next_names", "name of next target(s)", false);

	createValue (img_id, "img_id", "ID of current image", false);

	createValue (doDarks, "do_darks", "if darks target should be picked by executor", false);
	doDarks->setValueBool (true);

	createValue (ignoreDay, "ignore_day", "whenever executor should run in daytime", false);
	ignoreDay->setValueBool (false);

	addOption (OPT_IGNORE_DAY, "ignore-day", 0, "observe even during daytime");
	addOption (OPT_DONT_DARK, "no-dark", 0, "do not take on its own dark frames");
}


Executor::~Executor (void)
{
	postEvent (new Rts2Event (EVENT_KILL_ALL));
	delete currentTarget;
	clearNextTargets ();
}


int
Executor::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_IGNORE_DAY:
			ignoreDay->setValueBool (true);
			break;
		case OPT_DONT_DARK:
			doDarks->setValueBool (false);
			break;
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}


int
Executor::reloadConfig ()
{
	int ret;
	Rts2Config *config;
	ret = Rts2DeviceDb::reloadConfig ();
	if (ret)
		return ret;
	config = Rts2Config::instance ();
	observer = config->getObserver ();
	config->getDouble ("grbd", "seplimit", grb_sep_limit);
	config->getDouble ("grbd", "minsep", grb_min_sep);
	return 0;
}


int
Executor::setValue (Rts2Value *oldValue, Rts2Value *newValue)
{
	if (oldValue == doDarks || oldValue == ignoreDay)
		return 0;
	return Rts2DeviceDb::setValue (oldValue, newValue);
}


Rts2DevClient *
Executor::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new Rts2DevClientTelescopeExec (conn);
		case DEVICE_TYPE_CCD:
			return new Rts2DevClientCameraExecDb (conn);
		case DEVICE_TYPE_FOCUS:
			return new Rts2DevClientFocusImage (conn);
		case DEVICE_TYPE_PHOT:
			return new Rts2DevClientPhotExec (conn);
		case DEVICE_TYPE_MIRROR:
			return new Rts2DevClientMirrorExec (conn);
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_SENSOR:
			return new Rts2DevClientWriteImage (conn);
		default:
			return Rts2DeviceDb::createOtherType (conn, other_device_type);
	}
}


void
Executor::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_SLEW_TO_TARGET:
			maskState (EXEC_STATE_MASK, EXEC_MOVE);
			break;
		case EVENT_OBSERVE:
			// we can get EVENT_OBSERVE in case of continues observation
		case EVENT_SCRIPT_STARTED:
			maskState (EXEC_STATE_MASK, EXEC_OBSERVE);
			break;
		case EVENT_ACQUIRE_START:
			maskState (EXEC_STATE_MASK, EXEC_ACQUIRE);
			break;
		case EVENT_ACQUIRE_WAIT:
			maskState (EXEC_STATE_MASK, EXEC_ACQUIRE_WAIT);
			break;
		case EVENT_ACQUSITION_END:
			// we receive event before any connection - connections
			// receive it from Rts2Device.
			// So we can safely change target status here, and it will
			// propagate to devices connections
			maskState (EXEC_STATE_MASK, EXEC_OBSERVE);
			switch (*(int *) event->getArg ())
			{
				case NEXT_COMMAND_PRECISION_OK:
					if (currentTarget)
					{
						logStream (MESSAGE_DEBUG) << "NEXT_COMMAND_PRECISION_OK " <<
							currentTarget->getObsTargetID () << sendLog;
						currentTarget->acqusitionEnd ();
					}
					maskState (EXEC_MASK_ACQ, EXEC_ACQ_OK);
					break;
				case -5:
				case NEXT_COMMAND_PRECISION_FAILED:
					if (currentTarget)
						currentTarget->acqusitionFailed ();
					maskState (EXEC_MASK_ACQ, EXEC_ACQ_FAILED);
					break;
			}
			break;
		case EVENT_LAST_READOUT:
			updateScriptCount ();
			// that was last script running
			if (scriptCount->getValueInteger () == 0)
			{
				maskState (EXEC_STATE_MASK, EXEC_LASTREAD);
				switchTarget ();
			}
			break;
		case EVENT_SCRIPT_ENDED:
			updateScriptCount ();
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) <<
				"EVENT_SCRIPT_ENDED Executor currentTarget " << currentTarget <<
				" nextTarget " << nextTargets.size () << sendLog;
		#endif					 /* DEBUG_EXTRA */
			if (currentTarget)
			{
				if (scriptCount->getValueInteger () == 0
					&& currentTarget->observationStarted ())
				{
					maskState (EXEC_STATE_MASK, EXEC_IDLE);
					switchTarget ();
				}
				// scriptCount is not 0, but we hit continues target..
				else if (currentTarget->isContinues () == 1
					&& (nextTargets.size () == 0 || nextTargets.front()->getTargetID () == currentTarget->getTargetID ())
					)
				{
					// wait, if we are in stop..don't que it again..
					if ((getState () & EXEC_MASK_END) != EXEC_END)
						event->setArg ((void *) currentTarget);
					// that will eventually hit devclient which post that message, which
					// will set currentTarget to this value and handle it same way as EVENT_OBSERVE,
					// which is exactly what we want
				}
			}
			else
			{
				if (scriptCount->getValueInteger () == 0)
					switchTarget ();
			}
			break;
		case EVENT_MOVE_OK:
			if (waitState)
			{
				postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
				break;
			}
			postEvent (new Rts2Event (EVENT_OBSERVE));
			break;
		case EVENT_CORRECTING_OK:
			if (waitState)
			{
				postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
			}
			else
			{
				// we aren't waiting, let's observe target again..
				postEvent (new Rts2Event (EVENT_OBSERVE));
			}
			break;
		case EVENT_MOVE_FAILED:
			if (*((int *) event->getArg ()) == DEVICE_ERROR_KILL)
			{
				break;
			}
			postEvent (new Rts2Event (EVENT_STOP_OBSERVATION));
			if (waitState)
			{
				postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
			}
			else if (currentTarget)
			{
				// get us lover priority to prevent moves to such dangerous
				// position
				currentTarget->changePriority (-100,
					ln_get_julian_from_sys () +
					12 * (1.0 / 1440.0));
			}
			updateScriptCount ();
			if (scriptCount->getValueInteger () == 0)
				switchTarget ();
			break;
		case EVENT_ENTER_WAIT:
			waitState = 1;
			break;
		case EVENT_CLEAR_WAIT:
			waitState = 0;
			break;
		case EVENT_GET_ACQUIRE_STATE:
			*((int *) event->getArg ()) =
				(currentTarget) ? currentTarget->getAcquired () : -2;
			break;
	}
	Rts2Device::postEvent (event);
}


void
Executor::deviceReady (Rts2Conn * conn)
{
	if (currentTarget)
		conn->postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
}


int
Executor::info ()
{
	updateScriptCount ();
	if (currentTarget)
	{
		current_id->setValueInteger (currentTarget->getObsTargetID ());
		current_name->setValueCharArr (currentTarget->getTargetName ());
		current_id_sel->setValueInteger (currentTarget->getTargetID ());
		current_obsid->setValueInteger (currentTarget->getObsId ());
		img_id->setValueInteger (currentTarget->getCurrImgId ());
	}
	else
	{
		current_id->setValueInteger (-1);
		current_id_sel->setValueInteger (-1);
		current_name->setValueCharArr (NULL);
		current_obsid->setValueInteger (-1);
		img_id->setValueInteger (-1);
	}
	std::vector <int> _id_arr;
	std::vector <std::string> _name_arr;
	if (nextTargets.size () != 0)
	{
		next_id->setValueInteger (nextTargets.front ()->getTargetID ());
		next_name->setValueCharArr (nextTargets.front ()->getTargetName ());
		for (std::list <Target *>::iterator iter = nextTargets.begin (); iter != nextTargets.end (); iter++)
		{
			_id_arr.push_back ((*iter)->getTargetID ());
			_name_arr.push_back ((*iter)->getTargetName ());
		}
		next_ids->setValueArray (_id_arr);
		next_names->setValueArray (_name_arr);
	}
	else
	{
		next_id->setValueInteger (-1);
		next_name->setValueCharArr (NULL);
		next_ids->setValueArray (_id_arr);
		next_names->setValueArray (_name_arr);
	}

	return Rts2DeviceDb::info ();
}


int
Executor::changeMasterState (int new_state)
{
	if (ignoreDay->getValueBool () == true)
		return Rts2DeviceDb::changeMasterState (new_state);

	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_EVENING:
		case SERVERD_DAWN:
		case SERVERD_NIGHT:
		case SERVERD_DUSK:
			// unblock stop state
			if ((getState () & EXEC_MASK_END) == EXEC_END)
				maskState (EXEC_MASK_END, EXEC_NOT_END);
			if (!currentTarget && nextTargets.size () != 0)
				switchTarget ();
			break;
		case (SERVERD_DAWN | SERVERD_STANDBY):
		case (SERVERD_NIGHT | SERVERD_STANDBY):
		case (SERVERD_DUSK | SERVERD_STANDBY):
			clearNextTargets ();
			// next will be dark..
			if (doDarks->getValueBool () == true)
			{
				nextTargets.push_front (createTarget (1, observer));
				if (!currentTarget)
					switchTarget ();
			}
			break;
		default:
			// we need to stop observation that is continuus
			// that will guarantie that in isContinues call, we will not que our target again
			end ();
			break;
	}
	return Rts2DeviceDb::changeMasterState (new_state);
}


int
Executor::setNext (int nextId)
{
	if (nextTargets.size() != 0)
	{
		if (nextTargets.front ()->getTargetID () == nextId)
			// we observe the same target..
			return 0;
		clearNextTargets ();
	}
	return queueTarget (nextId);
}


int
Executor::queueTarget (int tarId)
{
	Target *nt = createTarget (tarId, observer);
	if (!nt)
	{
		return -2;
	}
	if (nt->getTargetType () == TYPE_DARK && doDarks->getValueBool () == false)
	{
		stop ();
		delete nt;
		return 0;
	}
	nextTargets.push_back (nt);
	if (!currentTarget)
		switchTarget ();
	else
		infoAll ();
	return 0;
}


int
Executor::setNow (int nextId)
{
	Target *newTarget;

	if (!currentTarget)
		return setNext (nextId);

	newTarget = createTarget (nextId, observer);
	if (!newTarget)
	{
		// error..
		return -2;
	}

	return setNow (newTarget);
}


int
Executor::setNow (Target * newTarget)
{
	if (currentTarget)
	{
		currentTarget->endObservation (-1);
		processTarget (currentTarget);
	}
	currentTarget = newTarget;

	// at this situation, we would like to get rid of nextTarget as
	// well
	clearNextTargets ();

	clearAll ();
	postEvent (new Rts2Event (EVENT_KILL_ALL));
	queAll (new Rts2CommandKillAll (this));

	postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
	postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET));

	infoAll ();

	return 0;
}


int
Executor::setGrb (int grbId)
{
	Target *grbTarget;
	struct ln_hrz_posn grbHrz;
	int ret;

	// is during night and ready?
	if (!(getMasterState () == SERVERD_NIGHT
		|| getMasterState () == SERVERD_DUSK
		|| getMasterState () == SERVERD_DAWN))
	{
		logStream (MESSAGE_DEBUG) << "Executor::setGrb daylight GRB ignored"
			<< sendLog;
		return -2;
	}

	grbTarget = createTarget (grbId, observer);

	if (!grbTarget)
	{
		return -2;
	}
	grbTarget->getAltAz (&grbHrz);
	logStream (MESSAGE_DEBUG) << "Rts2Executor::setGrb grbHrz alt:" << grbHrz.alt << sendLog;
	if (grbHrz.alt < 0)
	{
		delete grbTarget;
		return -2;
	}
	// if we're already disabled, don't execute us
	if (grbTarget->getTargetEnabled () == false)
	{
		logStream (MESSAGE_INFO)
			<< "Ignored execution request for GRB target " << grbTarget->
			getTargetName () << " (# " << grbTarget->
			getObsTargetID () << ") because this target is disabled" << sendLog;
		return -2;
	}
	if (!currentTarget)
	{
		return setNow (grbTarget);
	}
	// it's not same..
	ret = grbTarget->compareWithTarget (currentTarget, grb_sep_limit);
	if (ret == 0)
	{
		return setNow (grbTarget);
	}
	// if that's only few arcsec update, don't change
	ret = grbTarget->compareWithTarget (currentTarget, grb_min_sep);
	if (ret == 1)
	{
		return 0;
	}
	// otherwise set us as next target
	clearNextTargets ();
	nextTargets.push_back (grbTarget);
	return 0;
}


int
Executor::setShower ()
{
	// is during night and ready?
	if (!(getMasterState () == SERVERD_NIGHT))
	{
		logStream (MESSAGE_DEBUG) <<
			"Executor::setShower daylight shower ignored" << sendLog;
		return -2;
	}

	return setNow (TARGET_SHOWER);
}


int
Executor::stop ()
{
	postEvent (new Rts2Event (EVENT_STOP_OBSERVATION));
	updateScriptCount ();
	if (scriptCount->getValueInteger () == 0)
		switchTarget ();
	return 0;
}


void
Executor::clearNextTargets ()
{
	for (std::list <Target *>::iterator iter = nextTargets.begin (); iter != nextTargets.end (); iter++)
	{
		delete *iter;
	}
	nextTargets.clear ();
}


void
Executor::queDarks ()
{
	Rts2Conn *minConn = getMinConn ("que_size");
	if (!minConn)
		return;
	minConn->queCommand (new Rts2Command (this, "que_darks"));
}


void
Executor::queFlats ()
{
	Rts2Conn *minConn = getMinConn ("que_size");
	if (!minConn)
		return;
	minConn->queCommand (new Rts2Command (this, "que_flats"));
}


void
Executor::beforeChange ()
{
	// both currentTarget and nextTarget are defined
	char currType;
	char nextType;
	if (currentTarget)
		currType = currentTarget->getTargetType ();
	else
		currType = TYPE_UNKNOW;
	if (nextTargets.size () != 0)
		nextType = nextTargets.front ()->getTargetType ();
	else
		nextType = currType;
	if (currType == TYPE_DARK && nextType != TYPE_DARK)
		queDarks ();
	if (currType == TYPE_FLAT && nextType != TYPE_FLAT)
		queFlats ();
}


void
Executor::doSwitch ()
{
	int ret;
	int nextId;
	// we need to change current target - usefull for planner runs
	if (currentTarget && currentTarget->isContinues () == 2
		&& (nextTargets.size () == 0 || nextTargets.front ()->getTargetID () == currentTarget->getTargetID ()))
	{
		if (nextTargets.size () != 0)
			nextTargets.pop_front ();
		// create again our target..since conditions changed, we will get different target id
		nextTargets.push_front (createTarget (currentTarget->getTargetID (), observer));
	}
	// check dark and flat processing
	beforeChange ();
	if (nextTargets.size () != 0)
	{
		// go to post-process
		if (currentTarget)
		{
			// next target is defined - tested on line -5
			nextId = (*nextTargets.begin ())->getTargetID ();
			ret = currentTarget->endObservation (nextId);
			if (!(ret == 1 && nextId == currentTarget->getTargetID ()))
				// don't que only in case nextTarget and currentTarget are
				// same and endObservation returns 1
			{
				processTarget (currentTarget);
				currentTarget = nextTargets.front ();
				nextTargets.pop_front ();
			}
		}
		else
		{
			currentTarget = nextTargets.front ();
			nextTargets.pop_front ();
		}
	}
	if (currentTarget)
	{
		// send script_ends to all devices..
		queAll (new Rts2CommandScriptEnds (this));
		postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
		postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET));
	}
}


void
Executor::switchTarget ()
{
	if (((getState () & EXEC_MASK_END) == EXEC_END)
		|| (autoLoop->getValueBool () == false && nextTargets.size () == 0))
	{
		maskState (EXEC_MASK_END, EXEC_NOT_END);
		postEvent (new Rts2Event (EVENT_KILL_ALL));
		if (currentTarget)
		{
			currentTarget->endObservation (-1);
			processTarget (currentTarget);
		}
		currentTarget = NULL;
		clearNextTargets ();
	}
	else if (ignoreDay->getValueBool () == true)
	{
		doSwitch ();
	}
	else
	{
		// we will not observe during daytime..
		switch (getMasterState ())
		{
			case SERVERD_EVENING:
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				doSwitch ();
				break;
			case SERVERD_DUSK | SERVERD_STANDBY:
			case SERVERD_DAWN | SERVERD_STANDBY:
				if (!currentTarget && nextTargets.size () != 0 && nextTargets.front ()->getTargetID () == 1)
				{
					// switch to dark..
					doSwitch ();
					break;
				}
			default:
				if (currentTarget)
				{
					currentTarget->endObservation (-1);
					processTarget (currentTarget);
				}
				currentTarget = NULL;
				clearNextTargets ();
		}
	}
	infoAll ();
}


void
Executor::processTarget (Target * in_target)
{
	int ret;
	// test for final acq..
	switch (in_target->getAcquired ())
	{
		case 1:
			acqusitionOk++;
			break;
		case -1:
			acqusitionFailed++;
			break;
	}
	ret = in_target->postprocess ();
	if (!ret)
		targetsQue.push_back (in_target);
	else
		delete in_target;
}


void
Executor::updateScriptCount ()
{
	int scriptRunning = 0;
	postEvent (new Rts2Event (EVENT_SCRIPT_RUNNING_QUESTION, (void *) &scriptRunning));
	scriptCount->setValueInteger (scriptRunning);
}


int
Executor::commandAuthorized (Rts2Conn * conn)
{
	int tar_id;
	if (conn->isCommand ("grb"))
	{
		// change observation if we are to far from GRB position..
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		return setGrb (tar_id);
	}
	else if (conn->isCommand ("shower"))
	{
		if (!conn->paramEnd ())
			return -2;
		return setShower ();
	}
	else if (conn->isCommand ("now"))
	{
		// change observation imediatelly - in case of burst etc..
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		return setNow (tar_id);
	}
	else if (conn->isCommand ("queue"))
	{
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		return queueTarget (tar_id);
	}
	else if (conn->isCommand ("next"))
	{
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		return setNext (tar_id);
	}
	else if (conn->isCommand ("end"))
	{
		if (!conn->paramEnd ())
			return -2;
		return end ();
	}
	else if (conn->isCommand ("stop"))
	{
		if (!conn->paramEnd ())
			return -2;
		return stop ();
	}
	else if (conn->isCommand ("clear"))
	{
		if (!conn->paramEnd ())
		  	return -2;
		clearNextTargets ();
		return 0;
	}
	return Rts2DeviceDb::commandAuthorized (conn);
}


int
main (int argc, char **argv)
{
	Executor executor = Executor (argc, argv);
	return executor.run ();
}
