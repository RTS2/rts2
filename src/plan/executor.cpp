/*
 * Executor body.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "../utils/valuearray.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"
#include "executorque.h"
#include "rts2execcli.h"
#include "rts2execclidb.h"
#include "rts2devcliphot.h"

#define OPT_IGNORE_DAY    OPT_LOCAL + 100
#define OPT_DONT_DARK     OPT_LOCAL + 101

namespace rts2plan
{

/**
 * Executor class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Executor:public Rts2DeviceDb
{
	public:
		Executor (int argc, char **argv);
		virtual ~ Executor (void);
		virtual Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);

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

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

		virtual int setValue (Rts2Value *oldValue, Rts2Value *newValue);

	private:
		rts2db::Target * currentTarget;

		void clearNextTargets ();

		void queDarks ();
		void beforeChange ();
		void doSwitch ();
		void switchTarget ();

		int setNext (int nextId, const char *que = "next");
		int queueTarget (int nextId, double t_start = rts2_nan("f"), double t_end = rts2_nan ("f"));
		int setNow (int nextId);
		int setGrb (int grbId);
		int setShower ();

		// -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
		Rts2ValueInteger *scriptCount;
		int waitState;
		std::list < rts2db::Target * > targetsQue;
		struct ln_lnlat_posn *observer;

		// Queue management
		std::list < ExecutorQueue > queues;
		Rts2ValueSelection *activeQueue;

		ExecutorQueue * getActiveQueue () { return (ExecutorQueue *) (activeQueue->getData ()); }

		void createQueue (const char *name);

		Rts2ValueDouble *grb_sep_limit;
		Rts2ValueDouble *grb_min_sep;

		Rts2ValueBool *enabled;

		Rts2ValueInteger *acqusitionOk;
		Rts2ValueInteger *acqusitionFailed;

		int setNow (rts2db::Target * newTarget);

		void processTarget (rts2db::Target * in_target);
		void updateScriptCount ();

		Rts2ValueInteger *current_id;
		Rts2ValueInteger *current_id_sel;
		Rts2ValueString *current_name;
		Rts2ValueString *current_type;
		Rts2ValueInteger *current_obsid;

		Rts2ValueString *pi;
		Rts2ValueString *program;

		Rts2ValueBool *autoLoop;

		Rts2ValueInteger *next_id;
		Rts2ValueString *next_name;

		Rts2ValueBool *doDarks;
		Rts2ValueBool *flatsDone;
		Rts2ValueBool *ignoreDay;

		Rts2ValueInteger *img_id;
};

}

using namespace rts2plan;

Executor::Executor (int in_argc, char **in_argv):Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_EXECUTOR, "EXEC")
{
	currentTarget = NULL;
	createValue (scriptCount, "script_count", "number of running scripts", false);
	scriptCount->setValueInteger (-1);

	waitState = 0;

	createValue (enabled, "enabled", "if false, executor will not perform its duties, thus enabling problem-free full manual control", false, RTS2_VALUE_WRITABLE);
	enabled->setValueBool (true);

	createValue (acqusitionOk, "acqusition_ok", "number of acqusitions completed sucesfully", false);
	acqusitionOk->setValueInteger (0);

	createValue (acqusitionFailed, "acqusition_failed", "number of acqusitions which failed", false);
	acqusitionFailed->setValueInteger (0);

	createValue (current_id, "current", "ID of current target", false);
	createValue (current_id_sel, "current_sel", "ID of currently selected target", false);
	createValue (current_name, "current_name", "name of current target", false);
	createValue (current_type, "current_type", "type of current target", false);
	createValue (current_obsid, "obsid", "ID of observation", false);

	createValue (pi, "PI", "project investigator of the target", true, RTS2_VALUE_WRITABLE);
	createValue (program, "PROGRAM", "target program name", true, RTS2_VALUE_WRITABLE);

	createValue (autoLoop, "auto_loop", "if enabled, observation will loop on its own after current script ends", false, RTS2_VALUE_WRITABLE);
	autoLoop->setValueBool (true);

	createValue (next_id, "next", "ID of next target", false, RTS2_VALUE_WRITABLE);
	createValue (next_name, "next_name", "name of next target", false);

	createValue (activeQueue, "queue", "selected queueu", true, RTS2_VALUE_WRITABLE);
	createQueue ("next");

	createValue (img_id, "img_id", "ID of current image", false);

	createValue (doDarks, "do_darks", "if darks target should be picked by executor", false, RTS2_VALUE_WRITABLE);
	doDarks->setValueBool (true);

	createValue (flatsDone, "flats_done", "if current skyflat routine was finished", false, RTS2_VALUE_WRITABLE);
	flatsDone->setValueBool (false);

	createValue (ignoreDay, "ignore_day", "whenever executor should run in daytime", false, RTS2_VALUE_WRITABLE);
	ignoreDay->setValueBool (false);

	createValue (grb_sep_limit, "grb_sep_limit", "[deg] when GRB distane is above grb_sep_limit degrees from current position, telescope will be immediatelly slewed to new position", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	grb_sep_limit->setValueDouble (0);

	createValue (grb_min_sep, "grb_min_sep", "[deg] when GRB is bellow grb_min_sep degrees from current position, telescope will not be slewed", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	grb_min_sep->setValueDouble (0);

	addOption (OPT_IGNORE_DAY, "ignore-day", 0, "observe even during daytime");
	addOption (OPT_DONT_DARK, "no-dark", 0, "do not take on its own dark frames");
}

Executor::~Executor (void)
{
	postEvent (new Rts2Event (EVENT_KILL_ALL));
	delete currentTarget;
	clearNextTargets ();
}

int Executor::processOption (int in_opt)
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

int Executor::reloadConfig ()
{
	int ret;
	double f;
	Rts2Config *config;
	ret = Rts2DeviceDb::reloadConfig ();
	if (ret)
		return ret;
	config = Rts2Config::instance ();
	observer = config->getObserver ();
	f = 0;
	config->getDouble ("grbd", "seplimit", f);
	grb_sep_limit->setValueDouble (f);

	f = 0;
	config->getDouble ("grbd", "minsep", f);
	grb_min_sep->setValueDouble (f);

	return 0;
}

int Executor::setValue (Rts2Value *oldValue, Rts2Value *newValue)
{
	if (oldValue == enabled && ((Rts2ValueBool *) newValue)->getValueBool () == false)
	{
		stop ();
		return 0;
	}
	if (oldValue == next_id)
	{
		return setNext (newValue->getValueInteger ()) ? 0 : -2;
	}
	return Rts2DeviceDb::setValue (oldValue, newValue);
}

Rts2DevClient * Executor::createOtherType (Rts2Conn * conn, int other_device_type)
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
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_SENSOR:
			return new Rts2DevClientWriteImage (conn);
		default:
			return Rts2DeviceDb::createOtherType (conn, other_device_type);
	}
}

void Executor::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_SLEW_TO_TARGET:
		case EVENT_SLEW_TO_TARGET_NOW:
			maskState (EXEC_STATE_MASK, EXEC_MOVE);
			break;
		case EVENT_NEW_TARGET:
			currentTarget->newObsSlew ((struct ln_equ_posn *) event->getArg ());
			break;
		case EVENT_CHANGE_TARGET:
			currentTarget->updateSlew ((struct ln_equ_posn *) event->getArg ());
			break;
		case EVENT_NEW_TARGET_ALTAZ:
		case EVENT_CHANGE_TARGET_ALTAZ:
			{
				struct ln_hrz_posn *hrz = (struct ln_hrz_posn *) event->getArg ();
				struct ln_equ_posn pos;
				ln_get_equ_from_hrz (hrz, observer, ln_get_julian_from_sys (), &pos);
				if (event->getType () == EVENT_NEW_TARGET_ALTAZ)
					currentTarget->newObsSlew (&pos);
				else
					currentTarget->updateSlew (&pos);
			}
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
				" next que  " << getActiveQueue ()->size () << sendLog;
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
					&& (getActiveQueue ()->size () == 0 || getActiveQueue ()->front ().target->getTargetID () == currentTarget->getTargetID ())
					)
				{
					// wait, if we are in stop..don't queue it again..
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
				currentTarget->changePriority (-100, ln_get_julian_from_sys () + 12 * (1.0 / 1440.0));
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

void Executor::deviceReady (Rts2Conn * conn)
{
	if (currentTarget)
		conn->postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
}

int Executor::info ()
{
  	char buf[2];
	updateScriptCount ();
	if (currentTarget)
	{
		current_id->setValueInteger (currentTarget->getObsTargetID ());
		current_id_sel->setValueInteger (currentTarget->getTargetID ());
		buf[0] = currentTarget->getTargetType ();
		buf[1] = '\0';
		current_type->setValueCharArr (buf);
		current_name->setValueCharArr (currentTarget->getTargetName ());
		img_id->setValueInteger (currentTarget->getCurrImgId ());
		current_obsid->setValueInteger (currentTarget->getObsId ());
	}
	else
	{
		current_id->setValueInteger (-1);
		current_id_sel->setValueInteger (-1);
		current_name->setValueCharArr (NULL);
		img_id->setValueInteger (-1);
		current_obsid->setValueInteger (-1);
	}


	if (getActiveQueue ()->empty ())
	{
		next_id->setValueInteger (-1);
		next_name->setValueCharArr (NULL);
	}
	else
	{
		next_id->setValueInteger (getActiveQueue ()->front ().target->getTargetID ());
		next_name->setValueCharArr (getActiveQueue ()->front ().target->getTargetName ());
	}

	return Rts2DeviceDb::info ();
}

int Executor::changeMasterState (int new_state)
{
	if (ignoreDay->getValueBool () == true)
		return Rts2DeviceDb::changeMasterState (new_state);

	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_NIGHT:
			flatsDone->setValueBool (false);
			sendValueAll (flatsDone);
		case SERVERD_MORNING:
			// switch target if current is flats..
			if (currentTarget && currentTarget->getTargetType () == TYPE_FLAT)
				queueTarget (activeQueue->getValueInteger ());
		case SERVERD_EVENING:
		case SERVERD_DAWN:
		case SERVERD_DUSK:
			// unblock stop state
			if ((getState () & EXEC_MASK_END) == EXEC_END)
				maskState (EXEC_MASK_END, EXEC_NOT_END);
			if (!currentTarget && getActiveQueue ()->size () != 0)
				switchTarget ();
			break;
		case (SERVERD_DAWN | SERVERD_STANDBY):
		case (SERVERD_NIGHT | SERVERD_STANDBY):
		case (SERVERD_DUSK | SERVERD_STANDBY):
			stop ();
			// next will be dark..
			if (doDarks->getValueBool () == true)
			{
			  	try
				{
					getActiveQueue ()->addFront (createTarget (1, observer));
					if (!currentTarget)
						switchTarget ();
				}
				catch (rts2core::Error &er)
				{
					logStream (MESSAGE_ERROR) << "cannot start dark target in standby : " << er << sendLog;
					doDarks->setValueBool (false);
					sendValueAll (doDarks);
				}
			}
			break;
		default:
			// we need to stop observation that is continuus
			// that will guarantie that in isContinues call, we will not queue our target again
			stop ();
			break;
	}
	return Rts2DeviceDb::changeMasterState (new_state);
}

int Executor::setNext (int nextId, const char *queue)
{
	if (getActiveQueue ()->size() != 0)
	{
		if (getActiveQueue ()->front ().target->getTargetID () == nextId && (!currentTarget || currentTarget->getTargetType () != TYPE_FLAT))
			// asked for same target
			return 0;
		clearNextTargets ();
	}
	return queueTarget (nextId);
}

int Executor::queueTarget (int tarId, double t_start, double t_end)
{
	try
	{
		rts2db::Target *nt = createTarget (tarId, observer);
		if (!nt)
			return -2;
		if (nt->getTargetType () == TYPE_DARK && doDarks->getValueBool () == false)
		{
			stop ();
			delete nt;
			return 0;
		}
		if (currentTarget && currentTarget->getTargetType () == TYPE_FLAT && nt->getTargetType () != TYPE_FLAT
			 && (getMasterState () == SERVERD_NIGHT || getMasterState () == SERVERD_MORNING))
		{
			return setNow (nt);
		}
		// switch immediately to flats..
		if (nt->getTargetType () == TYPE_FLAT && (!currentTarget || currentTarget->getTargetType () != TYPE_FLAT))
		{
			return setNow (nt);
		}
		// overwrite darks with something usefull..
		if (currentTarget && currentTarget->getTargetType () == TYPE_DARK && nt->getTargetType () != TYPE_DARK)
		{
			return setNow (nt);
		}
		getActiveQueue ()->addTarget (nt, t_start, t_end);
		if (!currentTarget)
			switchTarget ();
		else
			infoAll ();
	}
	catch (rts2core::Error &ex)
	{
		logStream (MESSAGE_ERROR) << "cannot queue target with ID " << tarId << " :" << ex << sendLog;
		return -2;
	}
	return 0;
}

void Executor::createQueue (const char *name)
{
  	queues.push_back (ExecutorQueue (this, name, &observer));
	activeQueue->addSelVal (name, (Rts2SelData *) &(queues.back()));
}

int Executor::setNow (int nextId)
{
	rts2db::Target *newTarget;

	if (!currentTarget)
		return setNext (nextId);

	try
	{
		newTarget = createTarget (nextId, observer);
		if (!newTarget)
			// error..
			return -2;
		return setNow (newTarget);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot set now target with ID " << nextId << " : " << er << sendLog;
		return -2;
	}
}

int Executor::setNow (rts2db::Target * newTarget)
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
	postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET_NOW));

	infoAll ();

	struct ln_equ_posn pos;
	currentTarget->getPosition (&pos);

	logStream (MESSAGE_INFO) << "executing now target " << currentTarget->getTargetName () << " at RA DEC " << LibnovaRaDec (&pos) << sendLog;
	return 0;
}

int Executor::setGrb (int grbId)
{
	rts2db::Target *grbTarget;
	struct ln_hrz_posn grbHrz;
	int ret;

	// is during night and ready?
	if (!(getMasterState () == SERVERD_NIGHT || getMasterState () == SERVERD_DUSK || getMasterState () == SERVERD_DAWN))
	{
		logStream (MESSAGE_DEBUG) << "daylight / not on state GRB ignored" << sendLog;
		return -2;
	}
	try
	{
		grbTarget = createTarget (grbId, observer);

		if (!grbTarget)
			return -2;
		grbTarget->getAltAz (&grbHrz);
		if (grbHrz.alt < 0)
		{
			logStream (MESSAGE_DEBUG) << "GRB is bellow horizon and is ignored. GRB altitude " << grbHrz.alt << sendLog;
			delete grbTarget;
			return -2;
		}
		// if we're already disabled, don't execute us
		if (grbTarget->getTargetEnabled () == false)
		{
			logStream (MESSAGE_INFO)
				<< "ignored execution request for GRB target " << grbTarget->getTargetName ()
				<< " (# " << grbTarget->getObsTargetID () << ") because this target is disabled" << sendLog;
			return -2;
		}
		if (!currentTarget)
		{
			return setNow (grbTarget);
		}
		// it's not same..
		ret = grbTarget->compareWithTarget (currentTarget, grb_sep_limit->getValueDouble ());
		if (ret == 0)
		{
			return setNow (grbTarget);
		}
		// if that's only few arcsec update, don't change
		ret = grbTarget->compareWithTarget (currentTarget, grb_min_sep->getValueDouble ());
		if (ret == 1)
		{
			logStream (MESSAGE_INFO) << "GRB update for target " << grbTarget->getTargetName () << " (#"
				<< grbTarget->getObsTargetID () << ") ignored, as its distance from current target "
				<< currentTarget->getTargetName () << " (#" << currentTarget->getObsTargetID ()
				<< ") is bellow separation limit of " << LibnovaDegDist (grb_min_sep->getValueDouble ())
				<< "." << sendLog;
			return 0;
		}
		// otherwise set us as next target
		clearNextTargets ();
		getActiveQueue ()->addTarget (grbTarget);
		return 0;
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot set grb target with ID " << grbId << " :" << er << sendLog;
		return -2;
	}
}

int Executor::setShower ()
{
	// is during night and ready?
	if (!(getMasterState () == SERVERD_NIGHT))
	{
		logStream (MESSAGE_DEBUG) << "daylight shower ignored" << sendLog;
		return -2;
	}

	return setNow (TARGET_SHOWER);
}

int Executor::stop ()
{
	flatsDone->setValueBool (false);
	sendValueAll (flatsDone);
	clearNextTargets ();
	postEvent (new Rts2Event (EVENT_KILL_ALL));
	postEvent (new Rts2Event (EVENT_STOP_OBSERVATION));
	updateScriptCount ();
	if (scriptCount->getValueInteger () == 0)
		switchTarget ();
	return 0;
}

void Executor::clearNextTargets ()
{
  	getActiveQueue ()->clearNext (currentTarget);
	sendValueAll (next_id);
	sendValueAll (next_name);
	logStream (MESSAGE_DEBUG) << "cleared list of next targets" << sendLog;
}

void Executor::queDarks ()
{
	Rts2Conn *minConn = getMinConn ("queue_size");
	if (!minConn)
		return;
	minConn->queCommand (new Rts2Command (this, "que_darks"));
}

void Executor::beforeChange ()
{
	// both currentTarget and nextTarget are defined
	char currType;
	char nextType;
	if (currentTarget)
		currType = currentTarget->getTargetType ();
	else
		currType = TYPE_UNKNOW;
	if (getActiveQueue ()->size () != 0)
		nextType = getActiveQueue ()->front ().target->getTargetType ();
	else
		nextType = currType;
	if (currType == TYPE_DARK && nextType != TYPE_DARK)
		queDarks ();
}

void Executor::doSwitch ()
{
	int ret;
	int nextId;
	// make sure queue is configured for target change
	getActiveQueue ()->beforeChange ();
	// we need to change current target - usefull for planner runs
	if (currentTarget && currentTarget->isContinues () == 2 && (getActiveQueue ()->size () == 0 || getActiveQueue ()->front ().target->getTargetID () == currentTarget->getTargetID ()))
	{
		if (getActiveQueue ()->size () != 0)
			getActiveQueue ()->popFront ();
		// create again our target..since conditions changed, we will get different target id
		getActiveQueue ()->addFront (createTarget (currentTarget->getTargetID (), observer));
	}
	// check dark and flat processing
	beforeChange ();
	if (getActiveQueue ()->size () != 0)
	{
		// go to post-process
		if (currentTarget)
		{
			// next target is defined - tested on line -5
			nextId = getActiveQueue ()->front ().target->getTargetID ();
			ret = currentTarget->endObservation (nextId);
			if (!(ret == 1 && nextId == currentTarget->getTargetID ()))
				// don't queue only in case nextTarget and currentTarget are
				// same and endObservation returns 1
			{
				processTarget (currentTarget);
				currentTarget = getActiveQueue ()->front ().target;
				getActiveQueue ()->popFront ();
			}
			// switch auto loop back to true
			autoLoop->setValueBool (true);
			sendValueAll (autoLoop);
		}
		else
		{
			currentTarget = getActiveQueue ()->front ().target;
			getActiveQueue ()->popFront ();
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

void Executor::switchTarget ()
{
	if (enabled->getValueBool () == false)
	{
		clearNextTargets ();
		logStream (MESSAGE_WARNING) << "please switch executor to enabled to allow it carrying observations" << sendLog;
		return;
	}

	if (((getState () & EXEC_MASK_END) == EXEC_END)
		|| (autoLoop->getValueBool () == false && getActiveQueue ()->size () == 0))
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
				if (!currentTarget && getActiveQueue ()->size () != 0 && getActiveQueue ()->front ().target->getTargetID () == 1)
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

void Executor::processTarget (rts2db::Target * in_target)
{
	int ret;
	// test for final acq..
	switch (in_target->getAcquired ())
	{
		case 1:
			acqusitionOk->inc ();
			break;
		case -1:
			acqusitionFailed->inc ();
			break;
	}
	logStream (MESSAGE_INFO) << "observation # " << current_obsid->getValueInteger () << " of target " << in_target->getTargetName () << " ended." << sendLog;
	// if the target was flat..
	if (in_target->getTargetType () == TYPE_FLAT)
	{
		flatsDone->setValueBool (true);
		sendValueAll (flatsDone);
	}
	ret = in_target->postprocess ();
	if (!ret)
		targetsQue.push_back (in_target);
	else
		delete in_target;
}

void Executor::updateScriptCount ()
{
	int scriptRunning = 0;
	postEvent (new Rts2Event (EVENT_SCRIPT_RUNNING_QUESTION, (void *) &scriptRunning));
	scriptCount->setValueInteger (scriptRunning);
}

int Executor::commandAuthorized (Rts2Conn * conn)
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
		int failed = 0;
		while (!conn->paramEnd ())
		{
			if (conn->paramNextInteger (&tar_id))
			{
				failed++;
				continue;
			}
			int ret = queueTarget (tar_id);
			if (ret)
				failed++;
		}
		if (failed != 0)
			return -2;
		return failed == 0 ? 0 : -2;
	}
	else if (conn->isCommand ("queue_at"))
	{
		int failed = 0;
		double t_start;
		double t_end;
		while (!conn->paramEnd ())
		{
			if (conn->paramNextInteger (&tar_id) || conn->paramNextDouble (&t_start) || conn->paramNextDouble (&t_end))
			{
				failed++;
				continue;
			}
			int ret = queueTarget (tar_id, t_start, t_end);
			if (ret)
				failed++;
		}
		return failed == 0 ? 0 : -2;
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
		end ();
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

int main (int argc, char **argv)
{
	Executor executor (argc, argv);
	return executor.run ();
}
