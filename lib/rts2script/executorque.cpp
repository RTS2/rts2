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

#include "rts2script/simulque.h"
#include "rts2script/executorque.h"
#include "rts2script/script.h"
#include "rts2db/constraints.h"
#include "rts2db/sqlerror.h"

using namespace rts2plan;

int qid_seq = 0;

QueuedTarget::QueuedTarget (unsigned int _queue_id, rts2db::Target * _target, double _t_start, double _t_end, int _rep_n, float _rep_separation, int _plan_id, bool _hard, bool _persistent):QueueEntry (0, _queue_id)
{
	target = _target;

	qid = nextQid ();
	t_start = _t_start;
	t_end = _t_end;
	plan_id = _plan_id;
	tar_id = target->getTargetID ();
	hard = _hard;
	rep_n = _rep_n;
	rep_separation = _rep_separation;

	unobservable_reported = false;

	create ();
}

QueuedTarget::QueuedTarget (unsigned int _queue_id, unsigned int _qid, struct ln_lnlat_posn *observer, double obs_altitude):QueueEntry (_qid, _queue_id)
{
	load ();
	target = createTarget (tar_id, observer, obs_altitude);
}

/**
 * Sorting based on altitude.
 */
class sortQuedTargetByAltitude:public rts2db::sortByAltitude
{
	public:
		sortQuedTargetByAltitude (struct ln_lnlat_posn *_observer = NULL, double _jd = NAN):rts2db::sortByAltitude (_observer, _jd) {}
		bool operator () (QueuedTarget &tar1, QueuedTarget &tar2) { return doSort (tar1.target, tar2.target); }
};

/**
 * Sort from westmost to eastmost objects
 */
