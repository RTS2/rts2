/*
 * Executor body.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010-2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "valuearray.h"
#include "rts2db/constraints.h"
#include "rts2db/devicedb.h"
#include "rts2db/plan.h"
#include "rts2db/target.h"
#include "rts2db/targetgrb.h"
#include "rts2script/executorque.h"
#include "rts2script/execcli.h"
#include "rts2script/execclidb.h"
#include "rts2devcliphot.h"

#define OPT_IGNORE_DAY    OPT_LOCAL + 100
#define OPT_DONT_DARK     OPT_LOCAL + 101
#define OPT_DISABLE_AUTO  OPT_LOCAL + 102

namespace rts2plan
{

/**
 * Executor class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Executor:public rts2db::DeviceDb
{
	public:
		Executor (int argc, char **argv);
		virtual ~ Executor (void);
		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);

		virtual void postEvent (rts2core::Event * event);

		virtual void deviceReady (rts2core::Connection * conn);

		virtual int info ();

		virtual void changeMasterState (int old_state, int new_state);

		int end ()
		{
			maskState (EXEC_MASK_END, EXEC_END);
			return 0;
		}

		int stop ();

		virtual int commandAuthorized (rts2core::Connection * conn);

		virtual void fileModified (struct inotify_event *event);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int reloadConfig ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

	private:
		rts2db::Target * currentTarget;

		void clearNextTargets ();

		void doSwitch ();
		int switchTarget ();

		int setNext (int nextId);
		int setNextPlan (int nextPlanId);
		int queueTarget (int nextId, double t_start = NAN, double t_end = NAN, int plan_id = -1);
		int setNow (int nextId, int plan_id);
		int setGrb (int grbId);
		int setShower ();

		// -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
		rts2core::ValueInteger *scriptCount;
		int waitState;
		std::list < rts2db::Target * > targetsQue;
		struct ln_lnlat_posn *observer;

		// Queue management
		std::list < ExecutorQueue > queues;
		rts2core::ValueSelection *activeQueue;

		ExecutorQueue * getActiveQueue () { return (ExecutorQueue *) (activeQueue->getData ()); }

		void createQueue (const char *name);

		rts2core::ValueDouble *grb_sep_limit;
		rts2core::ValueDouble *grb_min_sep;

		rts2core::ValueBool *enabled;
		rts2core::ValueBool *selectorNext;
		bool selector_next_reported;

		rts2core::ValueInteger *acqusitionOk;
		rts2core::ValueInteger *acqusitionFailed;

		rts2core::ValueBool *next_night;

		int setNow (rts2db::Target * newTarget, int plan_id);

		void processTarget (rts2db::Target * in_target);
		void updateScriptCount ();

		rts2core::ValueInteger *current_id;
		rts2core::ValueInteger *current_id_sel;
		rts2core::ValueString *current_name;
		rts2core::ValueInteger *current_plan_id;
		rts2core::ValueString *current_type;
		rts2core::ValueDouble *current_errorbox;
		rts2core::ValueInteger *current_obsid;

		rts2core::ValueString *pi;
		rts2core::ValueString *program;

		rts2core::ValueBool *autoLoop;
		rts2core::ValueBool *defaultAutoLoop;

		rts2core::ValueInteger *next_id;
		rts2core::ValueString *next_name;
		rts2core::ValueInteger *next_plan_id;

		rts2core::ValueSelection *doDarks;
		rts2core::ValueBool *flatsDone;
		rts2core::ValueBool *ignoreDay;

		rts2core::ValueInteger *img_id;

		rts2core::ConnNotify *notifyConn;
};

}

using namespace rts2plan;

Executor::Executor (int in_argc, char **in_argv):rts2db::DeviceDb (in_argc, in_argv, DEVICE_TYPE_EXECUTOR, "EXEC")
{
	currentTarget = NULL;
	createValue (scriptCount, "script_count", "number of running scripts", false);
	scriptCount->setValueInteger (-1);

	notifyConn = new rts2core::ConnNotify (this);
	rts2db::MasterConstraints::setNotifyConnection (notifyConn);

	waitState = 0;

	createValue (enabled, "enabled", "if false, executor will not perform its duties, thus enabling problem-free full manual control", false, RTS2_VALUE_WRITABLE);
	enabled->setValueBool (true);

	createValue (selectorNext, "selector_next", "enables execution of next targets from selector", false, RTS2_VALUE_WRITABLE);
	selectorNext->setValueBool (true);
	selector_next_reported = false;

	createValue (acqusitionOk, "acqusition_ok", "number of acqusitions completed sucesfully", false);
	acqusitionOk->setValueInteger (0);

	createValue (acqusitionFailed, "acqusition_failed", "number of acqusitions which failed", false);
	acqusitionFailed->setValueInteger (0);

	createValue (next_night, "next_night", "true if next target is the first target in given night");
	next_night->setValueBool (false);

	createValue (current_id, "current", "ID of current target", false);
	createValue (current_id_sel, "current_sel", "ID of currently selected target", false);
	createValue (current_name, "current_name", "name of current target", false);
	createValue (current_plan_id, "current_plan_id", "plan id of current target", false);
	createValue (current_type, "current_type", "type of current target", false);
	createValue (current_errorbox, "current_errorbox", "current target error box (if any)", false);
	createValue (current_obsid, "obsid", "ID of observation", false);

	createValue (pi, "PI", "project investigator of the target", true, RTS2_VALUE_WRITABLE);
	createValue (program, "PROGRAM", "target program name", true, RTS2_VALUE_WRITABLE);

	createValue (autoLoop, "auto_loop", "if enabled, observation will loop on its own after current script ends", false, RTS2_VALUE_WRITABLE);
	autoLoop->setValueBool (true);

	createValue (defaultAutoLoop, "default_auto_loop", "default state of auto loop (after execution of a script)", false, RTS2_VALUE_WRITABLE);
	defaultAutoLoop->setValueBool (true);

	createValue (next_id, "next", "ID of next target", false, RTS2_VALUE_WRITABLE);
	createValue (next_name, "next_name", "name of next target", false);
	createValue (next_plan_id, "next_plan_id", "next plan ID", false);

	createValue (activeQueue, "queue", "selected queueu", true, RTS2_VALUE_WRITABLE);
	createQueue ("next");

	createValue (img_id, "img_id", "ID of current image", false);

	createValue (doDarks, "do_darks", "if darks target should be picked by executor", false, RTS2_VALUE_WRITABLE);
	doDarks->addSelVal ("not at all");
	doDarks->addSelVal ("just from queue");
	doDarks->addSelVal ("from queue or if system is in standby");
	doDarks->setValueInteger (2);

	createValue (flatsDone, "flats_done", "if current skyflat routine was finished", false, RTS2_VALUE_WRITABLE);
	flatsDone->setValueBool (false);

	createValue (ignoreDay, "ignore_day", "whenever executor should run in daytime", false, RTS2_VALUE_WRITABLE);
	ignoreDay->setValueBool (false);

	createValue (grb_sep_limit, "grb_sep_limit", "[deg] when GRB distane is above grb_sep_limit degrees from current position, telescope will be immediatelly slewed to new position", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	grb_sep_limit->setValueDouble (0);

	createValue (grb_min_sep, "grb_min_sep", "[deg] when GRB is below grb_min_sep degrees from current position, telescope will not be slewed", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEG_DIST);
	grb_min_sep->setValueDouble (0);

	addOption (OPT_IGNORE_DAY, "ignore-day", 0, "observe even during daytime");
	addOption (OPT_DONT_DARK, "no-dark", 0, "do not take on its own dark frames");
	addOption (OPT_DISABLE_AUTO, "no-auto", 0, "disable autolooping");
}

Executor::~Executor (void)
{
	postEvent (new rts2core::Event (EVENT_KILL_ALL));
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
			doDarks->setValueInteger (0);
			break;
		case OPT_DISABLE_AUTO:
			autoLoop->setValueBool (false);
			defaultAutoLoop->setValueBool (false);
			break;
		default:
			return rts2db::DeviceDb::processOption (in_opt);
	}
	return 0;
}

int Executor::init ()
{
	int ret = rts2db::DeviceDb::init ();
	if (ret)
		return ret;
	ret = notifyConn->init ();
	if (ret)
		return ret;
	
	addConnection (notifyConn);

	return ret;
}

int Executor::reloadConfig ()
{
	int ret;
	double f;
	Configuration *config;
	ret = rts2db::DeviceDb::reloadConfig ();
	if (ret)
		return ret;
	config = Configuration::instance ();
	observer = config->getObserver ();
	f = 0;
	config->getDouble ("grbd", "seplimit", f);
	grb_sep_limit->setValueDouble (f);

	f = 0;
	config->getDouble ("grbd", "minsep", f);
	grb_min_sep->setValueDouble (f);

	return 0;
}

int Executor::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == enabled && ((rts2core::ValueBool *) newValue)->getValueBool () == false)
	{
		stop ();
		return 0;
	}
	if (oldValue == next_id)
	{
		return setNext (newValue->getValueInteger ()) ? 0 : -2;
	}
	return rts2db::DeviceDb::setValue (oldValue, newValue);
}

rts2core::DevClient * Executor::createOtherType (rts2core::Connection * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new rts2script::DevClientTelescopeExec (conn);
		case DEVICE_TYPE_CCD:
			return new rts2script::DevClientCameraExecDb (conn);
		case DEVICE_TYPE_FOCUS:
			return new rts2image::DevClientFocusImage (conn);
		case DEVICE_TYPE_PHOT:
			return new rts2script::DevClientPhotExec (conn);
		case DEVICE_TYPE_DOME:
		case DEVICE_TYPE_SENSOR:
			return new rts2image::DevClientWriteImage (conn);
		default:
			return rts2db::DeviceDb::createOtherType (conn, other_device_type);
	}
}

void Executor::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_SLEW_TO_TARGET:
		case EVENT_SLEW_TO_TARGET_NOW:
			maskState (EXEC_STATE_MASK, EXEC_MOVE);
			break;
		case EVENT_NEW_TARGET:
			currentTarget->newObsSlew ((struct ln_equ_posn *) event->getArg (), current_plan_id->getValueInteger ());
			break;
		case EVENT_CHANGE_TARGET:
			currentTarget->updateSlew ((struct ln_equ_posn *) event->getArg (), current_plan_id->getValueInteger ());
			break;
		case EVENT_NEW_TARGET_ALTAZ:
		case EVENT_CHANGE_TARGET_ALTAZ:
			{
				struct ln_hrz_posn *hrz = (struct ln_hrz_posn *) event->getArg ();
				struct ln_equ_posn pos;
				ln_get_equ_from_hrz (hrz, observer, ln_get_julian_from_sys (), &pos);
				if (event->getType () == EVENT_NEW_TARGET_ALTAZ)
					currentTarget->newObsSlew (&pos, current_plan_id->getValueInteger ());
				else
					currentTarget->updateSlew (&pos, current_plan_id->getValueInteger ());
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
			// receive it from rts2core::Device.
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
					if ((getState () & EXEC_MASK_END) != EXEC_END && autoLoop->getValueBool () == true)
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
				postEvent (new rts2core::Event (EVENT_CLEAR_WAIT));
				break;
			}
			postEvent (new rts2core::Event (EVENT_OBSERVE));
			break;
		case EVENT_CORRECTING_OK:
			if (waitState)
			{
				postEvent (new rts2core::Event (EVENT_CLEAR_WAIT));
			}
			else
			{
				// we aren't waiting, let's observe target again..
				postEvent (new rts2core::Event (EVENT_OBSERVE));
			}
			break;
		case EVENT_MOVE_FAILED:
			if (*((int *) event->getArg ()) == DEVICE_ERROR_KILL)
			{
				break;
			}
			postEvent (new rts2core::Event (EVENT_STOP_OBSERVATION));
			if (waitState)
			{
				postEvent (new rts2core::Event (EVENT_CLEAR_WAIT));
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
	rts2core::Device::postEvent (event);
}

void Executor::deviceReady (rts2core::Connection * conn)
{
	if (currentTarget)
		conn->postEvent (new rts2core::Event (EVENT_SET_TARGET, (void *) currentTarget));
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
		current_errorbox->setValueDouble (currentTarget->getErrorBox ());
		current_name->setValueCharArr (currentTarget->getTargetName ());
		img_id->setValueInteger (currentTarget->getCurrImgId ());
		current_obsid->setValueInteger (currentTarget->getObsId ());
	}
	else
	{
		current_id->setValueInteger (-1);
		current_id_sel->setValueInteger (-1);
		current_plan_id->setValueInteger (-1);
		current_errorbox->setValueDouble (NAN);
		current_name->setValueCharArr (NULL);
		img_id->setValueInteger (-1);
		current_obsid->setValueInteger (-1);
	}


	if (getActiveQueue ()->empty ())
	{
		next_id->setValueInteger (-1);
		next_name->setValueCharArr (NULL);
		next_plan_id->setValueInteger (-1);
	}
	else
	{
		next_id->setValueInteger (getActiveQueue ()->front ().target->getTargetID ());
		next_name->setValueCharArr (getActiveQueue ()->front ().target->getTargetName ());
		next_plan_id->setValueInteger (getActiveQueue ()->front ().plan_id);
	}

	return rts2db::DeviceDb::info ();
}

void Executor::changeMasterState (int old_state, int new_state)
{
	if (ignoreDay->getValueBool () == true)
		return rts2db::DeviceDb::changeMasterState (old_state, new_state);

	next_night->setValueBool (false);

	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_ONOFF_MASK))
	{
		case SERVERD_NIGHT:
			flatsDone->setValueBool (false);
			sendValueAll (flatsDone);
			// kill darks if they are running from evening
			if (currentTarget && currentTarget->getTargetType () == TYPE_DARK && !(getActiveQueue ()->empty ()))
			{
				next_night->setValueBool (true);
				setNow (getActiveQueue()->front().target->getTargetID (), getActiveQueue ()->front ().plan_id);
			}
			else
			{
				next_night->setValueBool (true);
			}
			sendValueAll (next_night);
		case SERVERD_MORNING:
			// re-enables selector selection
			selectorNext->setValueBool (true);
			sendValueAll (selectorNext);
			// switch target if current is flats or darks
			if (currentTarget && currentTarget->getTargetType () == TYPE_FLAT && !(getActiveQueue ()->empty ()))
				setNow (getActiveQueue ()->front ().target->getTargetID (), getActiveQueue ()->front ().plan_id);
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
			if (doDarks->getValueInteger () == 2)
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
					doDarks->setValueInteger (1);
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
	return rts2db::DeviceDb::changeMasterState (old_state, new_state);
}

int Executor::setNext (int nextId)
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

int Executor::setNextPlan (int nextPlanId)
{
	if (getActiveQueue ()->size() != 0)
	{
		if (getActiveQueue ()->front ().plan_id == nextPlanId && (!currentTarget || currentTarget->getTargetType () != TYPE_FLAT))
			// asked for same target
			return 0;
		clearNextTargets ();
	}
	rts2db::Plan p (nextPlanId);
	p.load ();

	return queueTarget (p.getTarget ()->getTargetID (), NAN, NAN, nextPlanId);
}

int Executor::queueTarget (int tarId, double t_start, double t_end, int plan_id)
{
	try
	{
		rts2db::Target *nt = createTarget (tarId, observer);
		if (!nt)
			return -2;
		if (nt->getTargetType () == TYPE_DARK && doDarks->getValueInteger () == 0)
		{
			if (currentTarget == NULL || currentTarget->getTargetType () != TYPE_FLAT)
				stop ();
			delete nt;
			return 0;
		}
		if (currentTarget && currentTarget->getTargetType () == TYPE_FLAT && nt->getTargetType () != TYPE_FLAT
			 && (getMasterState () == SERVERD_NIGHT || getMasterState () == SERVERD_MORNING))
		{
			return setNow (nt, plan_id);
		}
		// switch immediately to flats if in twilight
		if ((getMasterState () == SERVERD_DUSK || getMasterState () == SERVERD_DAWN) && nt->getTargetType () == TYPE_FLAT && (!currentTarget || currentTarget->getTargetType () != TYPE_FLAT))
		{
			return setNow (nt, plan_id);
		}
		// kill darks during nights..
		if (next_night->getValueBool () && getMasterState () == SERVERD_NIGHT && currentTarget && currentTarget->getTargetType () == TYPE_DARK && nt->getTargetType () != TYPE_DARK)
		{
			if (setNow (nt, plan_id) == 0)
			{
				next_night->setValueBool (false);
				sendValueAll (next_night);
			}
		}
		getActiveQueue ()->addTarget (nt, t_start, t_end, -1, plan_id);
		if (!currentTarget)
			return switchTarget () == 0 ? 0 : -2;
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
  	queues.push_back (ExecutorQueue (this, name, &observer, -1));
	activeQueue->addSelVal (name, (Rts2SelData *) &(queues.back()));
}

int Executor::setNow (int nextId, int plan_id)
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
		return setNow (newTarget, plan_id);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "cannot set now target with ID " << nextId << " : " << er << sendLog;
		return -2;
	}
}

int Executor::setNow (rts2db::Target * newTarget, int plan_id)
{
	if (currentTarget)
	{
		currentTarget->endObservation (-1);
		processTarget (currentTarget);
	}
	currentTarget = newTarget;
	current_plan_id->setValueInteger (plan_id);
	sendValueAll (current_plan_id);

	// at this situation, we would like to get rid of nextTarget as
	// well
	clearNextTargets ();

	clearAll ();
	postEvent (new rts2core::Event (EVENT_SET_TARGET_KILL, (void *) currentTarget));
	postEvent (new rts2core::Event (EVENT_SLEW_TO_TARGET_NOW, (void *) current_plan_id));

	infoAll ();

	struct ln_equ_posn pos;
	currentTarget->getPosition (&pos);

	logStream (MESSAGE_INFO) << "executing now target " << currentTarget->getTargetName () << "(#" << currentTarget->getTargetID () << ") at RA DEC " << LibnovaRaDec (&pos) << sendLog;
	return 0;
}

int Executor::setGrb (int grbId)
{
	rts2db::Target *grbTarget = NULL;
	int ret;

	// is during night and ready?
	if (!(getMasterState () == SERVERD_NIGHT || getMasterState () == SERVERD_DUSK || getMasterState () == SERVERD_DAWN))
	{
		logStream (MESSAGE_DEBUG) << "daylight / not on state GRB ignored" << sendLog;
		return -2;
	}
	try
	{
		if (Configuration::instance ()->grbdValidity () == 0)
		{
			logStream (MESSAGE_INFO) << "GRBs has 0 validity period, grb command ignored for GRB with target id " << grbId << sendLog;
			return 0;
		}
		grbTarget = createTarget (grbId, observer);

		if (!grbTarget)
			return -2;

		double JD = ln_get_julian_from_sys ();

		if (grbTarget->checkConstraints (JD))
		{
			logStream (MESSAGE_INFO) << "GRB does not meet constraints: violated " << grbTarget->getViolatedConstraints (JD).toString () << ", satisfied " << grbTarget->getSatisfiedConstraints (JD).toString () << sendLog;
			delete grbTarget;
			return -2;
		}

		// if we're already disabled, don't execute us
		if (grbTarget->getTargetEnabled () == false)
		{
			logStream (MESSAGE_INFO)
				<< "ignored execution request for GRB target " << grbTarget->getTargetName ()
				<< " (# " << grbTarget->getObsTargetID () << ") because this target is disabled" << sendLog;
			delete grbTarget;
			return 0;
		}
		if (!currentTarget)
		{
			return setNow (grbTarget, -1);
		}

		if (currentTarget->getTargetType () == TYPE_GRB && grbTarget->getTargetType () == TYPE_GRB)
		{
			// targets closer than 5 minutes are probably same GRBs. Then choose one with lower error box
			if (fabs (((rts2db::TargetGRB *) grbTarget)->getGrbDate () - ((rts2db::TargetGRB *) currentTarget)->getGrbDate ()) < 300)
			{
				if (((rts2db::TargetGRB *) grbTarget)->getErrorBox () > ((rts2db::TargetGRB *) currentTarget)->getErrorBox ())
				{
					logStream (MESSAGE_INFO) << "GRB targets " << grbTarget->getTargetID () << " and " << currentTarget->getTargetID () << ", errors " << ((rts2db::TargetGRB *) grbTarget)->getErrorBox () << " and " << ((rts2db::TargetGRB *) currentTarget)->getErrorBox () << " are probably same, ignoring update" << sendLog;
					delete grbTarget;
					return 0;
				}
			}
		}
		// it's not same..
		ret = grbTarget->compareWithTarget (currentTarget, grb_sep_limit->getValueDouble ());
		if (ret == 0)
		{
			return setNow (grbTarget, -1);
		}
		// if that's only few arcsec update, don't change
		ret = grbTarget->compareWithTarget (currentTarget, grb_min_sep->getValueDouble ());
		if (ret == 1)
		{
			logStream (MESSAGE_INFO) << "GRB update for target " << grbTarget->getTargetName () << " (#"
				<< grbTarget->getObsTargetID () << ") ignored, as its distance from current target "
				<< currentTarget->getTargetName () << " (#" << currentTarget->getObsTargetID ()
				<< ") is below separation limit of " << LibnovaDegDist (grb_min_sep->getValueDouble ())
				<< "." << sendLog;
			delete grbTarget;
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
		delete grbTarget;
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

	return setNow (TARGET_SHOWER, -1);
}

int Executor::stop ()
{
	flatsDone->setValueBool (false);
	sendValueAll (flatsDone);
	clearNextTargets ();
	postEvent (new rts2core::Event (EVENT_KILL_ALL));
	postEvent (new rts2core::Event (EVENT_STOP_OBSERVATION));
	updateScriptCount ();
	if (scriptCount->getValueInteger () == 0)
		switchTarget ();
	return 0;
}

void Executor::clearNextTargets ()
{
	getActiveQueue ()->setCurrentTarget (currentTarget);
  	getActiveQueue ()->clearNext ();
	sendValueAll (next_id);
	sendValueAll (next_name);
	logStream (MESSAGE_DEBUG) << "cleared list of next targets" << sendLog;
}

void Executor::doSwitch ()
{
	int ret;
	int nextId;
	// make sure queue is configured for target change
	getActiveQueue ()->setCurrentTarget (currentTarget);
	getActiveQueue ()->beforeChange (getNow ());
	// we need to change current target - usefull for planner runs
	if (currentTarget && currentTarget->isContinues () == 2 && (getActiveQueue ()->size () == 0 || getActiveQueue ()->front ().target->getTargetID () == currentTarget->getTargetID ()))
	{
		// create again our target..since conditions changed, we will get different target id
		getActiveQueue ()->addFront (createTarget (currentTarget->getTargetID (), observer));
	}
	if (autoLoop->getValueBool () == false && currentTarget && currentTarget->observationStarted ())
	{
		// don't execute already started observation, if auto loop is off
		currentTarget = NULL;
		current_plan_id->setValueInteger (-1);
	}
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
			}
			// switch auto loop back to true
			autoLoop->setValueBool (defaultAutoLoop->getValueBool ());
			sendValueAll (autoLoop);
		}
		else
		{
			currentTarget = getActiveQueue ()->front ().target;
			current_plan_id->setValueInteger (getActiveQueue ()->front ().plan_id);
		}
	}
	if (currentTarget)
	{
		// send script_ends to all devices..
		queAll (new rts2core::CommandScriptEnds (this));
		postEvent (new rts2core::Event (EVENT_SET_TARGET, (void *) currentTarget));
		postEvent (new rts2core::Event (EVENT_SLEW_TO_TARGET, (void *) current_plan_id));
		// send note to selector..
		connections_t::iterator c = getConnections ()->begin ();
		getOpenConnectionType (DEVICE_TYPE_SELECTOR, c);
		while (c != getConnections ()->end ())
		{
			(*c)->queCommand (new rts2core::CommandObservation (this, currentTarget->getTargetID (), currentTarget->getObsId ()));
			c++;
			getOpenConnectionType (DEVICE_TYPE_SELECTOR, c);
		}
	}
}

int Executor::switchTarget ()
{
	if (enabled->getValueBool () == false)
	{
		clearNextTargets ();
		logStream (MESSAGE_WARNING) << "please switch executor to enabled to allow it carrying observations" << sendLog;
		return -1;
	}

	if (((getState () & EXEC_MASK_END) == EXEC_END)
		|| (autoLoop->getValueBool () == false && getActiveQueue ()->size () == 0))
	{
		maskState (EXEC_MASK_END, EXEC_NOT_END);
		if (autoLoop->getValueBool () == true)
			postEvent (new rts2core::Event (EVENT_KILL_ALL));

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
				logStream (MESSAGE_ERROR) << "system not in ON state, and ignore_day in EXECutor is not set - not changing the target" << sendLog;
				return -1;
		}
	}
	infoAll ();
	return 0;
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
	logStream (INFO_OBSERVATION_END | MESSAGE_INFO) << current_obsid->getValueInteger () << " " << in_target->getTargetID () << sendLog;
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
	postEvent (new rts2core::Event (EVENT_SCRIPT_RUNNING_QUESTION, (void *) &scriptRunning));
	scriptCount->setValueInteger (scriptRunning);
}

int Executor::commandAuthorized (rts2core::Connection * conn)
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
		selectorNext->setValueBool (true);
		selector_next_reported = false;
		sendValueAll (selectorNext);
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		return setNow (tar_id, -1);
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
		switch (conn->getOtherType ())
		{
			case DEVICE_TYPE_SELECTOR:
				if (selectorNext->getValueBool () == false)
				{
					if (selector_next_reported == false)
					{
						logStream (MESSAGE_WARNING) << "ignoring selector next, as selector_next is set to false (for target #" << tar_id << ")" << sendLog;
						selector_next_reported = true;
					}
					return -2;
				}
				break;
		}
		selectorNext->setValueBool (true);
		selector_next_reported = false;
		sendValueAll (selectorNext);
		return setNext (tar_id);
	}
	else if (conn->isCommand ("next_plan"))
	{
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		switch (conn->getOtherType ())
		{
			case DEVICE_TYPE_SELECTOR:
				if (selectorNext->getValueBool () == false)
				{
					if (selector_next_reported == false)
					{
						logStream (MESSAGE_WARNING) << "ignoring selector next, as selector_next is set to false (for target #" << tar_id << ")" << sendLog;
						selector_next_reported = true;
					}
					return -2;
				}
				break;
		}
		selectorNext->setValueBool (true);
		selector_next_reported = false;
		sendValueAll (selectorNext);
		return setNextPlan (tar_id);
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
		selectorNext->setValueBool (false);
		selector_next_reported = false;
		sendValueAll (selectorNext);
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
	else if (conn->isCommand ("write_headers"))
	{
		char *imgn;
		if (conn->paramNextString (&imgn) || !conn->paramEnd ())
			return -1;
		rts2image::Image image;
		image.openFile (imgn, false, true);
		postEvent (new rts2core::Event (EVENT_WRITE_ONLY_IMAGE, (void *) &image));
		image.saveImage ();
		return 0;
	}
	return rts2db::DeviceDb::commandAuthorized (conn);
}


void Executor::fileModified (struct inotify_event *event)
{
	currentTarget->revalidateConstraints (event->wd);
	for (std::list <ExecutorQueue>::iterator iter = queues.begin (); iter != queues.end (); iter++)
		iter->revalidateConstraints (event->wd);
}

int main (int argc, char **argv)
{
	Executor executor (argc, argv);
	return executor.run ();
}
