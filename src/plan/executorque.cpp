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
#include "script.h"
#include "../utilsdb/constraints.h"

using namespace rts2plan;

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
	// if both targets are did not yet pass meridian, pick the highest
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

bool QueuedTarget::notExpired (double now)
{
	return (isnan (t_start) || t_start <= now) && (isnan (t_end) || t_end > now);
}

ExecutorQueue::ExecutorQueue (Rts2DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer)
{
  	master = _master;
	std::string sn (name);
	observer = _observer;
	master->createValue (nextIds, (sn + "_ids").c_str (), "next queue IDs", false, RTS2_VALUE_WRITABLE);
	master->createValue (nextNames, (sn + "_names").c_str (), "next queue names", false);
	master->createValue (nextStartTimes, (sn + "_start").c_str (), "times of element execution", false);
	master->createValue (nextEndTimes, (sn + "_end").c_str (), "times of element execution", false);
	master->createValue (nextPlanIds, (sn + "_planid").c_str (), "plan ID's", false);
	master->createValue (queueType, (sn + "_queing").c_str (), "queing mode", false, RTS2_VALUE_WRITABLE);
	master->createValue (skipBelowHorizon, (sn + "_skip_below").c_str (), "skip targets below horizon (otherwise remove them)", false, RTS2_VALUE_WRITABLE);
	skipBelowHorizon->setValueBool (true);

	master->createValue (testConstraints, (sn + "_test_constr").c_str (), "test target constraints (e.g. not only simple horizon)", false, RTS2_VALUE_WRITABLE);
	testConstraints->setValueBool (true);

	master->createValue (removeAfterExecution, (sn + "_remove_executed").c_str (), "remove observations once they are executed - run script only once", false, RTS2_VALUE_WRITABLE);
	removeAfterExecution->setValueBool (true);

	master->createValue (queueEnabled, (sn + "_enabled").c_str (), "enable queuei for selection", false, RTS2_VALUE_WRITABLE);
	queueEnabled->setValueBool (true);

	queueType->addSelVal ("FIFO");
	queueType->addSelVal ("CIRCULAR");
	queueType->addSelVal ("HIGHEST");
	queueType->addSelVal ("WESTEAST");
	queueType->addSelVal ("WESTEAST_MERIDIAN");
}

ExecutorQueue::~ExecutorQueue ()
{
	clearNext (NULL);
}

int ExecutorQueue::addFront (rts2db::Target *nt, double t_start, double t_end)
{
	push_front (QueuedTarget (nt, t_start, t_end));
	updateVals ();
	return 0;
}

int ExecutorQueue::addTarget (rts2db::Target *nt, double t_start, double t_end, int plan_id)
{
	push_back (QueuedTarget (nt, t_start, t_end, plan_id));
	updateVals ();
	return 0;
}

double ExecutorQueue::getMaximalDuration (rts2db::Target *tar)
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

const ExecutorQueue::iterator ExecutorQueue::findTarget (rts2db::Target *tar)
{
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter->target == tar)
			return iter;  
	}
	return end ();
}

void ExecutorQueue::orderByTargetList (std::list <rts2db::Target *> tl)
{
	ExecutorQueue::iterator fi = begin ();
	for (std::list <rts2db::Target *>::iterator iter = tl.begin (); iter != tl.end (); iter++)
	{
		const ExecutorQueue::iterator qi = findTarget (*iter);
		if (qi != end ())
		{
			fi = insert (fi, *qi);
			erase (qi);
			fi++;
		}
	}
}

void ExecutorQueue::filter ()
{
	filterExpired ();
	filterBelowHorizon ();
	updateVals ();
}

void ExecutorQueue::sortWestEastMeridian ()
{
	std::list < rts2db::Target *> preparedTargets;  
	std::list < rts2db::Target *> orderedTargets;
	double jd = ln_get_julian_from_sys ();
	for (ExecutorQueue::iterator ti = begin (); ti != end (); ti++)
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
		  	std::cout << (*iter)->getTargetName () << " " << (*iter)->getHourAngle (jd) << " " << getMaximalDuration (*iter) << " " << (*iter)->getHourAngle (jd) / 15.0 + getMaximalDuration (*iter) / 3600.0 << std::endl; 
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
			std::cout << "end reached, takes from beginning" << std::endl;  
			iter = preparedTargets.begin ();
		}

		jd += getMaximalDuration (*iter) / 86400.0;  
		orderedTargets.push_back ((*iter));
		std::cout << LibnovaDate (jd) << " " << (*iter)->getTargetID () << " " << (*iter)->getTargetName () << std::endl;
		preparedTargets.erase (iter);
	}
	std::cout << "ordering.." << std::endl;
	for (std::list < rts2db::Target *>::iterator i = orderedTargets.begin (); i != orderedTargets.end (); i++)
	{
		std::cout << (*i)->getTargetName () << " " << (*i)->getTargetID () << std::endl;
	}
	orderByTargetList (orderedTargets);
}

void ExecutorQueue::beforeChange ()
{
	switch (queueType->getValueInteger ())
	{
		case QUEUE_FIFO:
			break;
		case QUEUE_CIRCULAR:
			// shift only if queue is not empty and time for first observation already expires..
			if (!empty () && frontTimeExpires ())
			{
				push_back (createTarget (front ().target->getTargetID (), *observer));
				pop_front ();
			}
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
	}
	filter ();
}

void ExecutorQueue::clearNext (rts2db::Target *currentTarget)
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

