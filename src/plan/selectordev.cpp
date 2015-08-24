/*
 * Selector body.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "selector.h"
#include "rts2db/constraints.h"
#include "rts2script/connselector.h"
#include "rts2script/executorque.h"
#include "rts2script/simulque.h"

#include "connnotify.h"
#include "devclient.h"
#include "event.h"
#include "command.h"
#include "rts2db/devicedb.h"
#include "rts2db/planset.h"

#define OPT_IDLE_SELECT         OPT_LOCAL + 5
#define OPT_ADD_QUEUE           OPT_LOCAL + 6
#define OPT_FILTERS             OPT_LOCAL + 7
#define OPT_FILTER_FILE         OPT_LOCAL + 8
#define OPT_FILTER_ALIAS        OPT_LOCAL + 9

namespace rts2selector
{

class Rts2DevClientTelescopeSel:public rts2core::DevClientTelescope
{
	public:
		Rts2DevClientTelescopeSel (rts2core::Connection * in_connection):rts2core::DevClientTelescope (in_connection) {}

	protected:
		virtual void moveEnd ()
		{
			if (!moveWasCorrecting)
				connection->getMaster ()->postEvent (new rts2core::Event (EVENT_IMAGE_OK));
			rts2core::DevClientTelescope::moveEnd ();
		}
};

class Rts2DevClientExecutorSel:public rts2core::DevClientExecutor
{
	public:
		Rts2DevClientExecutorSel (rts2core::Connection * in_connection):rts2core::DevClientExecutor (in_connection) {}

	protected:
		virtual void lastReadout ()
		{
			connection->getMaster ()->postEvent (new rts2core::Event (EVENT_IMAGE_OK));
			rts2core::DevClientExecutor::lastReadout ();
		}
};

class SelectorDev:public rts2db::DeviceDb
{
	public:
		SelectorDev (int argc, char **argv);
		virtual ~ SelectorDev (void);

		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);
		virtual void postEvent (rts2core::Event * event);
		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

#ifdef RTS2_HAVE_SYS_INOTIFY_H
		virtual void fileModified (struct inotify_event *event);
#endif

		int selectNext ();		 // return next observation..

		/**
		 * Update next observation.
		 *
		 * @param started    if true, queue providing last target ID will be informed that the target was observed/is being observed before it will be asked for next ID
		 */
		int updateNext (bool started = false, int tar_id = -1, int obs_id = -1);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void message (Message & msg);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

		virtual int init ();

		virtual int idle ();

		virtual void valueChanged (rts2core::Value *value);

	private:
		rts2plan::Selector * sel;

		rts2core::ValueInteger *next_id;
		rts2core::ValueInteger *next_qid;
		rts2core::ValueInteger *next_plan_id;
		rts2core::ValueBool *next_started;
		rts2core::ValueTime *selectUntil;
		rts2core::ValueTime *queueSelectUntil;
		rts2core::ValueTime *nextTime;
		rts2core::ValueBool *interrupt;

		rts2core::ValueInteger *idle_select;
		rts2core::ValueInteger *night_idle_select;

		rts2core::ValueBool *selEnabled;
		rts2core::ValueBool *queueOnly;

		rts2core::ValueDouble *flatSunMin;
		rts2core::ValueDouble *flatSunMax;

		rts2core::ValueTime *flatBeginning;
		rts2core::ValueTime *flatEnding;

		rts2core::ValueString *nightDisabledTypes;

		struct ln_lnlat_posn *observer;
		double obs_altitude;
		int last_auto_id;

		rts2core::StringArray *selQueNames;

		rts2core::ValueSelection *lastQueue;

		rts2core::ValueInteger *current_target;
		rts2core::ValueInteger *current_qid;
		rts2core::ValueInteger *current_plan_id;
		rts2core::ValueTime *current_start;
		rts2core::ValueTime *current_end;
		rts2core::ValueSelection *current_queue;
		rts2core::ValueInteger *current_obs;

		rts2core::TimeArray *free_start;
		rts2core::TimeArray *free_end;

		double last_p;

		double nightHorizon;

		rts2plan::Queues queues;

		rts2core::ValueTime *simulTime;
		rts2plan::SimulQueue *simulQueue;

		std::deque <const char *> queueNames;

		std::list <const char *> filterOptions;
		std::list <const char *> filterFileOptions;
		std::list <const char *> aliasFiles;

		rts2core::ConnNotify *notifyConn;

		// load plan to queue
		void queuePlan (rts2plan::ExecutorQueue *q, double t);
		void queuePlanId (rts2plan::ExecutorQueue *q, int plan_id);

		rts2plan::Queues::iterator findQueue (const char *name);

		/**
		 * Check if selectUntil is in future. If it is in past,
		 * update it.
		 */
		void updateSelectLength ();

		void afterQueueChange (rts2plan::ExecutorQueue *q);

		double simulStart;
		// expected simulation duration
		rts2core::ValueDouble *simulExpected;

		double from;

		bool selFailureReported;

		void updateFlats ();
};

}

