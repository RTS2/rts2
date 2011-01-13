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

#include "executorque.h"

using namespace rts2plan;

ExecutorQueue::ExecutorQueue (Rts2DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer)
{
  	master = _master;
	std::string sn (name);
	observer = _observer;
	master->createValue (nextIds, (sn + "_ids").c_str (), "next queue IDs", false, RTS2_VALUE_WRITABLE);
	master->createValue (nextNames, (sn + "_names").c_str (), "next queue names", false);
	master->createValue (nextTimes, (sn + "_times").c_str (), "times of element execution", false, RTS2_VALUE_WRITABLE);
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
	updateVals ();
}

int ExecutorQueue::addFront (rts2db::Target *nt, double t)
{
	push_front (nt);
	targetTimes[nt] = t;
	updateVals ();
	return 0;
}

int ExecutorQueue::addTarget (rts2db::Target *nt, double t)
{
	push_back (nt);
	targetTimes[nt] = t;
	updateVals ();
	return 0;
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
				push_back (createTarget (front ()->getTargetID (), *observer));
				pop_front ();
			}
			break;
		case QUEUE_HIGHEST:
			sort (rts2db::sortByAltitude (*observer));
			break;
		case QUEUE_WESTEAST:
			sort (rts2db::sortWestEast (*observer));
			break;
	}
	filterBellowHorizon ();
	filterExpired ();
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
		if (*iter != currentTarget)
			delete *iter;
	}
	clear ();
	targetTimes.clear ();
}

void ExecutorQueue::updateVals ()
{
	std::vector <int> _id_arr;
	std::vector <std::string> _name_arr;
	std::vector <double> _times_arr;
	for (ExecutorQueue::iterator iter = begin (); iter != end (); iter++)
	{
		_id_arr.push_back ((*iter)->getTargetID ());
		_name_arr.push_back ((*iter)->getTargetName ());
		_times_arr.push_back (targetTimes[(*iter)]);
	}
	nextIds->setValueArray (_id_arr);
	nextNames->setValueArray (_name_arr);
	nextTimes->setValueArray (_times_arr);
	master->sendValueAll (nextIds);
	master->sendValueAll (nextNames);
	master->sendValueAll (nextTimes);
}

void ExecutorQueue::filterBellowHorizon ()
{
	if (!empty ())
	{
		rts2db::Target *firsttar = NULL;
		double JD = ln_get_julian_from_sys ();

		for (ExecutorQueue::iterator iter = begin (); iter != end () && *iter != firsttar;)
		{
			struct ln_hrz_posn hrz;
			(*iter)->getAltAz (&hrz, JD, *observer);
			if ((*iter)->isAboveHorizon (&hrz))
			{
				iter++;
				continue;
			}

			if (skipBellowHorizon->getValueBool ())
			{
				if (firsttar == NULL)
					firsttar = *iter;

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
	iter2++;
	while (iter2 != end ())
	{
		double t = targetTimes[(*iter2)];
		bool do_erase = false;
		switch (queueType->getValueInteger ())
		{
			case QUEUE_FIFO:
			case QUEUE_CIRCULAR:
				do_erase = (((*iter)->observationStarted () && isnan (t)) || t <= master->getNow ());
				break;
			default:
				do_erase = (!isnan (t) && t <= master->getNow ());
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
	double t = targetTimes[*iter];
	std::cout << "frontTimeExpires t: " << t << std::endl;
	if (isnan (t) || t <= master->getNow ())
		return true;
	return false;
}
