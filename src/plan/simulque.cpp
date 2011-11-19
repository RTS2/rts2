/*
 * Simulation queue.
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

#include "simulque.h"

using namespace rts2plan;

SimulQueueTargets::SimulQueueTargets (ExecutorQueue &eq):TargetQueue (eq.master, eq.observer)
{
  	queueType = eq.getQueueType ();
	removeAfterExecution = eq.getRemoveAfterExecution ();
	skipBelowHorizon = eq.getSkipBelowHorizon ();
	testConstraints = eq.getTestConstraints ();

	for (ExecutorQueue::iterator qi = eq.begin (); qi != eq.end (); qi++)
		push_back ( QueuedTarget (*qi, createTarget (qi->target->getTargetID(), *observer, NULL) ) );
}

SimulQueueTargets::~SimulQueueTargets ()
{
}

void SimulQueueTargets::clearNext ()
{
	for (SimulQueueTargets::iterator iter = begin (); iter != end (); iter = erase (iter))
		delete iter->target;
}


TargetQueue::iterator SimulQueueTargets::removeEntry (TargetQueue::iterator &iter, const int reason)
{
	delete iter->target;
	return erase (iter);
}

SimulQueue::SimulQueue (Rts2DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer, Queues *_queues):ExecutorQueue (_master, name, _observer, true)
{
	queues = _queues;
}

SimulQueue::~SimulQueue ()
{

}

void SimulQueue::start (double _from, double _to)
{
  	from = _from;
	to = _to;
	t = from;

	sqs.clear ();

	// fill in simulation queues
	for (Queues::iterator qi = queues->begin (); qi != queues->end (); qi++)
		sqs.push_back (SimulQueueTargets (*qi));

	clear ();
}

double SimulQueue::step ()
{
	std::vector <SimulQueueTargets>::iterator sq;

	if (t < to)
	{
	  	sq = sqs.begin ();
		bool found = false;
		for (Queues::iterator qi = queues->begin (); qi != queues->end (); qi++)
		{
			sq->filter (t);
			int n_id = qi->selectNextSimulation (*sq, t, to, t);
			// remove target from simulation..
			if (n_id > 0)
			{
				addTarget (createTarget (n_id, *observer, NULL), from, t);
				sq->front ().target->startObservation ();
				sq->beforeChange (t);
				found = true;
				break;
			}
			sq++;
		}
		if (!found)
			t += 60;
		from = t;
		return (t - from) / (to - from);
	}

	for (sq = sqs.begin (); sq != sqs.end (); sq++)
		sq->clearNext ();

	updateVals ();

	return 2;
}