using namespace rts2selector;

SelectorDev::SelectorDev (int argc, char **argv):rts2db::DeviceDb (argc, argv, DEVICE_TYPE_SELECTOR, "SEL")
{
	sel = NULL;
	observer = NULL;
	obs_altitude = NAN;

	simulQueue = NULL;

	last_auto_id = -2;

	selFailureReported = false;

	notifyConn = new rts2core::ConnNotify (this);
	addConnection (notifyConn);
	rts2db::MasterConstraints::setNotifyConnection (notifyConn);

	createValue (next_id, "next_id", "ID of next target for selection", false);
	next_id->setValueInteger (-1);

	createValue (next_qid, "next_qid", "QID of next target (only used if next target is from queue)", false);
	next_qid->setValueInteger (-1);

	createValue (next_plan_id, "next_plan_id", "plan_id of next planned target", false);
	next_plan_id->setValueInteger (-1);

	createValue (next_started, "next_started", "if true, next observation was already started - queue operation is not needed", false);
	next_started->setValueBool (false);

	createValue (selQueNames, "queue_names", "selector queue names", false);
	createValue (lastQueue, "last_queue", "queue used for last selection", false);
	lastQueue->addSelVal ("automatic");

	createValue (selectUntil, "select_until", "observations should end by this time", false, RTS2_VALUE_WRITABLE);
	selectUntil->setValueDouble (NAN);

	createValue (queueSelectUntil, "queue_select_until", "time used to select onto, including possible changes from the queues", false);
	queueSelectUntil->setValueDouble (NAN);

	createValue (nextTime, "next_time", "time when selection method was run", false);
	createValue (interrupt, "interrupt", "if next target from queue hard interrupt current observations", false, RTS2_VALUE_WRITABLE);
	interrupt->setValueBool (false);

	createValue (current_target, "current_target", "current target ID", false);
	current_target->setValueInteger (-1);
	createValue (current_qid, "current_qid", "current target queue ID", false);
	current_qid->setValueInteger (-1);
	createValue (current_plan_id, "current_plan_id", "current target plan ID", false);
	current_plan_id->setValueInteger (-1);
	createValue (current_start, "current_start", "current target start time", false);
	current_start->setValueDouble (NAN);
	createValue (current_end, "current_end", "current target end time", false);
	current_end->setValueDouble (NAN);
	createValue (current_queue, "current_queue", "current target queue", false);
	current_queue->addSelVal ("automatic");
	createValue (current_obs, "current_obs", "current observation ID", false);
	current_obs->setValueInteger (-1);

	createValue (free_start, "free_start", "start times of free intervals", false);
	createValue (free_end, "free_end", "end times of free intervals", false);

	createValue (idle_select, "idle_select", "delay in sec for selection", false, RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL);
	idle_select->setValueInteger (300);

	createValue (night_idle_select, "night_idle_select", "delay in sec for selection of next target during night", false, RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL);
	night_idle_select->setValueInteger (300);

	createValue (selEnabled, "selector_enabled", "if selector should select next targets", false, RTS2_VALUE_WRITABLE);
	selEnabled->setValueBool (true);

	createValue (queueOnly, "queue_only", "select targets only from queue", false, RTS2_VALUE_WRITABLE);
	queueOnly->setValueBool (false);

	createValue (flatSunMin, "flat_sun_min", "minimal Solar height for flat selection", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (flatSunMax, "flat_sun_max", "maximal Solar height for flat selection", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);

	createValue (flatBeginning, "flat_beginning", "beginning of the next skyflats period", false);
	createValue (flatEnding, "flat_ending", "ending of the current or next skyflats period", false);

	createValue (nightDisabledTypes, "night_disabled_types", "list of target types which will not be selected during night", false, RTS2_VALUE_WRITABLE);

	createValue (simulExpected, "simul_expected", "[s] expected simulation duration", false, RTS2_DT_TIMEINTERVAL);
	simulExpected->setValueDouble (60);

	addOption (OPT_IDLE_SELECT, "idle-select", 1, "selection timeout (reselect every I seconds)");

	addOption (OPT_FILTERS, "available-filters", 1, "available filters for given camera. Camera name is separated with space, filters with :");
	addOption (OPT_FILTER_FILE, "filter-file", 1, "available filter for camera and file separated with :");
	addOption (OPT_FILTER_ALIAS, "filter-aliases", 1, "filter aliases file");

	addOption (OPT_ADD_QUEUE, "add-queue", 1, "add queues with given names; queues will have priority in selection in order they are added");
}

SelectorDev::~SelectorDev (void)
{
	delete sel;
	delete simulQueue;
}

int SelectorDev::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_IDLE_SELECT:
			idle_select->setValueCharArr (optarg);
			night_idle_select->setValueCharArr (optarg);
			break;
		case OPT_ADD_QUEUE:
			queueNames.push_back (optarg);
			break;
		case OPT_FILTERS:
			filterOptions.push_back (optarg);
			break;
		case OPT_FILTER_FILE:
			filterFileOptions.push_back (optarg);
			break;
		case OPT_FILTER_ALIAS:
			aliasFiles.push_back (optarg);
			break;
		default:
			return rts2db::DeviceDb::processOption (in_opt);
	}
	return 0;
}

