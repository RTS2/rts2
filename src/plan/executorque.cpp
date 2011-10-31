/*
 * Executor queue.
 * Copyright (C) 2010,2011     Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "executorque.h"
#include "simulque.h"
#include "script.h"
#include "../../lib/rts2db/constraints.h"

using namespace rts2plan;

int qid_seq = 0;

QueuedTarget::QueuedTarget (rts2db::Target * _target, double _t_start, double _t_end, int _plan_id, bool _hard)
{
	target = _target;
	qid = ++qid_seq;
	t_start = _t_start;
	t_end = _t_end;
	planid = _plan_id;
	hard = _hard;
}

/**
 * Sorting based on altitude.
 */
class sortQuedTargetByAltitude:public rts2db::sortByAltitude
{
	public:
		sortQuedTargetByAltitude (struct ln_lnlat_posn *_observer = NULL, double _jd = rts2_nan ("f")):rts2db::sortByAltitude (_observer, _jd) {}
		bool operator () (QueuedTarget &tar1, QueuedTarget &tar2) { return doSort (tar1.target, tar2.target); }
};

/**
 * Sort from westmost to eastmost objects
 */
class sortQuedTargetWestEast:public rts2db::sortWestEast
{
	public:
		sortQuedTargetWestEast (struct ln_lnlat_posn *_observer = NULL, double _jd = rts2_nan ("f")):rts2db::sortWestEast (_observer, _jd) {};
		bool operator () (QueuedTarget &tar1, QueuedTarget &tar2) { return doSort (tar1.target, tar2.target); }
};

/**
 * Sort by priority, and then west to east
 */
class sortByMeridianPriority:public rts2db::sortWestEast
{
	public:
		sortByMeridianPriority (struct ln_lnlat_posn *_observer, double jd):rts2db::sortWestEast (_observer, jd) {};
		bool operator () (rts2db::Target *tar1, rts2db::Target *tar2) { return doSort (tar1, tar2); }
	protected:
		bool doSort (rts2db::Target *tar1, rts2db::Target *tar2);
};

bool sortByMeridianPriority::doSort (rts2db::Target *tar1, rts2db::Target *tar2)
{
	// if both targets did not yet pass meridian, pick the highest
	if (tar1->getHourAngle (JD, observer) < 0 && tar2->getHourAngle (JD, observer) < 0)
	{
		struct ln_hrz_posn hr1, hr2;
		tar1->getAltAz (&hr1, JD, observer);
		tar2->getAltAz (&hr2, JD, observer);
		return hr1.alt > hr2.alt;
	}	
	if (tar1->getTargetPriority () == tar2->getTargetPriority ())
		return sortWestEast::doSort (tar1, tar2);
	else
		return tar1->getTargetPriority () > tar2->getTargetPriority ();
}

/**
 * Sort targets by time they set. First will be targets that go out of limits, last targets that are
 * above limits for the longest period of time (or are below limit now)
 */
class sortByOutOfLimits:public rts2db::sortWestEast
{
	public:
		sortByOutOfLimits (struct ln_lnlat_posn *_observer, double jd):rts2db::sortWestEast (_observer, jd) {};
		bool operator () (rts2db::Target *tar1, rts2db::Target *tar2) { return doSort (tar1, tar2); }
	protected:
		bool doSort (rts2db::Target *tar1, rts2db::Target *tar2);
};

bool sortByOutOfLimits::doSort (rts2db::Target *tar1, rts2db::Target *tar2)
{
	time_t t_from;
	ln_get_timet_from_julian (JD, &t_from);
	double from = t_from;

	double v1 = tar1->getSatisfiedDuration (from, from + 86400, 0, 60);
	double v2 = tar2->getSatisfiedDuration (from, from + 86400, 0, 60);
	std::cout << "sorting " << tar1->getTargetName () << " " << v1 << " 2: " << tar2->getTargetName () << " " << v2 << std::endl;
	if ((isnan (v1) && isnan (v2)) || (isinf (v1) && isinf (v2)))
	{
		// if both are not visible, order west-east..
		return sortWestEast::doSort (tar1, tar2);
	}
	// only one is not visible..
	else if (isnan (v1) || isinf (v2))
	{
		return false;
	}
	else if (isnan (v2) || isinf (v1))
	{
		return true;
	}
	return v1 < v2;
}

