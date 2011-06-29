/*
 * Executor queue.
 * Copyright (C) 2010,2011      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_EXECUTORQUEUE__
#define __RTS2_EXECUTORQUEUE__

#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"

// queue modes
#define QUEUE_FIFO                   0
#define QUEUE_CIRCULAR               1
#define QUEUE_HIGHEST                2
#define QUEUE_WESTEAST               3
// observe targets only when they pass meridian
#define QUEUE_WESTEAST_MERIDIAN      4

// timer events for queued start/end
#define EVENT_NEXT_START      RTS2_LOCAL_EVENT + 1400
#define EVENT_NEXT_END        RTS2_LOCAL_EVENT + 1401

namespace rts2plan
{

/**
 * Target queue information.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class QueuedTarget
{
	public:
		QueuedTarget (rts2db::Target * _target, double _t_start = rts2_nan ("f"), double _t_end = rts2_nan ("f"), int _plan_id = -1)
		{
			target = _target;
			t_start = _t_start;
			t_end = _t_end;
			planid = _plan_id;
		}

		~QueuedTarget () {}

		/**
		 * Return true if target observation times does not expired - e.g. t_start is nan or t_start <= now.
		 */
		bool notExpired (double now);

		rts2db::Target *target;
		double t_start;
		double t_end;
		int planid;
};

class SimulQueueTargets;

/**
 * Executor queue. Used to freely create queue inside executor
 * for queue execution. Allow users to define rules how the queue
 * should be used, provides method to support basic queue operations.
 * 
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ExecutorQueue:public std::list <QueuedTarget>
{
	public:
		ExecutorQueue (Rts2DeviceDb *master, const char *name, struct ln_lnlat_posn **_observer);
		virtual ~ExecutorQueue ();

		/**
		 * Set time for which queue will be tested.
		 *
		 * @param _now  new time value (in ctime - sec from 1/1/1970)
		 */
		void setNow (double _now) { now = _now; }

		int addFront (rts2db::Target *nt, double t_start = rts2_nan ("f"), double t_end = rts2_nan ("f"));
		int addTarget (rts2db::Target *nt, double t_start = rts2_nan ("f"), double t_end = rts2_nan ("f"), int plan_id = -1);

		double getMaximalDuration (rts2db::Target *tar);

		const ExecutorQueue::iterator findTarget (rts2db::Target *tar);

		// order by given target list
		void orderByTargetList (std::list <rts2db::Target *> tl);

		/**
		 * Runs queue filter, remove expired observations.
		 */
		void filter ();

		void sortWestEastMeridian ();

		/**
		 * Do not delete pointer to this target, as it is used somewhere else.
		 */
		void setCurrentTarget (rts2db::Target *ct) { currentTarget = ct; }
		
		/**
		 * Put next target on front of the queue.
		 */
		void beforeChange ();

		void clearNext ();

		/**
		 * Select next valid observation target. As queues might hold targets which are invalid (e.g. start date of observation
		 * is in future, object is bellow horizon,..), can return failure even if queue is not empty.
		 *
		 * @return -1 on failure, indicating that the queue does not hold any valid targets, otherwise target id of selected observation.
		 */
		int selectNextObservation (int &pid);

		/**
		 * Simulate selection of next observation from the queue. Adjust sq list if 
		 * observation is selected and will not be repeated.
		 */
		int selectNextSimulation (SimulQueueTargets &sq, double from, double to, double &e_end);

		int queueFromConn (Rts2Conn *conn, bool withTimes = false, rts2core::ConnNotify *watchConn = NULL);

		void setSkipBelowHorizon (bool skip) { skipBelowHorizon->setValueBool (skip); master->sendValueAll (skipBelowHorizon); }

		// prints queue configuration into stream
		friend std::ostream & operator << (std::ostream &os, const ExecutorQueue *eq)
		{
			os << " type " << eq->queueType->getDisplayValue () << " skip below " << eq->skipBelowHorizon->getValueBool () << " test constraints " << eq->testConstraints->getValueBool () << " remove after execution " << eq->removeAfterExecution->getValueBool () << " enabled " << eq->queueEnabled->getValueBool () << " contains";
			std::vector <int>::iterator niditer = eq->nextIds->valueBegin ();
			std::vector <double>::iterator startiter = eq->nextStartTimes->valueBegin ();
			std::vector <double>::iterator enditer = eq->nextEndTimes->valueBegin ();
			for (; niditer != eq->nextIds->valueEnd (); niditer++, startiter++, enditer++)
			{
				os << " " << *niditer << "(" << LibnovaDateDouble (*startiter) << " to " << LibnovaDateDouble (*enditer) << ")";
			}
			return os;
		}

		const rts2core::ValueBool * getEnabledValue () { return queueEnabled; }

		void revalidateConstraints (int watch_id);

		struct ln_lnlat_posn **observer;

	private:
		Rts2DeviceDb *master;

		rts2core::IntegerArray *nextIds;
		rts2core::StringArray *nextNames;
		rts2core::TimeArray *nextStartTimes;
		rts2core::TimeArray *nextEndTimes;
		rts2core::IntegerArray *nextPlanIds;

		rts2core::ValueSelection *queueType;
		rts2core::ValueBool *skipBelowHorizon;
		rts2core::ValueBool *testConstraints;
		rts2core::ValueBool *removeAfterExecution;
		rts2core::ValueBool *queueEnabled;

		double now;
		double getNow () { return now; }

		// update values from the target list
		void updateVals ();

		bool isAboveHorizon (QueuedTarget &tar, double &JD);

		// filter or skip observations bellow horizon
		// if skipBelowHorizon is set to false (default), remove observations which are currently
		// bellow horizon. If skipBelowHorizon is true, put them to back of the queue (so they will not be scheduled).
		void filterBelowHorizon ();

		// remove observations which observing time expired
		void filterExpired ();

		// return true if its't time to remove first element from the queue. This is usaully when the
		// second observation next time is before the current time
		bool frontTimeExpires ();

		// remove timers set by targets in queue
		void removeTimers ();

		// remove target with debug entry why it was removed from the queue
		ExecutorQueue::iterator removeEntry (ExecutorQueue::iterator &iter, const char *reason);

		rts2db::Target *currentTarget;
};

class Queues: public std::deque <ExecutorQueue>
{
	public:
		Queues ():std::deque <ExecutorQueue> () {}
};

}

#endif // !__RTS2_EXECUTORQUEUE__