int SelectorDev::reloadConfig ()
{
	int ret;
	ret = rts2db::DeviceDb::reloadConfig ();
	if (ret)
		return ret;

	Configuration *devConfig;
	devConfig = Configuration::instance ();
	observer = devConfig->getObserver ();
	obs_altitude = devConfig->getObservatoryAltitude ();

	nightHorizon = devConfig->getDoubleDefault ("observatory", "night_horizon", -10);

	delete sel;

	sel = new rts2plan::Selector (&cameras);

	sel->setObserver (observer, obs_altitude);
	sel->init ();

	std::list <const char *>::iterator iter;

	for (iter = filterOptions.begin (); iter != filterOptions.end (); iter++)
		sel->parseFilterOption (*iter);

	for (iter = filterFileOptions.begin (); iter != filterFileOptions.end (); iter++)
		sel->parseFilterFileOption (*iter);

	for (iter = aliasFiles.begin (); iter != aliasFiles.end (); iter++)
		sel->readAliasFile (*iter);

	flatSunMin->setValueDouble (sel->getFlatSunMin ());
	flatSunMax->setValueDouble (sel->getFlatSunMax ());

	nightDisabledTypes->setValueCharArr (sel->getNightDisabledTypes ().c_str ());

	deleteTimers (EVENT_SELECT_NEXT);
	addTimer (idle_select->getValueInteger (), new rts2core::Event (EVENT_SELECT_NEXT));

	return 0;
}

int SelectorDev::init ()
{
	int ret = rts2db::DeviceDb::init ();
	if (ret)
		return ret;

	ret = notifyConn->init ();
	if (ret)
		return ret;

	int i = 0;

	setMessageMask (INFO_OBSERVATION_SLEW | INFO_OBSERVATION_INTERRUPTED | INFO_OBSERVATION_LOOP);
	
	for (std::deque <const char *>::iterator iter = queueNames.begin (); iter != queueNames.end (); iter++, i++)
	{
		queues.push_back (rts2plan::ExecutorQueue (this, *iter, &observer, obs_altitude, i));
		queues.back ().load (i);

		lastQueue->addSelVal (*iter);
		current_queue->addSelVal (*iter);
		selQueNames->addValue (std::string (*iter));
	}

	notifyConn->setDebug (true);

	// create and add simulation queue
	createValue (simulTime, "simul_time", "simulation time", false);
	simulQueue = new rts2plan::SimulQueue (this, "simul", &observer, &queues);

	lastQueue->addSelVal ("simul");
	current_queue->addSelVal ("simul");
	selQueNames->addValue ("simul");

	time_t nstart, nstop;
	time_t curr_time;

	time (&curr_time);

	getNight (curr_time, observer, nightHorizon, nstart, nstop);

	// fill from, to
	if (nstart > nstop)
		nstart = curr_time;
	free_start->addValue (nstart);
	free_end->addValue (nstop);

	updateFlats ();

	return 0;
}