bool QueuedTarget::notExpired (double now)
{
	return (isnan (t_start) || t_start <= now) && (isnan (t_end) || t_end > now);
}

double TargetQueue::getMaximalDuration (rts2db::Target *tar)
{
	double md = 0;
	for (Rts2CamList::iterator cam = master->cameras.begin (); cam != master->cameras.end (); cam++)
	{
		try
		{
			std::string script_buf;
			rts2script::Script script;
			tar->getScript (cam->c_str(), script_buf);
			script.setTarget (cam->c_str (), tar);
			double d = script.getExpectedDuration ();
			if (d > md)
				md = d;  
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_ERROR) << "cannot parsing script for camera " << *cam << ": " << er << sendLog;
		}
	}
	return md;
}

void TargetQueue::beforeChange (double now)
{
	sortQueue ();
	switch (getQueueType ())
	{
		case QUEUE_FIFO:
			break;
		case QUEUE_CIRCULAR:
			// shift only if queue is not empty and time for first observation already expires or its start and end time is not specified..
			if (!empty () && (frontTimeExpires (now) || (isnan (begin ()->t_start) && isnan (begin ()->t_end))))
			{
				push_back (createTarget (front ().target->getTargetID (), *observer, front ().target->getWatchConnection ()));
				delete front ().target;
				pop_front ();
			}
			break;
		case QUEUE_HIGHEST:
		case QUEUE_WESTEAST:
		case QUEUE_WESTEAST_MERIDIAN:
		case QUEUE_OUT_OF_LIMITS:
			break;
	}
	filter (now);
}

void TargetQueue::sortQueue ()
{
	switch (getQueueType ())
	{
		case QUEUE_FIFO:
		case QUEUE_CIRCULAR:
			break;
		case QUEUE_HIGHEST:
			sort (sortQuedTargetByAltitude (*observer));
			break;
		case QUEUE_WESTEAST:
			sort (sortQuedTargetWestEast (*observer));
			break;
		case QUEUE_WESTEAST_MERIDIAN:
			sortWestEastMeridian ();
			break;
		case QUEUE_OUT_OF_LIMITS:
			sortOutOfLimits ();
			break;	
	}
}

void TargetQueue::filter (double now)
{
	filterExpired (now);
	filterBelowHorizon (now);
	updateVals ();
}

const TargetQueue::iterator TargetQueue::findTarget (rts2db::Target *tar)
{
	for (TargetQueue::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter->target == tar)
			return iter;  
	}
	return end ();
}

void TargetQueue::orderByTargetList (std::list <rts2db::Target *> tl)
{
	TargetQueue::iterator fi = begin ();
	for (std::list <rts2db::Target *>::iterator iter = tl.begin (); iter != tl.end (); iter++)
	{
		const TargetQueue::iterator qi = findTarget (*iter);
		if (qi != end ())
		{
			fi = insert (fi, *qi);
			erase (qi);
			fi++;
		}
	}
}

