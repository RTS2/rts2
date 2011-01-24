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

#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"

// queue modes
#define QUEUE_FIFO          0
#define QUEUE_CIRCULAR      1
#define QUEUE_HIGHEST       2
#define QUEUE_WESTEAST      3

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
		QueuedTarget (rts2db::Target * _target, double _t_start = rts2_nan ("f"), double _t_end = rts2_nan ("f"))
		{
			target = _target;
			t_start = _t_start;
			t_end = _t_end;
		}

		~QueuedTarget () {}

		/**
		 * Return true if target observation times does not expired - e.g. t_start is nan or t_start <= now.
		 */
		bool notExpired (double now);

		rts2db::Target *target;
		double t_start;
		double t_end;
};

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

		int addFront (rts2db::Target *nt, double t_start = rts2_nan ("f"), double t_end = rts2_nan ("f"));
		int addTarget (rts2db::Target *nt, double t_start = rts2_nan ("f"), double t_end = rts2_nan ("f"));

		/**
		 * Runs queue filter, remove expired observations.
		 */
		void filter ();
		
		/**
		 * Put next target on front of the queue.
		 */
		void beforeChange ();

		void popFront ();

		void clearNext (rts2db::Target *currentTarget);

		/**
		 * Select next valid observation target. As queues might hold targets which are invalid (e.g. start date of observation
		 * is in future, object is bellow horizon,..), can return failure even if queue is not empty.
		 *
		 * @return -1 on failure, indicating that the queue does not hold any valid targets, otherwise target id of selected observation.
		 */
		int selectNextObservation ();

		int queueFromConn (Rts2Conn *conn, bool withTimes = false);

	private:
		Rts2DeviceDb *master;

		rts2core::IntegerArray *nextIds;
		rts2core::StringArray *nextNames;
		rts2core::TimeArray *nextStartTimes;
		rts2core::TimeArray *nextEndTimes;

		Rts2ValueSelection *queueType;
		Rts2ValueBool *skipBellowHorizon;

		struct ln_lnlat_posn **observer;

		// update values from the target list
		void updateVals ();

		// filter or skip observations bellow horizon
		// if skipBellowHorizon is set to false (default), remove observations which are currently
		// bellow horizon. If skipBellowHorizon is true, put them to back of the queue (so they will not be scheduled).
		void filterBellowHorizon ();

		// remove observations which observing time expired
		void filterExpired ();

		// return true if its't time to remove first element from the queue. This is usaully when the
		// second observation next time is before the current time
		bool frontTimeExpires ();

		// remove timers set by targets in queue
		void removeTimers ();
};

}