int SelectorDev::idle ()
{
	if (getState () & SEL_SIMULATING)
	{
		double p = simulQueue->step ();
		if (p == 2)
		{
			maskState (SEL_SIMULATING, SEL_IDLE, "simulation finished");
			simulExpected->setValueDouble (getNow () - simulStart);
			sendValueAll (simulExpected);

			if (last_p < 0)
			{
				if (free_start->size () == 0)
				{
					free_start->addValue (from);
					sendValueAll (free_start);
				}
				free_end->addValue (simulQueue->getSimulationTime ());
				sendValueAll (free_end);
			}

			setTimeout (60 * USEC_SEC);
		}
		else
		{
			if (last_p < 0 && p > 0)
			{
				free_end->addValue (simulTime->getValueDouble ());
				sendValueAll (free_end);
			}
			else if (last_p > 0 && p < 0)
			{
				free_start->addValue (simulTime->getValueDouble ());
				sendValueAll (free_start);
			}

			simulTime->setValueDouble (simulQueue->getSimulationTime ());
			sendValueAll (simulTime);

			last_p = p;
		}
	}
	return rts2db::DeviceDb::idle ();
}

rts2core::DevClient *SelectorDev::createOtherType (rts2core::Connection * conn, int other_device_type)
{
	rts2core::DevClient *ret;
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new Rts2DevClientTelescopeSel (conn);
		case DEVICE_TYPE_EXECUTOR:
			ret = rts2db::DeviceDb::createOtherType (conn, other_device_type);
			updateNext ();
			if (selEnabled->getValueBool ())
			{
				if (next_plan_id->getValueInteger () > 0)
					conn->queCommand (new rts2core::CommandExecNextPlan (this, next_plan_id->getValueInteger ()));
				else if (next_id->getValueInteger () > 0)
					conn->queCommand (new rts2core::CommandExecNext (this, next_id->getValueInteger ()));
			}
			return ret;
		default:
			return rts2db::DeviceDb::createOtherType (conn, other_device_type);
	}
}

void SelectorDev::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_IMAGE_OK:
			updateNext ();
			break;
		case EVENT_SELECT_NEXT:
			updateNext ();
			addTimer (idle_select->getValueInteger (), event);
			return;
		case EVENT_NEXT_START:
		case EVENT_NEXT_END:
			{
				int last_id = next_id->getValueInteger ();
				updateNext ();
				if (last_id != next_id->getValueInteger ())
				{
					interrupt->setValueBool (true);
					sendValueAll (interrupt);
					logStream (MESSAGE_INFO) << "set interrupt to true during observation start" << sendLog;
				}
			}
			break;
	}
	rts2db::DeviceDb::postEvent (event);
}

int SelectorDev::selectNext ()
{
	try
	{
		double next_time = NAN;
		double next_length = NAN;
		double selectLength = selectUntil->getValueDouble ();
		bool removeObserved = true;
		if (!isnan (selectLength))
		{
			selectLength -= getNow ();
		  	next_length = selectLength;
		}

		queueSelectUntil->setValueDouble (NAN);

	 	if (getMasterState () == SERVERD_NIGHT && queues.size () > 0)
		{
			int id = -1;
			int q = 1;
			int n_pid = -1;
			int n_qid = -1;
			rts2plan::Queues::iterator iter;
			for (iter = queues.begin (); iter != queues.end (); iter++, q++)
			{
				bool hard;
				id = iter->selectNextObservation (n_pid, n_qid, hard, next_time, next_length, removeObserved);
				// don't remove target from the queue, if the queue will be interrupted
				if (!isnan (next_time))
					removeObserved = false;

				if (id >= 0)
				{
					lastQueue->setValueInteger (q);
					if (next_id->getValueInteger () != id || next_qid->getValueInteger () != n_qid)
					{
						next_started->setValueBool (false);
						sendValueAll (next_started);
					}
					next_qid->setValueInteger (n_qid);
					next_plan_id->setValueInteger (n_pid);
					interrupt->setValueBool (hard);
					sendValueAll (interrupt);
					if (hard)
						logStream (MESSAGE_INFO) << "set interrupt to true (next target has hard time start)" << sendLog;
					sendValueAll (queueSelectUntil);
					return id;
				}
				double n = getNow ();
				if (!isnan (next_time) && next_time > n && next_length > next_time - n)
				{
					next_length = next_time - n;
				}
				queueSelectUntil->setValueDouble (next_time);
			}
			sendValueAll (queueSelectUntil);
			// use selector as fall-back, if queues are empty
			lastQueue->setValueInteger (0);
		}
		// select calibration frames even if in queue mode
		if (queueOnly->getValueBool () == false || getMasterState () != SERVERD_NIGHT)
		{
			int id = sel->selectNext (getMasterState (), next_length);
			if (id != last_auto_id)
			{
				logStream (MESSAGE_INFO) << "selecting from automatic selector " << id << sendLog;
				last_auto_id = id;
				if (interrupt->getValueBool ())
				{
					interrupt->setValueBool (false);
					sendValueAll (interrupt);
					logStream (MESSAGE_INFO) << "set interrupt to false, as next target was selected from automatic selector" << sendLog;
				}
			}
			next_qid->setValueInteger (-1);
			next_plan_id->setValueInteger (-1);
			return id;
		}
		logStream (MESSAGE_WARNING) << "empty queue, target not selected" << sendLog;
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "while selecting next target:" << er << sendLog;
	}
	return -1;
}