void TargetQueue::sortWestEastMeridian ()
{
	std::list < rts2db::Target *> preparedTargets;  
	std::list < rts2db::Target *> orderedTargets;
	double jd = ln_get_julian_from_sys ();
	for (TargetQueue::iterator ti = begin (); ti != end (); ti++)
	{
		preparedTargets.push_back (ti->target);
	}
	while (!preparedTargets.empty ())
	{
		// find maximal priority in targets, and order by HA..
		preparedTargets.sort (sortByMeridianPriority (*observer, jd));
		// now find the first target which is on west (for HA + its duration).
		std::list < rts2db::Target *>::iterator iter;

		for (iter = preparedTargets.begin (); iter != preparedTargets.end (); iter++)
		{
		  	// std::cout << (*iter)->getTargetName () << " " << (*iter)->getHourAngle (jd) << " " << getMaximalDuration (*iter) << " " << (*iter)->getHourAngle (jd) / 15.0 + getMaximalDuration (*iter) / 3600.0 << std::endl; 
			// skip target if it's not above horizon
			ExecutorQueue::iterator qi = findTarget (*iter);
			double tjd = jd;
			if (!isAboveHorizon (*qi, tjd))
			{
				// std::cout << "target not met constraints: " << (*iter)->getTargetName () << std::endl;
				continue;
			}

			jd = tjd;	
			if ((*iter)->getHourAngle (jd) / 15.0 + getMaximalDuration (*iter) / 3600.0 > 0)
				break;  
		}
		// If such target does not exists, select front..
		if (iter == preparedTargets.end ())
		{
			// std::cout << "end reached, takes from beginning" << std::endl;  
			iter = preparedTargets.begin ();
		}

		jd += getMaximalDuration (*iter) / 86400.0;  
		orderedTargets.push_back ((*iter));
		std::cout << LibnovaDate (jd) << " " << (*iter)->getTargetID () << " " << (*iter)->getTargetName () << std::endl;
		preparedTargets.erase (iter);
	}
	// std::cout << "ordering.." << std::endl;
	/**for (std::list < rts2db::Target *>::iterator i = orderedTargets.begin (); i != orderedTargets.end (); i++)
	{
		std::cout << (*i)->getTargetName () << " " << (*i)->getTargetID () << std::endl;
	}*/
	orderByTargetList (orderedTargets);
}

void TargetQueue::sortOutOfLimits ()
{
	std::list < rts2db::Target *> preparedTargets;  
	std::list < rts2db::Target *> orderedTargets;
	double jd = ln_get_julian_from_sys ();
	for (TargetQueue::iterator ti = begin (); ti != end (); ti++)
	{
		preparedTargets.push_back (ti->target);
	}
	while (!preparedTargets.empty ())
	{
		std::cout << "sorting by out of limits" << std::endl;
		// find maximal priority in targets, and order by HA..
		preparedTargets.sort (sortByOutOfLimits (*observer, jd));
		std::cout << "sorting finished" << std::endl;
		// now find the first target which is on west (for HA + its duration).
		std::list < rts2db::Target *>::iterator iter;

		for (iter = preparedTargets.begin (); iter != preparedTargets.end (); iter++)
		{
		  	std::cout << "sort out of limits " << (*iter)->getTargetName () << " " << (*iter)->getHourAngle (jd) << " " << getMaximalDuration (*iter) << " " << (*iter)->getHourAngle (jd) / 15.0 + getMaximalDuration (*iter) / 3600.0 << std::endl; 
			// skip target if it's not above horizon
			ExecutorQueue::iterator qi = findTarget (*iter);
			double tjd = jd;
			if (!isAboveHorizon (*qi, tjd))
			{
				std::cout << "target not met constraints: " << (*iter)->getTargetName () << std::endl;
				continue;
			}

			jd = tjd;	
			if ((*iter)->getHourAngle (jd) / 15.0 + getMaximalDuration (*iter) / 3600.0 > 0)
				break;  
		}
		// If such target does not exists, select front..
		if (iter == preparedTargets.end ())
		{
			// std::cout << "end reached, takes from beginning" << std::endl;  
			iter = preparedTargets.begin ();
		}

		jd += getMaximalDuration (*iter) / 86400.0;  
		orderedTargets.push_back ((*iter));
		std::cout << LibnovaDate (jd) << " " << (*iter)->getTargetID () << " " << (*iter)->getTargetName () << std::endl;
		preparedTargets.erase (iter);
	}
	std::cout << "sort out of limits - ordering.." << std::endl;
	for (std::list < rts2db::Target *>::iterator i = orderedTargets.begin (); i != orderedTargets.end (); i++)
	{
		std::cout << (*i)->getTargetName () << " " << (*i)->getTargetID () << std::endl;
	}
	orderByTargetList (orderedTargets);
}

