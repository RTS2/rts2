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

SimulQueueTargets::SimulQueueTargets (ExecutorQueue &eq):TargetQueue (eq.master, eq.observer, eq.obs_altitude)
{
  	queueType = eq.getQueueType ();
	removeAfterExecution = eq.getRemoveAfterExecution ();
	skipBelowHorizon = eq.getSkipBelowHorizon ();
	testConstraints = eq.getTestConstraints ();
	blockUntilVisible = eq.getBlockUntilVisible ();
	checkTargetLength = eq.getCheckTargetLength ();

	for (ExecutorQueue::iterator qi = eq.begin (); qi != eq.end (); qi++)
		push_back ( QueuedTarget (*qi, createTarget (qi->target->getTargetID(), *observer, obs_altitude) ) );
}

SimulQueueTargets::~SimulQueueTargets ()
{
}

void SimulQueueTargets::clearNext ()
{
	for (SimulQueueTargets::iterator iter = begin (); iter != end (); iter = erase (iter))
		delete iter->target;
}


TargetQueue::iterator SimulQueueTargets::removeEntry (TargetQueue::iterator &iter, __attribute__ ((unused)) const removed_t reason)
{
	delete iter->target;
	return erase (iter);
}

SimulQueue::SimulQueue (rts2db::DeviceDb *_master, const char *name, struct ln_lnlat_posn **_observer, Queues *_queues):ExecutorQueue (_master, name, _observer, -1, true)
{
	queues = _queues;
}

SimulQueue::~SimulQueue ()
{

}

void SimulQueue::start (double _from, double _to)
{
  	from = _from;
	fr = from;
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
				for (std::vector <SimulQueueTargets>::iterator sq2 = sqs.begin (); sq2 != sq; sq2++)
				{
					if (!(sq2->empty ()) && !std::isnan (sq2->front ().t_start) && sq2->front ().t_start < e_end && sq2->front ().t_start > t)
					{
						e_end = sq2->front ().t_start;
						break;		
					}
						
				}
				t = e_end;
				rts2db::Target *tar = createTarget (n_id, *observer, obs_altitude);
				addTarget (tar, fr, t, -1, -1, false, false);
				logStream (MESSAGE_DEBUG) << "adding to simulation:" << n_id << " " << tar->getTargetName () << " from " << LibnovaDateDouble (fr) << " to " << LibnovaDateDouble (t) << sendLog;
				sq->front ().target->startObservation ();
				sq->beforeChange (t);
				found = true;
				currentp.ra = nextp.ra;
				currentp.dec = nextp.dec;
				break;
			}
			// e_end holds possible start of next target..
			if (!std::isnan (e_end) && e_end < to)
				t_to = e_end;
			sq++;
		}
		if (found && !std::isnan (e_end) && e_end > t)
			t = e_end;
		else
			t += 60;

		fr = t;
		if (found)
			return (t - from) / (to - from);
		else
			return (from - t) / (to - from);
	}

	for (sq = sqs.begin (); sq != sqs.end (); sq++)
		sq->clearNext ();

	updateVals ();

	return 2;
}