int SelectorDev::updateNext (bool started, int tar_id, int obs_id)
{
	if (started)
	{
		// see what was selected from the queue..
		interrupt->setValueBool (false);
		sendValueAll (interrupt);
		bool qfind = false;
		if (lastQueue->getValueInteger () > 0)
		{
			rts2plan::ExecutorQueue *eq = &(queues[lastQueue->getValueInteger () - 1]);
			if (eq && eq->size () > 0 && eq->front ().target->getTargetID () == tar_id)
			{
				// log it..
				logStream (MESSAGE_INFO) << "selecting from queue " << queueNames[lastQueue->getValueInteger () - 1] << ", " << eq << sendLog;
				qfind = true;
				current_qid->setValueInteger (eq->front ().qid);
				current_plan_id->setValueInteger (eq->front ().plan_id);
				current_start->setValueDouble (eq->front ().t_start);
				current_end->setValueDouble (eq->front ().t_end);
				eq->front ().target->startObservation ();
				// update plan entry..
				if (next_started->getValueBool () == false)
				{
					next_started->setValueBool (true);
					sendValueAll (next_started);
				}
				eq->beforeChange (getNow ());
			}
			else
			{
				if (eq && eq->size () > 0)
					logStream (MESSAGE_ERROR) << "cannot find target with ID " << tar_id << " in queue " << lastQueue->getValueInteger () << ", " << eq << sendLog;
				else if (eq)
					logStream (MESSAGE_ERROR) << "cannot find target with ID " << tar_id << " in queue " << lastQueue->getValueInteger () << ", queue size is 0" << sendLog;
				else
					logStream (MESSAGE_ERROR) << "cannot find target with ID " << tar_id << " in queue " << lastQueue->getValueInteger () << sendLog;
			}
		}
		current_target->setValueInteger (tar_id);
		current_obs->setValueInteger (obs_id);
		current_queue->setValueInteger (lastQueue->getValueInteger ());
		if (qfind == false)
		{
			current_qid->setValueInteger (-1);
			current_plan_id->setValueInteger (-1);
			current_start->setValueDouble (NAN);
			current_end->setValueDouble (NAN);
			current_queue->setValueInteger (0);			
		}

		sendValueAll (current_target);
		sendValueAll (current_qid);
		sendValueAll (current_plan_id);
		sendValueAll (current_start);
		sendValueAll (current_end);
		sendValueAll (current_queue);
		sendValueAll (current_obs);
	}
	int nid = selectNext ();
	if (nid < 0)
	{
		if (selFailureReported == false)
		{
			logStream (MESSAGE_ERROR) << "selector was unable to select next target. Observation might continue only with human intervention." << sendLog;
			selFailureReported = true;
		}
	}
	else
	{
		selFailureReported = false;
	}
	next_id->setValueInteger (nid);
	sendValueAll (next_id);
	sendValueAll (next_qid);
	sendValueAll (next_plan_id);
	nextTime->setValueDouble (getNow ());
	sendValueAll (nextTime);

	sendValueAll (lastQueue);

	if (next_id->getValueInteger () > 0)
	{
		connections_t::iterator iexec = getConnections ()->begin ();  	
		getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iexec);
		if (iexec != getConnections ()->end () && selEnabled->getValueBool ())
		{
			if (interrupt->getValueBool () == true)
			{
				logStream (MESSAGE_INFO) << "interrupt is set to true, executing next target with now" << sendLog;
				(*iexec)->queCommand (new rts2core::CommandExecNow (this, next_id->getValueInteger ()));
				interrupt->setValueBool (false);
				sendValueAll (interrupt);
			}
			else
			{
				if (next_plan_id->getValueInteger () > 0)
					(*iexec)->queCommand (new rts2core::CommandExecNextPlan (this, next_plan_id->getValueInteger ()));
				else
					(*iexec)->queCommand (new rts2core::CommandExecNext (this, next_id->getValueInteger ()));
			}
		}
		return 0;
	}
	return -1;
}

