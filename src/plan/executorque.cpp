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
	master->createValue (nextStartTimes, (sn + "_start").c_str (), "times of element execution", false, RTS2_VALUE_WRITABLE);
	master->createValue (nextEndTimes, (sn + "_end").c_str (), "times of element execution", false, RTS2_VALUE_WRITABLE);
	master->createValue (queueType, (sn + "_queing").c_str (), "queing mode", false, RTS2_VALUE_WRITABLE);
	master->createValue (skipBellowHorizon, (sn + "_skip_bellow").c_str (), "skip targets bellow horizon (otherwise remove them)", false, RTS2_VALUE_WRITABLE);
	skipBellowHorizon->setValueBool (false);

	queueType->addSelVal ("FIFO");
	queueType->addSelVal ("CIRCULAR");
	queueType->addSelVal ("HIGHEST");
	queueType->addSelVal ("WESTEAST");
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

int ExecutorQueue::addTarget (rts2db::Target *nt, double t_start, double t_end)
{
	push_back (QueuedTarget (nt, t_start, t_end));
	updateVals ();
	return 0;
}

void ExecutorQueue::filter ()
{
	filterBellowHorizon ();
	filterExpired ();
	updateVals ();
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
	}
	filter ();
	updateVals ();
}

void ExecutorQueue::popFront ()
{
	switch (queueType->getValueInteger ())
	{
		case QUEUE_FIFO:
		case QUEUE_HIGHEST:
		case QUEUE_WESTEAST:
			if (frontTimeExpires ())
				pop_front ();
			break;
		case QUEUE_CIRCULAR:
			break;
	}
	updateVals ();
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

int ExecutorQueue::selectNextObservation ()
{
	removeTimers ();
	if (size () > 0)
	{
		struct ln_hrz_posn hrz;
		front ().target->getAltAz (&hrz, ln_get_julian_from_sys (), *observer);
		if (front ().target->isAboveHorizon (&hrz) && front ().notExpired (master->getNow ()))
		{
			return front ().target->getTargetID ();
		}
		else
		{
			double t = master->getNow ();
			// add timers..
			double t_start = front ().t_start;
			if (!isnan (t_start) && t_start > t)
			{
				master->addTimer (t - t_start, new Rts2Event (EVENT_NEXT_START));
			}
			else
			{
				double t_end = front ().t_end;
				if (!isnan (t_end) && t_end > t)
					master->addTimer (t - t_end, new Rts2Event (EVENT_NEXT_END));
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
			if (conn->paramNextDouble (&t_start) || conn->paramNextDouble (&t_end))
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
	return failed;
}

void ExecutorQueue::updateVals ()
{
	std::vector <int> _id_arr;
	std::vector <std::string> _name_arr;
	std::vector <double> _start_arr;
	std::vector <double> _end_arr;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		_id_arr.push_back (iter->target->getTargetID ());
		_name_arr.push_back (iter->target->getTargetName ());
		_start_arr.push_back (iter->t_start);
		_end_arr.push_back (iter->t_end);
	}
	nextIds->setValueArray (_id_arr);
	nextNames->setValueArray (_name_arr);
	nextStartTimes->setValueArray (_start_arr);
	nextEndTimes->setValueArray (_end_arr);

	master->sendValueAll (nextIds);
	master->sendValueAll (nextNames);
	master->sendValueAll (nextStartTimes);
	master->sendValueAll (nextEndTimes);
}

void ExecutorQueue::filterBellowHorizon ()
{
	if (!empty ())
	{
		rts2db::Target *firsttar = NULL;
		double JD = ln_get_julian_from_sys ();

		for (ExecutorQueue::iterator iter = begin (); iter != end () && iter->target != firsttar;)
		{
			struct ln_hrz_posn hrz;
			iter->target->getAltAz (&hrz, JD, *observer);
			if (iter->target->isAboveHorizon (&hrz))
			{
				iter++;
				continue;
			}

			if (skipBellowHorizon->getValueBool ())
			{
				if (firsttar == NULL)
					firsttar = iter->target;

				push_back (*iter);
				iter = erase (iter);
			}
			else
			{
				iter = erase (iter);
			}
		}
	}
}

void ExecutorQueue::filterExpired ()
{
	ExecutorQueue::iterator iter = begin ();
	if (iter == end ())
		return;
	ExecutorQueue::iterator iter2 = begin ();
	for (;iter2 != end ();)
	{
		double t_start = iter2->t_start;
		double t_end = iter2->t_end;
		if (!isnan (t_start) && t_start <= master->getNow () && !isnan (t_end) && t_end <= master->getNow ())
			iter2 = erase (iter2);
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
				do_erase = ((iter->target->observationStarted () && isnan (t_start)) || t_start <= master->getNow ());
				break;
			default:
				do_erase = (!isnan (t_start) && t_start <= master->getNow ());
				break;
		}

		if (do_erase)
		{
			iter = erase (iter);
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
	return iter->notExpired (master->getNow ());
}

void ExecutorQueue::removeTimers ()
{
	master->deleteTimers (EVENT_NEXT_START);
	master->deleteTimers (EVENT_NEXT_END);
}