void TargetQueue::filterExpired (double now)
{
	if (empty ())
		return;
	TargetQueue::iterator iter = begin ();
	// first: in FIFO, remove any requests which are before request with start or end time in the past 
	if (getQueueType () == QUEUE_FIFO)
	{
		for (;iter != end ();iter++)
		{
			double t_start = iter->t_start;
			double t_end = iter->t_end;
			if ((!isnan (t_start) && t_start <= now) || (!isnan (t_end) && t_end <= now))
			{
				for (TargetQueue::iterator irem = begin (); irem != iter;)
					irem = removeEntry (irem, REMOVED_NEXT_NEEDED);
			}
		}
	}
	// second: check if requests must be removed, either because their end time is in past
	// or they were observed and should be observed only once
	iter = begin ();
	for (;iter != end ();)
	{
		double t_end = iter->t_end;
		if (!isnan (t_end) && t_end <= now)
			iter = removeEntry (iter, REMOVED_TIMES_EXPIRED);
		else if (iter->target->observationStarted () && getRemoveAfterExecution () == true)
		  	iter = removeEntry (iter, REMOVED_STARTED);
		else  
			iter++;
	}
}

void TargetQueue::filterBelowHorizon (double now)
{
	if (!empty ())
	{
		rts2db::Target *firsttar = NULL;
		time_t n = now;
		double JD = ln_get_julian_from_timet (&n);

		for (ExecutorQueue::iterator iter = begin (); iter != end () && iter->target != firsttar;)
		{
			// isAboveHorizon changes jd parameter - we would like to keep the current time
		  	double tjd = JD;
			if (isAboveHorizon (*iter, tjd))
				return;

			if (getSkipBelowHorizon ())
			{
				if (firsttar == NULL)
					firsttar = iter->target;

				logStream (MESSAGE_WARNING) << "Target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") is at " << LibnovaDate (tjd) << " below horizon" << sendLog;

				push_back (*iter);
				iter = erase (iter);
			}
			else
			{
				logStream (MESSAGE_WARNING) << "Removing target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") is at " << LibnovaDate (tjd) << " below horizon" << sendLog;
				iter = erase (iter);
			}
		}
	}
}

bool TargetQueue::isAboveHorizon (QueuedTarget &qt, double &JD)
{
	struct ln_hrz_posn hrz;
	if (!isnan (qt.t_start))
	{
		time_t t = qt.t_start;
		JD = ln_get_julian_from_timet (&t);
	}
	qt.target->getAltAz (&hrz, JD, *observer);
	rts2db::ConstraintsList violated;
	return (qt.target->isAboveHorizon (&hrz) && (!getTestConstraints () || qt.target->getViolatedConstraints (JD, violated) == 0));
}

bool TargetQueue::frontTimeExpires (double now)
{
	TargetQueue::iterator iter = begin ();
	if (iter == end ())
		return false;
	iter++;
	if (iter == end ())
		return false;
	return !iter->notExpired (now);
}

ExecutorQueue::ExecutorQueue (Rts2DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer, bool read_only):TargetQueue (_master, _observer)
{
	std::string sn (name);
	currentTarget = NULL;

	int read_only_fl = read_only ? 0 : RTS2_VALUE_WRITABLE;

	master->createValue (nextIds, (sn + "_ids").c_str (), "next queue IDs", false);
	master->createValue (nextNames, (sn + "_names").c_str (), "next queue names", false);
	master->createValue (nextStartTimes, (sn + "_start").c_str (), "times of element execution", false);
	master->createValue (nextEndTimes, (sn + "_end").c_str (), "times of element execution", false);
	master->createValue (nextPlanIds, (sn + "_planid").c_str (), "plan ID's", false);
	master->createValue (nextHard, (sn + "_hard").c_str (), "hard/soft interruption", false, read_only_fl | RTS2_DT_ONOFF);
	master->createValue (queueEntry, (sn + "_qid").c_str (), "private queue entry", false);

	master->createValue (removedIds, (sn + "_removed_ids").c_str (), "removed observation IDS", false);
	master->createValue (removedNames, (sn + "_removed_names").c_str (), "names of removed IDS", false);
	master->createValue (removedTimes, (sn + "_removed_times").c_str (), "times when target was removed", false);
	master->createValue (removedWhy, (sn + "_removed_why").c_str (), "why target was removed", false);
	master->createValue (removedQueueEntry, (sn + "_removed_qid").c_str (), "queue entry of removed target", false);

	master->createValue (executedIds, (sn + "_executed_ids").c_str (), "ID of executed targets", false);
	master->createValue (executedNames, (sn + "_executed_names").c_str (), "executed targets names", false);
	master->createValue (executedTimes, (sn + "_executed_times").c_str (), "time when target was executed", false);
	master->createValue (executedQueueEntry, (sn + "_executed_qid").c_str (), "queue entry of executed target", false);

	master->createValue (queueType, (sn + "_queing").c_str (), "queing mode", false, read_only_fl);
	master->createValue (skipBelowHorizon, (sn + "_skip_below").c_str (), "skip targets below horizon (otherwise remove them)", false, read_only_fl);
	skipBelowHorizon->setValueBool (true);

	master->createValue (testConstraints, (sn + "_test_constr").c_str (), "test target constraints (e.g. not only simple horizon)", false, read_only_fl);
	testConstraints->setValueBool (true);

	master->createValue (removeAfterExecution, (sn + "_remove_executed").c_str (), "remove observations once they are executed - run script only once", false, read_only_fl);
	removeAfterExecution->setValueBool (true);

	master->createValue (queueEnabled, (sn + "_enabled").c_str (), "enable queuei for selection", false, read_only_fl);
	queueEnabled->setValueBool (true);

	queueType->addSelVal ("FIFO");
	queueType->addSelVal ("CIRCULAR");
	queueType->addSelVal ("HIGHEST");
	queueType->addSelVal ("WESTEAST");
	queueType->addSelVal ("WESTEAST_MERIDIAN");
	queueType->addSelVal ("SET_TIMES");
}