int SelectorDev::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == flatSunMin)
	{
		sel->setFlatSunMin (new_value->getValueDouble ());
		updateFlats ();
		return 0;
	}
	if (old_value == flatSunMax)
	{
		sel->setFlatSunMax (new_value->getValueDouble ());
		updateFlats ();
		return 0;
	}
	if (old_value == nightDisabledTypes)
	{
		sel->setNightDisabledTypes (new_value->getValue ());
		return 0;
	}

	return rts2db::DeviceDb::setValue (old_value, new_value);
}

void SelectorDev::valueChanged (rts2core::Value *value)
{
	if ((value == selEnabled && selEnabled->getValueBool ()) || value == queueOnly)
		updateNext ();
	else
	{
		for (rts2plan::Queues::iterator qi = queues.begin (); qi != queues.end (); qi++)
		{
			if (qi->getEnabledValue () == value)
			{
				updateNext ();
				break;
			}
			qi->valueChanged (value);
		}
	}
	rts2db::DeviceDb::valueChanged (value);
}

rts2plan::Queues::iterator SelectorDev::findQueue (const char *name)
{
	rts2plan::Queues::iterator qi = queues.begin ();
	std::deque <const char *>::iterator iter;
	for (iter = queueNames.begin (); iter != queueNames.end () && qi != queues.end (); iter++, qi++)
	{
		if (strcmp (*iter, name) == 0)
			break;
	}
	return qi;
}

void SelectorDev::updateSelectLength ()
{
	if (!isnan (selectUntil->getValueDouble ()) && selectUntil->getValueDouble () > getNow ())
		return;
	rts2core::Connection *centralConn = getSingleCentralConn ();
	if (centralConn != NULL)
	{
		rts2core::Value *night_ending = centralConn->getValue ("night_ending");
		if (night_ending)
		{
			selectUntil->setValueDouble (night_ending->getValueDouble ());
			logStream (MESSAGE_INFO) << "selector assumes night will end at " << Timestamp (selectUntil->getValueDouble ()) << sendLog;
			return;
		}
	}
	logStream (MESSAGE_WARNING) << "centrald not running, setting selection length to tomorrow" << sendLog;
	selectUntil->setValueDouble (getNow () + 86400);
}

void SelectorDev::message (Message & msg)
{
	switch (msg.getID ())
	{
		case INFO_OBSERVATION_SLEW:
		case INFO_OBSERVATION_LOOP:
			updateNext (true, msg.getMessageArgInt (1), msg.getMessageArgInt (0));
			break;
		case INFO_OBSERVATION_INTERRUPTED:
			if (current_queue->getValueInteger () > 0 && current_target->getValueInteger () == msg.getMessageArgInt (1))
			{
				rts2plan::ExecutorQueue *eq = &(queues[current_queue->getValueInteger () - 1]);
				if (!(isnan (current_start->getValueDouble ()) && isnan (current_end->getValueDouble ())) && eq->size () > 0 && eq->front ().target->getTargetID () == current_target->getValueInteger ())
				{
					logStream (MESSAGE_INFO) << "not re-queueing target #" << current_target->getValueInteger () << ", as it is still in the queue" << sendLog;
					return;
				}
				rts2db::Target *tar = createTarget (current_target->getValueInteger (), observer, obs_altitude);
				if (tar == NULL)
					return;
				logStream (MESSAGE_INFO) << "adding " << tar->getTargetName () << " (#" << tar->getTargetID () << ") to the top of the queue " << queueNames[current_queue->getValueInteger () - 1] << ", as a result of the interruption" << sendLog;
				eq->addFront (tar, current_start->getValueDouble (), current_end->getValueDouble ());
			}
			break;
	}
}

