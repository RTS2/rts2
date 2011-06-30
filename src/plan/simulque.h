/*
 * Simulation queue.
 * Copyright (C) 2011      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_SIMULQUEUE__
#define __RTS2_SIMULQUEUE__

#include "executorque.h"

namespace rts2plan
{
/**
 * Hold queue entries for simulation. As the code cannot remove observed
 * targets from real queues, it must create and fill queues for simulation.
 */
class SimulQueueTargets:public TargetQueue
{
	public:
		SimulQueueTargets (ExecutorQueue &eq);

	protected:
		virtual int getQueueType () { return queueType; }
		virtual bool getRemoveAfterExecution () { return removeAfterExecution; };
		virtual bool getSkipBelowHorizon () { return skipBelowHorizon; }
		virtual bool getTestConstraints () { return testConstraints; }

		virtual TargetQueue::iterator removeEntry (TargetQueue::iterator &iter, const char *reason);
	private:
		int queueType;
		bool removeAfterExecution;
		bool skipBelowHorizon;
		bool testConstraints;
};

/**
 * Simulation queue. Allows to simulate observing run from queues.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class SimulQueue:public ExecutorQueue
{
	public:
		SimulQueue (Rts2DeviceDb *master, const char *name, struct ln_lnlat_posn **_observer, Queues *_queues);
		virtual ~SimulQueue ();

		void simulate (double from, double to);

	private:
		// list of simulation input queues

		Queues *queues;
};

}

#endif // !__RTS2_SIMULQUEUE__