ExecutorQueue::~ExecutorQueue ()
{
	clearNext ();
}

int ExecutorQueue::addFront (rts2db::Target *nt, double t_start, double t_end)
{
	push_front (QueuedTarget (nt, t_start, t_end));
	updateVals ();
	return 0;
}

int ExecutorQueue::addTarget (rts2db::Target *nt, double t_start, double t_end, int plan_id, bool hard)
{
	push_back (QueuedTarget (nt, t_start, t_end, plan_id, hard));
	updateVals ();
	return 0;
}

void ExecutorQueue::clearNext ()
{
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		// do not delete current target
		if (iter->target == currentTarget)
			iter->target = NULL;
		else
			delete iter->target;
	}
	clear ();
	updateVals ();
}

int ExecutorQueue::selectNextObservation (int &pid, bool &hard)
{
	removeTimers ();
	if (queueEnabled->getValueBool () == false)
		return -1;
	if (size () > 0)
	{
		struct ln_hrz_posn hrz;
		double now = master->getNow ();
		front ().target->getAltAz (&hrz, ln_get_julian_from_sys (), *observer);
		if (front ().target->isAboveHorizon (&hrz) && front ().notExpired (now))
		{
			pid = front ().planid;
			if (isnan (front().t_start))
			{
				hard = false;
			}
			else
			{
				// setting hard to false signalized the program asked for execution, so next call will not issue hard interruption targets
				hard = front ().hard;
				front ().hard = false;
			}
			return front ().target->getTargetID ();
		}
		else
		{
			double t = now;
			// add timers..
			double t_start = front ().t_start;
			if (!isnan (t_start) && t_start > t)
			{
				master->addTimer (t_start - t, new Rts2Event (EVENT_NEXT_START));
			}
			else
			{
				double t_end = front ().t_end;
				if (!isnan (t_end) && t_end > t)
					master->addTimer (t_end - t, new Rts2Event (EVENT_NEXT_END));
			}

		}
	}
	return -1;
}

int ExecutorQueue::selectNextSimulation (SimulQueueTargets &sq, double from, double to, double &e_end)
{
	if (queueEnabled->getValueBool () == false)
		return -1;
	if (sq.size () > 0)
	{
		struct ln_hrz_posn hrz;
		time_t tn = from;
		sq.front ().target->getAltAz (&hrz, ln_get_julian_from_timet (&tn), *observer);
		if (sq.front ().target->isAboveHorizon (&hrz) && sq.front ().notExpired (from))
		{
		  	// single execution?
			if (removeAfterExecution->getValueBool ())
			{
				e_end = from + getMaximalDuration (sq.front ().target);
			}
			// otherwise, put end to either time_end, 
			else
			{
				if (!isnan (sq.front ().t_end))
				{
				  	e_end = sq.front ().t_end;
				}
				// or to time when target will become unacessible
				else
				{
					e_end = sq.front ().target->getSatisfiedDuration (from, to, getMaximalDuration (sq.front ().target), 60);
					if (isnan (e_end))
						e_end = to;
				}
			}
			return sq.front ().target->getTargetID ();
		}
	}
	return -1;
}

