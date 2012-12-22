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

#include "rts2script/simulque.h"

using namespace rts2plan;

SimulQueueTargets::SimulQueueTargets (ExecutorQueue &eq):TargetQueue (eq.master, eq.observer)
{
  	queueType = eq.getQueueType ();
	removeAfterExecution = eq.getRemoveAfterExecution ();
	skipBelowHorizon = eq.getSkipBelowHorizon ();
	testConstraints = eq.getTestConstraints ();
	blockUntilVisible = eq.getBlockUntilVisible ();

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

SimulQueue::SimulQueue (rts2db::DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer, Queues *_queues):ExecutorQueue (_master, name, _observer, true)
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

	double e_end = NAN;

	if (t < to)
	{
	  	sq = sqs.begin ();
		bool found = false;
		double t_to = to;
		for (Queues::iterator qi = queues->begin (); qi != queues->end (); qi++)
		{
			sq->filter (t);
			struct ln_equ_posn nextp;
			int n_id = qi->selectNextSimulation (*sq, t, t_to, e_end, &currentp, &nextp);
			// remove target from simulation..
			if (n_id > 0)
			{
				// check if there is target in upper queues..
				for (Queues::iterator qi2 = queues->begin (); qi2 != qi; qi2++)
				{
					if (!isnan (qi2->front ().t_start) && qi2->front ().t_start < e_end)
					{
						e_end = qi2->front ().t_start;
						break;		
					}
						
				}
				t = e_end;
				rts2db::Target *tar = createTarget (n_id, *observer, NULL);
				addTarget (tar, from, t);
				logStream (MESSAGE_DEBUG) << "adding to simulation:" << n_id << " " << tar->getTargetName () << " from " << LibnovaDateDouble (from) << " to " << LibnovaDateDouble (t) << sendLog;
				sq->front ().target->startObservation ();
				sq->beforeChange (t);
				found = true;
				currentp.ra = nextp.ra;
				currentp.dec = nextp.dec;
				break;
			}
			// e_end holds possible start of next target..
			if (!isnan (e_end) && e_end < to)
				t_to = e_end;
			sq++;
		}
		if (!found)
		{
			// something is ready in the queue, use its start time as next time..
			if (!isnan (e_end) && e_end > t)
				t = e_end;
			else
				t += 60;
		}
		from = t;
		return 0;
	}

	for (sq = sqs.begin (); sq != sqs.end (); sq++)
		sq->clearNext ();

	updateVals ();

	return 2;
}