int ExecutorQueue::selectNextObservation (int &pid)
{
	removeTimers ();
	if (queueEnabled->getValueBool () == false)
		return -1;
	if (size () > 0)
	{
		struct ln_hrz_posn hrz;
		front ().target->getAltAz (&hrz, ln_get_julian_from_sys (), *observer);
		if (front ().target->isAboveHorizon (&hrz) && front ().notExpired (master->getNow ()))
		{
			pid = front ().planid;
			return front ().target->getTargetID ();
		}
		else
		{
			double t = master->getNow ();
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

int ExecutorQueue::queueFromConn (Rts2Conn *conn, bool withTimes)
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
		rts2db::Target *nt = createTarget (tar_id, *observer);
		if (!nt)
		{
			failed++;
			continue;
		}
		addTarget (nt, t_start, t_end);
	}
	beforeChange ();
	return failed;
}

void ExecutorQueue::updateVals ()
{
	std::vector <int> _id_arr;
	std::vector <std::string> _name_arr;
	std::vector <double> _start_arr;
	std::vector <double> _end_arr;
	std::vector <int> _plan_arr;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		_id_arr.push_back (iter->target->getTargetID ());
		_name_arr.push_back (iter->target->getTargetName ());
		_start_arr.push_back (iter->t_start);
		_end_arr.push_back (iter->t_end);
		_plan_arr.push_back (iter->planid);
	}
	nextIds->setValueArray (_id_arr);
	nextNames->setValueArray (_name_arr);
	nextStartTimes->setValueArray (_start_arr);
	nextEndTimes->setValueArray (_end_arr);
	nextPlanIds->setValueArray (_plan_arr);

	master->sendValueAll (nextIds);
	master->sendValueAll (nextNames);
	master->sendValueAll (nextStartTimes);
	master->sendValueAll (nextEndTimes);
	master->sendValueAll (nextPlanIds);
}

bool ExecutorQueue::isAboveHorizon (QueuedTarget &qt, double &JD)
{
	struct ln_hrz_posn hrz;
	if (!isnan (qt.t_start))
	{
		time_t t = qt.t_start;
		JD = ln_get_julian_from_timet (&t);
	}
	qt.target->getAltAz (&hrz, JD, *observer);
	rts2db::ConstraintsList violated;
	return (qt.target->isAboveHorizon (&hrz) && (!testConstraints->getValueBool () || qt.target->getViolatedConstraints (JD, violated) == 0));
}

void ExecutorQueue::filterBelowHorizon ()
{
	if (!empty ())
	{
		rts2db::Target *firsttar = NULL;
		double JD = ln_get_julian_from_sys ();

		for (ExecutorQueue::iterator iter = begin (); iter != end () && iter->target != firsttar;)
		{
			// isAboveHorizon changes jd parameter - we would like to keep the current time
		  	double tjd = JD;
			if (isAboveHorizon (*iter, tjd))
				return;

			if (skipBelowHorizon->getValueBool ())
			{
				if (firsttar == NULL)
					firsttar = iter->target;

				logStream (MESSAGE_WARNING) << "Target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") is at " << LibnovaDate (tjd) << " bellow horizon" << sendLog;

				push_back (*iter);
				iter = erase (iter);
			}
			else
			{
				logStream (MESSAGE_WARNING) << "Removing target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") is at " << LibnovaDate (tjd) << " bellow horizon" << sendLog;
				iter = erase (iter);
			}
		}
	}
}

void ExecutorQueue::filterExpired ()
{
	if (empty ())
		return;
	ExecutorQueue::iterator iter;
	ExecutorQueue::iterator iter2 = begin ();
	for (;iter2 != end ();)
	{
		double t_start = iter2->t_start;
		double t_end = iter2->t_end;
		if (!isnan (t_start) && t_start <= master->getNow () && !isnan (t_end) && t_end <= master->getNow ())
			iter2 = removeEntry (iter2, "both start and end times expired");
		else if (iter2->target->observationStarted () && removeAfterExecution->getValueBool () == true)
		  	iter2 = removeEntry (iter2, "its observations started and removeAfterExecution was set to true");
		else  
			iter2++;
	}
	if (empty ())
		return;
	iter = iter2 = begin ();
	iter2++;
	while (iter2 != end ())
	{
		double t_start = iter2->t_start;
		bool do_erase = false;
		switch (queueType->getValueInteger ())
		{
			case QUEUE_FIFO:
			case QUEUE_CIRCULAR:
				do_erase = (iter->target->observationStarted () && (isnan (t_start) || t_start <= master->getNow ()));
				break;
			default:
				do_erase = (!isnan (t_start) && t_start <= master->getNow ());
				break;
		}

		if (do_erase)
		{
			iter = removeEntry (iter, "it was observed and next target start time passed");
			iter2 = iter;
			iter2++;
		}
		else
		{
			iter++;
			iter2++;
		}
	}
}

bool ExecutorQueue::frontTimeExpires ()
{
	ExecutorQueue::iterator iter = begin ();
	if (iter == end ())
		return false;
	iter++;
	if (iter == end ())
		return false;
	return !iter->notExpired (master->getNow ());
}

void ExecutorQueue::removeTimers ()
{
	master->deleteTimers (EVENT_NEXT_START);
	master->deleteTimers (EVENT_NEXT_END);
}

ExecutorQueue::iterator ExecutorQueue::removeEntry (ExecutorQueue::iterator &iter, const char *reason)
{
	logStream (MESSAGE_WARNING) << "removing target " << iter->target->getTargetName () << " (" << iter->target->getTargetID () << ") because " << reason << sendLog;
	return erase (iter);
}