int ExecutorQueue::queueFromConn (Rts2Conn *conn, bool withTimes, rts2core::ConnNotify *watchConn)
{
	double t_start = rts2_nan ("f");
	double t_end = rts2_nan ("f");
	int tar_id;
	int failed = 0;
	while (!conn->paramEnd ())
	{
		if (conn->paramNextInteger (&tar_id))
		{
			failed++;
			continue;
		}
		if (withTimes)
		{
			if (conn->paramNextDoubleTime (&t_start) || conn->paramNextDoubleTime (&t_end))
			{
				failed++;
				continue;
			}
		}
		rts2db::Target *nt = createTarget (tar_id, *observer, watchConn);
		if (!nt)
		{
			failed++;
			continue;
		}
		addTarget (nt, t_start, t_end);
	}
	return failed;
}

void ExecutorQueue::revalidateConstraints (int watch_id)
{
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
		iter->target->revalidateConstraints (watch_id);
}

void ExecutorQueue::updateVals ()
{
	std::vector <int> _id_arr;
	std::vector <std::string> _name_arr;
	std::vector <int> _qid_arr;
	std::vector <double> _start_arr;
	std::vector <double> _end_arr;
	std::vector <int> _plan_arr;
	std::vector <bool> _hard_arr;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		_id_arr.push_back (iter->target->getTargetID ());
		_name_arr.push_back (iter->target->getTargetName ());
		_qid_arr.push_back (iter->qid);
		_start_arr.push_back (iter->t_start);
		_end_arr.push_back (iter->t_end);
		_plan_arr.push_back (iter->planid);
		_hard_arr.push_back (iter->hard);
	}
	nextIds->setValueArray (_id_arr);
	nextNames->setValueArray (_name_arr);
	nextStartTimes->setValueArray (_start_arr);
	nextEndTimes->setValueArray (_end_arr);
	nextPlanIds->setValueArray (_plan_arr);
	nextHard->setValueArray (_hard_arr);
	queueEntry->setValueArray (_qid_arr);

	master->sendValueAll (nextIds);
	master->sendValueAll (nextNames);
	master->sendValueAll (nextStartTimes);
	master->sendValueAll (nextEndTimes);
	master->sendValueAll (nextPlanIds);
	master->sendValueAll (nextHard);
	master->sendValueAll (queueEntry);
}

void ExecutorQueue::removeTimers ()
{
	master->deleteTimers (EVENT_NEXT_START);
	master->deleteTimers (EVENT_NEXT_END);
}

ExecutorQueue::iterator ExecutorQueue::removeEntry (ExecutorQueue::iterator &iter, const int reason)
{
	logStream (MESSAGE_WARNING) << "removing target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") because " << reason << sendLog;

	// add why,..
	if (reason < 0)
	{
		removedIds->addValue (iter->target->getTargetID ());
		removedNames->addValue (iter->target->getTargetName ());
		removedTimes->addValue (master->getNow ());
		removedWhy->addValue (reason);
		removedQueueEntry->addValue (iter->qid);

		master->sendValueAll (removedIds);
		master->sendValueAll (removedNames);
		master->sendValueAll (removedTimes);
		master->sendValueAll (removedWhy);
		master->sendValueAll (removedQueueEntry);
	}
	else
	{
		executedIds->addValue (iter->target->getTargetID ());
		executedNames->addValue (iter->target->getTargetName ());
		executedTimes->addValue (master->getNow ());
		executedQueueEntry->addValue (iter->qid);

		master->sendValueAll (executedIds);
		master->sendValueAll (executedNames);
		master->sendValueAll (executedTimes);
		master->sendValueAll (executedQueueEntry);
	}

	if (iter->target != currentTarget)
		delete iter->target;
	else
		currentTarget = NULL;
	return erase (iter);
}