int SelectorDev::commandAuthorized (rts2core::Connection * conn)
{
	char *name;
	if (conn->isCommand ("next"))
	{
		if (!conn->paramEnd ())
			return -2;
		return updateNext () == 0 ? 0 : -2;
	}
	// when observation starts
	else if (conn->isCommand ("observation"))
	{
	  	int tar_id;
		int obs_id;
		if (conn->paramNextInteger (&tar_id) || conn->paramNextInteger (&obs_id) || !conn->paramEnd ())
			return -2;
		return updateNext (true, tar_id, obs_id) == 0 ? 0 : -2;
	}
	else if (conn->isCommand ("queue") || conn->isCommand ("queue_at") || conn->isCommand ("clear") || conn->isCommand ("queue_plan") || conn->isCommand ("queue_plan_id") || conn->isCommand ("insert") || conn->isCommand ("queue_nrep") || conn->isCommand ("queue_at_nrep"))
	{
		bool withTimes = conn->isCommand ("queue_at") || conn->isCommand ("queue_at_nrep");
		bool withNrep = conn->isCommand ("queue_nrep") || conn->isCommand ("queue_at_nrep");
		int index = -1;
		if (conn->paramNextString (&name))
			return -2;
		// try to find queue with name..
		rts2plan::Queues::iterator qi = findQueue (name);
		if (qi == queues.end ())
			return -2;
		rts2plan::ExecutorQueue * q = &(*qi);
		if (conn->isCommand ("clear"))
		{
			if (!conn->paramEnd ())
				return -2;
			q->clearNext ();
			logStream (MESSAGE_DEBUG) << "cleared queue " << name << sendLog;
			return 0;
		}
		else if (conn->isCommand ("queue_plan"))
		{
			double t;
			if (conn->paramNextDouble (&t) || !conn->paramEnd ())
				return -2;
			queuePlan (q, t);
			updateNext ();
			return 0;
		}
		else if (conn->isCommand ("queue_plan_id"))
		{
			int plan_id;
			if (conn->paramNextInteger (&plan_id) || !conn->paramEnd ())
				return -2;
			try
			{
				queuePlanId (q, plan_id);
			}
			catch (rts2core::Error &er)
			{
				logStream (MESSAGE_ERROR) << er << sendLog;
				return -2;
			}
			updateNext ();
			return 0;
		}
		else if (conn->isCommand ("insert"))
		{
			if (conn->paramNextInteger (&index))
				return -2;
		}
		try
		{
			if (q->queueFromConn (conn, index, withTimes, false, NAN, withNrep))
				return -2;
			afterQueueChange (q);
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_ERROR) << er << sendLog;
			return -2;
		}
		return 0;
	}
	else if (conn->isCommand ("queue_qids"))
	{
		if (conn->paramNextString (&name))
			return -2;
		// try to find queue with name..
		rts2plan::Queues::iterator qi = findQueue (name);
		if (qi == queues.end ())
			return -2;
		rts2plan::ExecutorQueue * q = &(*qi);
		try
		{
			if (q->queueFromConnQids (conn))
				return -2;
			afterQueueChange (q);
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_ERROR) << er << sendLog;
			return -2;
		}
		return 0;
	}
	else if (conn->isCommand ("remove"))
	{
		int index;
		if (conn->paramNextString (&name) || conn->paramNextInteger (&index) || !conn->paramEnd ())
			return -2;
		// try to find queue with name..
		rts2plan::Queues::iterator qi = findQueue (name);
		if (qi == queues.end ())
			return -2;
		rts2plan::ExecutorQueue * q = &(*qi);
		try
		{
			if (q->removeIndex (index))
				return -2;
			afterQueueChange (q);
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_ERROR) << er << sendLog;
			return -2;
		}
		return 0;
	}
	else if (conn->isCommand ("now") || conn->isCommand ("now_once"))
	{
		int tar_id;
		if (conn->paramNextString (&name) || conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		rts2plan::Queues::iterator qi = findQueue (name);
		if (qi == queues.end ())
			return -2;
		if (conn->isCommand ("now_once"))
		{
			if (qi->findTarget (tar_id) != qi->end ())
			{
				logStream (MESSAGE_INFO) << "target with ID " << tar_id << " is already queued, ignoring now_once request" << sendLog;
				return -2;
			}
		}
		rts2db::Target *tar = createTarget (tar_id, observer, obs_altitude);
		if (tar == NULL)
			return -2;
		logStream (MESSAGE_INFO) << "adding " << tar->getTargetName () << " (#" << tar->getTargetID () << ") on top of the queue " << name << " as the result of " << conn->getCommand () << " command." << sendLog;
		qi->addFront (tar);
		qi->filter (getNow ());
		if (qi->front ().target == tar)
		{
			if (getMasterState () == SERVERD_NIGHT)
				updateNext ();
			interrupt->setValueBool (true);
			sendValueAll (interrupt);
			logStream (MESSAGE_INFO) << "setting interrupt to true due to now queueing" << sendLog;
		}
		else
		{
			logStream (MESSAGE_WARNING) << "target " << tar->getTargetName () << "(#" << tar_id << ") was queued with now, but most probably is not visible" << sendLog;
		}
		return 0;
	}
	else if (conn->isCommand ("simulate") || conn->isCommand ("simulate_night"))
	{
		double to;

		simulStart = getNow ();

		if (conn->isCommand ("simulate"))
		{
			if (conn->paramNextDouble (&from) || conn->paramNextDouble (&to) || !conn->paramEnd ())
				return -2;
		}
		else
		{
			// if simulate_night is used, uses full night..
			from = getSingleCentralConn ()->getValueDouble ("night_beginning");
			to = getSingleCentralConn ()->getValueDouble ("night_ending");
			if (from < simulStart)
				from = simulStart;
		}

		last_p = 0;
		free_start->clear ();
		free_end->clear ();

		sendValueAll (free_start);
		sendValueAll (free_end);

		simulTime->setValueDouble (from);
		sendValueAll (simulTime);

		simulQueue->start (from, to);
		maskState (SEL_SIMULATING, SEL_SIMULATING, "starting simulation", simulStart, simulStart + simulExpected->getValueDouble ());
		setTimeout (0);
		return 0;
	}
	else
	{
		return rts2db::DeviceDb::commandAuthorized (conn);
	}
}

