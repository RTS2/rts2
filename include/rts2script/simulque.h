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

#include "rts2script/executorque.h"

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
		~SimulQueueTargets ();

		void clearNext ();

	protected:
		virtual int getQueueType () { return queueType; }
		virtual bool getSkipBelowHorizon () { return skipBelowHorizon; }
		virtual bool getTestConstraints () { return testConstraints; }
		virtual bool getRemoveAfterExecution () { return removeAfterExecution; };
		virtual bool getBlockUntilVisible () { return blockUntilVisible; }
		virtual bool getCheckTargetLength () { return checkTargetLength; }

		virtual TargetQueue::iterator removeEntry (TargetQueue::iterator &iter, const removed_t reason);
	private:
		int queueType;
		bool removeAfterExecution;
		bool skipBelowHorizon;
		bool testConstraints;
		bool blockUntilVisible;
		bool checkTargetLength;
};

/**
 * Simulation queue. Allows to simulate observing run from queues.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class SimulQueue:public ExecutorQueue
{
	public:
		SimulQueue (rts2db::DeviceDb *master, const char *name, struct ln_lnlat_posn **_observer, Queues *_queues);
		virtual ~SimulQueue ();

		void start (double from, double to);

		/**
		 * Performs one step of the simulation.
		 *
		 * @return Progress (0-1 range) of the simulation, 2 if simulation was done. Negative values means that queue target cannot be selected, but progress is reporetd anyway
		 */
		double step ();
		
		/**
		 * Get simulation time.
		 *
		 * @return current simulation time. You can get simulation progress when calling step ().
		 * @see step()
		 */
		double getSimulationTime () { return t; } 

	private:
		// list of simulation input queues

		Queues *queues;

		std::vector <SimulQueueTargets> sqs;

		double from;
		double fr;
		double to;
		double t;

		struct ln_equ_posn currentp;
};

}

#endif // !__RTS2_SIMULQUEUE__
