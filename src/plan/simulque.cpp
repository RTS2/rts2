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

SimulQueueTargets::SimulQueueTargets (ExecutorQueue &eq):TargetQueue (eq.observer)
{
  	queueType = eq.getQueueType ();
	removeAfterExecution = eq.getRemoveAfterExecution ();
	skipBelowHorizon = eq.getSkipBelowHorizon ();
	testConstraints = eq.getTestConstraints ();

	for (ExecutorQueue::iterator qi = eq.begin (); qi != eq.end (); qi++)
		push_back ( QueuedTarget (*qi) );
}

TargetQueue::iterator SimulQueueTargets::removeEntry (TargetQueue::iterator &iter, const char *reason)
{
	return erase (iter);
}

SimulQueue::SimulQueue (Rts2DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer, Queues *_queues):ExecutorQueue (_master, name, _observer)
{
	queues = _queues;
}

SimulQueue::~SimulQueue ()
{
}

void SimulQueue::simulate (double from, double to)
{
  	double t = from;
	// fill in simulation queue structure..
	std::vector <SimulQueueTargets> sqs;
	Queues::iterator qi;

	for (qi = queues->begin (); qi != queues->end (); qi++)
		sqs.push_back (SimulQueueTargets (*qi));

	clear ();

	while (t < to)
	{
	  	std::vector <SimulQueueTargets>::iterator sq = sqs.begin ();
		bool found = false;
		for (qi = queues->begin (); qi != queues->end (); qi++)
		{
			int n_id = qi->selectNextSimulation (*sq, t, to, t);
			// remove target from simulation..
			if (n_id > 0)
			{
				addTarget (createTarget (n_id, *observer, NULL), from, t);
				sq->pop_front ();
				found = true;
				break;
			}
			sq++;
		}
		if (!found)
			t += 60;
		from = t;	
	}
	updateVals ();
}