void SelectorDev::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
 	// don't do anything in OFF modes
	switch (new_state & SERVERD_ONOFF_MASK)
	{
		case SERVERD_SOFT_OFF:
		case SERVERD_HARD_OFF:
			rts2db::DeviceDb::changeMasterState (old_state, new_state);
			return;
	}
	// do something only in night/dawn/dusk
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_DUSK:
		case SERVERD_DAWN:
			// low latency select to catch right moment for flats
			idle_select->setValueInteger (30);
			sendValueAll (idle_select);
			break;
		case SERVERD_NIGHT:
			idle_select->setValueInteger (night_idle_select->getValueInteger ());
			sendValueAll (idle_select);
			updateFlats ();
			break;
		default:
			rts2db::DeviceDb::changeMasterState (old_state, new_state);
			return;
	}
	updateSelectLength ();
	updateNext ();
	rts2db::DeviceDb::changeMasterState (old_state, new_state);
}

#ifdef RTS2_HAVE_SYS_INOTIFY_H
void SelectorDev::fileModified (struct inotify_event *event)
{
	sel->revalidateConstraints (event->wd);
	for (rts2plan::Queues::iterator iter = queues.begin (); iter != queues.end (); iter++)
	{
		iter->revalidateConstraints (event->wd);
		updateNext ();
	}
}
#endif

void SelectorDev::queuePlan (rts2plan::ExecutorQueue *q, double t)
{
	q->clearNext ();
	q->setSkipBelowHorizon (true);
	rts2db::PlanSet p (getNow (), getNow () + t);
	p.load ();
	for (rts2db::PlanSet::iterator iter = p.begin (); iter != p.end (); iter++)
	{
		q->addTarget (iter->getTarget (), iter->getPlanStart (), iter->getPlanEnd (), iter->getPlanId ());
		iter->clearTarget ();
	}
}

void SelectorDev::queuePlanId (rts2plan::ExecutorQueue *q, int plan_id)
{
	rts2db::Plan p (plan_id);
	p.load ();

	q->addTarget (p.getTarget (), p.getPlanStart (), p.getPlanEnd (), -1, p.getPlanId ());
	p.clearTarget ();
}

void SelectorDev::afterQueueChange (rts2plan::ExecutorQueue *q)
{
	if (getMasterState () == SERVERD_NIGHT)
	{
		q->beforeChange (getNow ());
		updateNext ();
	}
	else
	{
		time_t now;
		rts2core::Connection *centralConn = getSingleCentralConn ();
		if (centralConn == NULL)
		{
			time (&now);
		}
		else
		{
			rts2core::Value *night_beginning = centralConn->getValue ("night_beginning");
			if (night_beginning != NULL)
				now = night_beginning->getValueDouble ();
			else
				time (&now);
		}
		q->sortQueue (now);
		q->updateVals ();
	}
}

void SelectorDev::updateFlats ()
{
	time_t fstart, fend;
	time_t curr_time;

	time (&curr_time);

	getNight (curr_time, observer, (flatSunMax->getValueDouble () > flatSunMin->getValueDouble ()) ? flatSunMax->getValueDouble () : flatSunMin->getValueDouble (), fstart, fend);

	flatBeginning->setValueTime (fstart);
	flatEnding->setValueTime (fend);

	sendValueAll (flatBeginning);
	sendValueAll (flatEnding);
}

int main (int argc, char **argv)
{
	SelectorDev selector (argc, argv);
	return selector.run ();
}