class sortQuedTargetWestEast:public rts2db::sortWestEast
{
	public:
		sortQuedTargetWestEast (struct ln_lnlat_posn *_observer = NULL, double _jd = NAN):rts2db::sortWestEast (_observer, _jd) {};
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

double TargetQueue::getMaximalDuration (rts2db::Target *tar, struct ln_equ_posn *currentp, int runnum)
{
	try
	{
		return rts2script::getMaximalScriptDuration (tar, master->cameras, currentp, runnum);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
	}
	return NAN;
}

void TargetQueue::beforeChange (double now)
{
	sortQueue (now);
	switch (getQueueType ())
	{
		case QUEUE_CIRCULAR:
			// shift only if queue is not empty
			if (!empty ())
			{
				if (front ().rep_n > 0)
				{
					front ().rep_n--;
					if (!isnan (front ().rep_separation))
					{
						front ().t_start = now + front ().rep_separation;
					}
				}
				push_back (QueuedTarget (front (), createTarget (front ().target->getTargetID (), *observer, obs_altitude)));
				delete front ().target;
				pop_front ();
			}
			break;
		case QUEUE_FIFO:
		case QUEUE_HIGHEST:
		case QUEUE_WESTEAST:
		case QUEUE_WESTEAST_MERIDIAN:
		case QUEUE_OUT_OF_LIMITS:
			// if number of repetitions is set, requeue
			if (!empty () && front ().rep_n > 1)
			{
				front ().rep_n--;
				if (!isnan (front ().rep_separation))
				{
					front ().t_start = now + front ().rep_separation;
				}
				push_back (QueuedTarget (front (), createTarget (front ().target->getTargetID (), *observer, obs_altitude)));
				delete front ().target;
				pop_front ();
			}
			break;
	}
	filter (now);
}

void TargetQueue::sortQueue (double now)
{
	time_t t_now = now;
	double now_JD = ln_get_julian_from_timet (&t_now);
	switch (getQueueType ())
	{
		case QUEUE_FIFO:
		case QUEUE_CIRCULAR:
			break;
		case QUEUE_HIGHEST:
			sort (sortQuedTargetByAltitude (*observer, now_JD));
			break;
		case QUEUE_WESTEAST:
			sort (sortQuedTargetWestEast (*observer, now_JD));
			break;
		case QUEUE_WESTEAST_MERIDIAN:
			sortWestEastMeridian (now_JD);
			break;
		case QUEUE_OUT_OF_LIMITS:
			sortOutOfLimits (now_JD);
			break;	
	}
}

bool TargetQueue::filter (double now, double maxLength, bool removeObserved, double *next_time)
{
	filterExpired (now);

	// don't filter for visibility
	if (getBlockUntilVisible () == true)
		return true;
	
	bool ret = false;
	std::list <QueuedTarget> skipped;
	filterUnobservable (now, maxLength, skipped, removeObserved);
	TargetQueue::iterator it = begin ();
	// there is some observation which needs to be observed at correct time
	if (next_time && !empty () && !isnan (front().t_start))
		*next_time = front().t_start;
	if (!empty () && (isnan (front().t_start) || front().t_start <= now))
	{
		ret = true;
		it++;
	}
	insert (it, skipped.begin (), skipped.end ());
	updateVals ();
	// if front target was not skipped, it can be observed
	return ret;
}

void TargetQueue::load (int queue_id)
{
	std::list <unsigned int> qids = rts2db::queueQids (queue_id);

	for (std::list <unsigned int>::iterator iter = qids.begin (); iter != qids.end (); iter++)
		push_back (QueuedTarget (queue_id, *iter, *observer, obs_altitude));
	
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

const TargetQueue::iterator TargetQueue::findTarget (int tar_id)
{
	for (TargetQueue::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter->target->getTargetID () == tar_id)
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

void TargetQueue::sortWestEastMeridian (double jd)
{
	std::list < rts2db::Target *> preparedTargets;  
	std::list < rts2db::Target *> orderedTargets;
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
		  	// std::cout << (*iter)->getTargetName () << " " << (*iter)->getHourAngle (jd, *observer) << " " << getMaximalDuration (*iter) << " " << (*iter)->getHourAngle (jd, *observer) / 15.0 + getMaximalDuration (*iter) / 3600.0 << std::endl; 
			// skip target if it's not above horizon
			ExecutorQueue::iterator qi = findTarget (*iter);
			double tjd = jd;
			if (!isAboveHorizon (*qi, tjd))
			{
				// std::cout << "target not met constraints: " << (*iter)->getTargetName () << std::endl;
				continue;
			}

			jd = tjd;	
			if ((*iter)->getHourAngle (jd, *observer) / 15.0 + getMaximalDuration (*iter) / 3600.0 > 0)
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

void TargetQueue::sortOutOfLimits (double jd)
{
	std::list < rts2db::Target *> preparedTargets;  
	std::list < rts2db::Target *> orderedTargets;
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
		  	std::cout << "sort out of limits " << (*iter)->getTargetName () << " " << (*iter)->getHourAngle (jd, *observer) << " " << getMaximalDuration (*iter) << " " << (*iter)->getHourAngle (jd, *observer) / 15.0 + getMaximalDuration (*iter) / 3600.0 << std::endl; 
			// skip target if it's not above horizon
			ExecutorQueue::iterator qi = findTarget (*iter);
			double tjd = jd;
			if (!isAboveHorizon (*qi, tjd))
			{
				std::cout << "target not met constraints: " << (*iter)->getTargetName () << std::endl;
				continue;
			}

			jd = tjd;	
			if ((*iter)->getHourAngle (jd, *observer) / 15.0 + getMaximalDuration (*iter) / 3600.0 > 0)
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
	switch (getQueueType ())
	{
		case QUEUE_FIFO:
			for (;iter != end ();iter++)
			{
				double t_start = iter->t_start;
				double t_end = iter->t_end;
				if ((!isnan (t_start) && t_start <= now) || (!isnan (t_end) && t_end <= now))
				{
					logStream (MESSAGE_DEBUG) << "target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") has start and end times set (" << LibnovaDateDouble (t_start) << " " << LibnovaDateDouble (t_end) << ", removing previous targets." << sendLog;
					for (TargetQueue::iterator irem = begin (); irem != iter;)
						irem = removeEntry (irem, REMOVED_NEXT_NEEDED);
				}
			}
			break;
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

void TargetQueue::filterUnobservable (double now, double maxLength, std::list <QueuedTarget> &skipped, bool removeObserved)
{
	if (!empty ())
	{
		time_t n = now;
		double JD = ln_get_julian_from_timet (&n);

		for (TargetQueue::iterator iter = begin (); iter != end ();)
		{
			// isAboveHorizon changes jd parameter - we would like to keep the current time
		  	double tjd = JD;
			bool shift_circular = false;
			if (getQueueType () == QUEUE_CIRCULAR && !isnan (iter->t_start) && iter->t_start > now)
			{
				// don't do anything, just make sure we will get to horizon check
				shift_circular = true;
			}
			else if (isnan (maxLength))
			{
				if (isAboveHorizon (*iter, tjd))
				{
					iter->unobservable_reported = false;
					return;
				}
			}
			else
			{
				if (cameras == NULL)
				{
					cameras = new rts2db::CamList ();
					cameras->load ();
				}
				// calculate target script length..
				double tl = getMaximalDuration (iter->target, NULL, iter->target->observationStarted () ? 1 : 0);
				// if target will fit into available time, and target isAbove..
				if (((getRemoveAfterExecution () == false && removeObserved == false) || tl < maxLength) && isAboveHorizon (*iter, tjd))
					return;
			}	

			if (getSkipBelowHorizon () || shift_circular)
			{
				// if not in CIRCULAR, and target has time information, let's remove it from the queue
				switch (getQueueType ())
				{
					case QUEUE_CIRCULAR:
						break;
					default:
						if (!(isnan (iter->t_start) && isnan (iter->t_end)) && iter->target->observationStarted ())
						{
							logStream (MESSAGE_WARNING) << "target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") was observed, and as it has specified start or end times (" << LibnovaDateDouble (iter->t_start) << " to " << LibnovaDateDouble (iter->t_end) << "), and queue is not circular (" << getQueueType () << "), it will be removed" << sendLog;
							iter->remove ();
							iter = erase (iter);
							continue;
						}
				}
				if (iter->unobservable_reported == false)
				{
					logStream (MESSAGE_WARNING) << "Target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") is at " << LibnovaDate (tjd) << " unobservable, putting it on back of the queue" << sendLog;
					iter->unobservable_reported = true;
				}

				skipped.push_back (*iter);
				iter = erase (iter);
			}
			else
			{
				logStream (MESSAGE_WARNING) << "Removing target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") is at " << LibnovaDate (tjd) << " below horizon" << sendLog;
				iter->remove ();
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
		double njd = ln_get_julian_from_timet (&t);
		// only change time to calculate conditions when start time is in future
		if (njd > JD)
			JD = njd;
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

ExecutorQueue::ExecutorQueue (rts2db::DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer, double _altitude, int _queue_id, bool read_only):TargetQueue (_master, _observer, _altitude), queue (_queue_id)
{
	std::string sn (name);
	currentTarget = NULL;
	timerAdded = NAN;
	queue_id = _queue_id;

	int read_only_fl = read_only ? 0 : RTS2_VALUE_WRITABLE;

	master->createValue (nextIds, (sn + "_ids").c_str (), "next queue IDs", false);
	master->createValue (nextNames, (sn + "_names").c_str (), "next queue names", false);
	master->createValue (nextStartTimes, (sn + "_start").c_str (), "times of element execution", false);
	master->createValue (nextEndTimes, (sn + "_end").c_str (), "times of element execution", false);
	master->createValue (nextPlanIds, (sn + "_planid").c_str (), "plan ID's", false);
	master->createValue (nextHard, (sn + "_hard").c_str (), "hard/soft interruption", false, read_only_fl | RTS2_DT_ONOFF);
	master->createValue (queueEntry, (sn + "_qid").c_str (), "private queue ID", false);
	if (!read_only)
	{
		master->createValue (repN, (sn + "_rep_n").c_str (), "number of repeats", false, RTS2_VALUE_WRITABLE);
		master->createValue (repSeparation, (sn + "_rep_separation").c_str (), "[s] seperation of queue entry repeats", false, RTS2_VALUE_WRITABLE);
	}
	else
	{
		repN = NULL;
		repSeparation = NULL;
	}

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

	master->createValue (blockUntilVisible, (sn + "_block_until_visible").c_str (), "block queue if the top target is not visible", false, read_only_fl);
	blockUntilVisible->setValueBool (false);

	master->createValue (checkTargetLength, (sn + "_check_target_length").c_str (), "check if target can be observed in remaining time", false, read_only_fl);
	checkTargetLength->setValueBool (true);

	master->createValue (queueEnabled, (sn + "_enabled").c_str (), "enable queue for selection", false, read_only_fl);
	queueEnabled->setValueBool (true);

	master->createValue (queueWindow, (sn + "_window").c_str (), "queue selection window", false, read_only_fl | RTS2_DT_TIMEINTERVAL);

	master->createValue (sumWest, (sn + "_sum_west").c_str (), "duration of queue targets on west from meridian", false, RTS2_DT_TIMEINTERVAL);
	master->createValue (sumEast, (sn + "_sum_east").c_str (), "duration of queue targets on east from meridian", false, RTS2_DT_TIMEINTERVAL);

	queueType->addSelVal ("FIFO");
	queueType->addSelVal ("CIRCULAR");
	queueType->addSelVal ("HIGHEST");
	queueType->addSelVal ("WESTEAST");
	queueType->addSelVal ("WESTEAST_MERIDIAN");
	queueType->addSelVal ("SET_TIMES");

	if (queue_id >= 0)
	{
		try
		{
			queue.load ();

			queueType->setValueInteger (queue.queue_type);
			skipBelowHorizon->setValueBool (queue.skip_below_horizon);
			testConstraints->setValueBool (queue.test_constraints);
			removeAfterExecution->setValueBool (queue.remove_after_execution);
			blockUntilVisible->setValueBool (queue.block_until_visible);
			checkTargetLength->setValueBool (queue.check_target_length);
			queueEnabled->setValueBool (queue.queue_enabled);
			queueWindow->setValueFloat (queue.queue_window);
		}
		catch (rts2db::SqlError)
		{
			queue.queue_type = getQueueType ();
			queue.skip_below_horizon = getSkipBelowHorizon ();
			queue.test_constraints = getTestConstraints ();
			queue.remove_after_execution = getRemoveAfterExecution ();
			queue.block_until_visible = getBlockUntilVisible ();
			queue.check_target_length = getCheckTargetLength ();
			queue.queue_enabled = queueEnabled->getValueBool ();
			queue.queue_window = queueWindow->getValueFloat ();
			queue.create ();
		}
	}
}

ExecutorQueue::~ExecutorQueue ()
{
	currentTarget = NULL;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		delete iter->target;
	}
}

int ExecutorQueue::addFront (rts2db::Target *nt, double t_start, double t_end)
{
	QueuedTarget qt (queue_id, nt, t_start, t_end);
	push_front (qt);
	updateVals ();
	return qt.qid;
}

int ExecutorQueue::addTarget (rts2db::Target *nt, double t_start, double t_end, int index, int rep_n, float rep_separation, int plan_id, bool hard, bool persistent)
{
  	QueuedTarget qt (queue_id, nt, t_start, t_end, rep_n, rep_separation, plan_id, hard, persistent);
	insert (findIndex (index), qt);
	updateVals ();
	return qt.qid;
}

int ExecutorQueue::removeIndex (int index)
{
	ExecutorQueue::iterator iter = findIndex (index);
	if (iter == end ())
		return -1;
	
	if (iter->target != currentTarget)
		delete iter->target;
	else
		currentTarget = NULL;
	iter->remove ();
	erase (iter);
	updateVals ();
	return 0;
}

int ExecutorQueue::addFirst (rts2db::Target *nt, first_ordering_t fo, double n_start, double t_start, double t_end, int rep_n, float rep_separation, int plan_id, bool hard)
{
	// find entry in queue to put target
	double now = n_start;
	if (!isnan (t_start))
		now = t_start;
	// total script length
	double tl = rts2script::getMaximalScriptDuration (nt, master->cameras);
	if (tl <= 0)
		tl = 60;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		time_t tnow = now;
		double JD = ln_get_julian_from_timet (&tnow);

		double to;
		if (!isnan (iter->t_start))
			to = iter->t_start + tl;
		else
			to = now + tl;
		double satDuration = nt->getSatisfiedDuration (now, to, tl, 60);

		// first check if we should insert after the current target
		bool skip = false;
		switch (fo)
		{
			case ORDER_HA:
				skip = nt->getHourAngle (JD, *observer) < iter->target->getHourAngle (JD, *observer);
				break;
			case ORDER_SETFIRST:
				skip = satDuration > iter->target->getSatisfiedDuration (now, to, tl, 60);
				break;
			case ORDER_NONE:
				break;
		}
		if (skip == false && (isinf (satDuration) || satDuration))
		{
			insert (iter, QueuedTarget (queue_id, nt, t_start, t_end, plan_id, hard));
			updateVals ();
			return 0;
		}
		now = to;
	}
	// if everything fails, add target to the end
	addTarget (nt, t_start, t_end, rep_n, rep_separation, plan_id, hard);
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
		// remove entry from database
		iter->remove ();
	}
	clear ();
	updateVals ();
}

int ExecutorQueue::selectNextObservation (int &pid, int &qid, bool &hard, double &next_time, double next_length, bool removeObserved)
{
	if (queueEnabled->getValueBool () == false)
		return -1;
	if (size () > 0)
	{
		double now = getNow ();
		double t_start = NAN;
		if (filter (now, next_length, removeObserved, &t_start) && front ().notExpired (now))
		{
			if (isnan (next_length))
			{
				pid = front ().plan_id;
				qid = front ().qid;
				return front ().target->getTargetID ();
			}
			else
			{
				// calculate target script length..
				// the code will go there even if top long targets were not removed, making sure 
				// it will still ignore too long targets
				double tl = rts2script::getMaximalScriptDuration (front ().target, master->cameras, NULL, front().target->observationStarted () ? 1 : 0);
				if (tl < next_length || checkTargetLength->getValueBool () == false)
				{
					pid = front ().plan_id;
					qid = front ().qid;
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
			}
		}
		else
		{
			double t = now + 60;
			// add timers..
			if (!isnan (t_start) && (isnan (timerAdded) || t_start != timerAdded) && t_start > t)
			{
				// if queue has window, add timer event to the end of the window
				// before window expires, right target should be selected on its own on observation end
				timerAdded = t_start;
				if (!isnan (queueWindow->getValueFloat ()))
				{
					t += queueWindow->getValueFloat ();
				}
				master->addTimer (t_start - t, new rts2core::Event (EVENT_NEXT_START));
			}
			else
			{
				double t_end = front ().t_end;
				if (!isnan (t_end) && (isnan (timerAdded) || t_end != timerAdded) && t_end > t)
				{
					master->addTimer (t_end - t, new rts2core::Event (EVENT_NEXT_END));
					timerAdded = t_end;
				}
			}
			if (!isnan (t_start) && (isnan(next_time) || next_time > t_start))
			{
				next_time = t_start;
				if (!isnan (queueWindow->getValueFloat ()))
					next_time += queueWindow->getValueFloat ();
			}
		}
	}
	return -1;
}

int ExecutorQueue::selectNextSimulation (SimulQueueTargets &sq, double from, double to, double &e_end, struct ln_equ_posn *currentp, struct ln_equ_posn *nextp)
{
	if (queueEnabled->getValueBool () == false)
		return -1;
	if (sq.size () > 0)
	{
		time_t tn = from;
		double JD = ln_get_julian_from_timet (&tn);
		sq.front ().target->getPosition (nextp, JD);
		double md = getMaximalDuration (sq.front ().target, currentp);
		if (isAboveHorizon (sq.front(), JD) && sq.front ().notExpired (from) && from + md < to)
		{
		  	// single execution?
			if (removeAfterExecution->getValueBool ())
			{
				e_end = from + md;
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
					e_end = sq.front ().target->getSatisfiedDuration (from, to, md, 60);
					if (isnan (e_end))
						e_end = to;
				}
			}
			return sq.front ().target->getTargetID ();
		}
		// if target is not visible, put its start time as cutoff to possible next queue simulation
		e_end = sq.front ().t_start;
	}
	return -1;
}

int ExecutorQueue::queueFromConn (rts2core::Connection *conn, int index, bool withTimes, bool tryFirstPossible, double n_start, bool withNRep)
{
	double t_start = NAN;
	double t_end = NAN;
	int nrep = -1;
	double separation = NAN;
	int tar_id;
	int failed = 0;
	first_ordering_t fo = ORDER_NONE;
	if (tryFirstPossible)
	{
		int fo_i;
		conn->paramNextInteger (&fo_i);
		switch (fo_i)
		{
			case ORDER_HA:
				fo = ORDER_HA;
				break;
			case ORDER_SETFIRST:
				fo = ORDER_SETFIRST;
				break;
			case ORDER_NONE:
			default:
				fo = ORDER_NONE;
		}
	}
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
		if (withNRep)
		{
			if (conn->paramNextInteger (&nrep) || conn->paramNextDouble (&separation))
			{
				failed++;
				continue;
			}
		}
		rts2db::Target *nt = createTarget (tar_id, *observer, obs_altitude);
		if (nt == NULL)
		{
			failed++;
			continue;
		}
		if (tryFirstPossible)
			addFirst (nt, fo, n_start, t_start, t_end, nrep, separation);
		else
			addTarget (nt, t_start, t_end, index, nrep, separation);
	}
	return failed;
}

int ExecutorQueue::queueFromConnQids (rts2core::Connection *conn)
{
	int failed = 0;
	ExecutorQueue::iterator iter = begin ();
	// QIDs which should be kept; all others will be deleted
	std::vector <int> knowQuids;
	while (!conn->paramEnd ())
	{
		int _qid;
		int tar_id;
		double t_start, t_end;
		if (conn->paramNextInteger (&_qid) || conn->paramNextInteger (&tar_id) || conn->paramNextDoubleTime (&t_start) || conn->paramNextDoubleTime (&t_end))
		{
			failed++;
			continue;
		}
		rts2db::Target *nt = createTarget (tar_id, *observer, obs_altitude);
		if (nt == NULL)
		{
			failed++;
			continue;
		}
		// new entry
		if (_qid == -1)
		{
			QueuedTarget qt (queue_id, nt, t_start, t_end);
			iter = insert (iter, qt);
			knowQuids.push_back (qt.qid);
		}
		else
		{
			// if entry with given quid don't exists, fail
			ExecutorQueue::iterator it;
			for (it = begin (); it != end (); it++)
			{
				if ((int) it->qid == _qid)
					break;
			}
			// quid not found - failure
			if (it == end ())
			{
				failed++;
				continue;
			}
			// remove it, and put it after iter, the current position
			it->t_start = t_start;
			it->t_end = t_end;
			if (iter != it)
			{
				iter = insert (iter, *it);
				erase (it);
			}
			knowQuids.push_back (_qid);
		}
		iter++;
	}
	// remove quieds which were not send on queue line
	for (iter = begin (); iter != end ();)
	{
		if (iter->qid > 0)
		{
			if (std::find (knowQuids.begin (), knowQuids.end (), iter->qid) != knowQuids.end ())
			{
				iter++;
			}
			else
			{
				iter->remove ();
				iter = erase (iter);
			}
		}
		else
		{
			iter++;
		}
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
	std::vector <int> _rep_n;
	std::vector <double> _rep_separation;

	int order = 0;

	sumWest->setValueFloat (0);
	sumEast->setValueFloat (0);

	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++, order++)
	{
		_id_arr.push_back (iter->target->getTargetID ());
		_name_arr.push_back (iter->target->getTargetName ());
		_qid_arr.push_back (iter->qid);
		_start_arr.push_back (iter->t_start);
		_end_arr.push_back (iter->t_end);
		_plan_arr.push_back (iter->plan_id);
		_hard_arr.push_back (iter->hard);
		if (repN)
		{
			_rep_n.push_back (iter->rep_n);
			_rep_separation.push_back (iter->rep_separation);
		}

		iter->queue_order = order;

		iter->update ();

		if (iter->target->getHourAngle () > 0)
			sumWest->setValueFloat (sumWest->getValueFloat () + getMaximalDuration (iter->target));
		else
			sumEast->setValueFloat (sumEast->getValueFloat () + getMaximalDuration (iter->target));
	}

	if (queue_id >= 0)
	{
		queue.queue_type = getQueueType ();
		queue.skip_below_horizon = getSkipBelowHorizon ();
		queue.test_constraints = getTestConstraints ();
		queue.remove_after_execution = getRemoveAfterExecution ();
		queue.block_until_visible = getBlockUntilVisible ();
		queue.check_target_length = getCheckTargetLength ();
		queue.queue_enabled = queueEnabled->getValueBool ();
		queue.queue_window = queueWindow->getValueFloat ();

		queue.update ();
	}

	nextIds->setValueArray (_id_arr);
	nextNames->setValueArray (_name_arr);
	nextStartTimes->setValueArray (_start_arr);
	nextEndTimes->setValueArray (_end_arr);
	nextPlanIds->setValueArray (_plan_arr);
	nextHard->setValueArray (_hard_arr);
	queueEntry->setValueArray (_qid_arr);
	// if repN is defined, all others are defined
	if (repN)
	{
		repN->setValueArray (_rep_n);
		repSeparation->setValueArray (_rep_separation);
	}

	master->sendValueAll (nextIds);
	master->sendValueAll (nextNames);
	master->sendValueAll (nextStartTimes);
	master->sendValueAll (nextEndTimes);
	master->sendValueAll (nextPlanIds);
	master->sendValueAll (nextHard);
	master->sendValueAll (queueEntry);

	if (repN)
	{
		master->sendValueAll (repN);
		master->sendValueAll (repSeparation);
	}

	master->sendValueAll (sumWest);
	master->sendValueAll (sumEast);
}

void ExecutorQueue::valueChanged (rts2core::Value *value)
{
	if (value == repN)
	{
		ExecutorQueue::iterator qe_iter = begin ();
		std::vector <int>::iterator v_iter = repN->valueBegin ();
		for (; qe_iter != end () && v_iter != repN->valueEnd (); qe_iter++, v_iter++)
		{
			if (qe_iter->rep_separation != *v_iter)
			{
				qe_iter->rep_n = *v_iter;
				qe_iter->update ();
			}
		}
	}
	else if (value == repSeparation)
	{
		ExecutorQueue::iterator qe_iter = begin ();
		std::vector <double>::iterator v_iter = repSeparation->valueBegin ();
		for (; qe_iter != end () && v_iter != repSeparation->valueEnd (); qe_iter++, v_iter++)
		{
			if (qe_iter->rep_separation != *v_iter)
			{
				qe_iter->rep_separation = *v_iter;
				qe_iter->update ();
			}
		}
	}
}

const char* getTextReason (const removed_t reason)
{
	switch (reason)
	{
		case REMOVED_NEXT_NEEDED:
			return "next target must be observed";
		case REMOVED_TIMES_EXPIRED:
			return "target times expired";
		case REMOVED_STARTED:
			return "target was observed";
	}
	return "unknow reason";
}

ExecutorQueue::iterator ExecutorQueue::removeEntry (ExecutorQueue::iterator &iter, const removed_t reason)
{
	logStream (MESSAGE_WARNING) << "removing target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ", start " << LibnovaDateDouble (iter->t_start) << ", end " << LibnovaDateDouble (iter->t_end) << ") because " << getTextReason (reason) << sendLog;

	// add why,..
	if (reason < 0)
	{
		removedIds->addValue (iter->target->getTargetID ());
		removedNames->addValue (iter->target->getTargetName ());
		removedTimes->addValue (getNow ());
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
		executedTimes->addValue (getNow ());
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

	iter->remove ();

	return erase (iter);
}

ExecutorQueue::iterator ExecutorQueue::findIndex (int index)
{
	ExecutorQueue::iterator iter;
	if (index < 0)
	{
		iter = end ();
		for (int i = index; i < -1 && iter != begin (); i++)
			iter--;
	}
	else
	{
		iter = begin ();
		for (int i = index; i > 0 && iter != end (); i--)
			iter++;
	}
	return iter;
}
