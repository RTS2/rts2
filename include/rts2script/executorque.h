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

#include "rts2db/devicedb.h"
#include "rts2db/queues.h"
#include "rts2db/target.h"

// queue modes
#define QUEUE_FIFO                   0
#define QUEUE_CIRCULAR               1
#define QUEUE_HIGHEST                2
#define QUEUE_WESTEAST               3
// observe targets only when they pass meridian
#define QUEUE_WESTEAST_MERIDIAN      4
// order targets by time remaining till they become unobservable
#define QUEUE_OUT_OF_LIMITS          5

// timer events for queued start/end
#define EVENT_NEXT_START      RTS2_LOCAL_EVENT + 1400
#define EVENT_NEXT_END        RTS2_LOCAL_EVENT + 1401

namespace rts2plan
{

typedef enum { REMOVED_TIMES_EXPIRED = -1, REMOVED_STARTED = 1, REMOVED_NEXT_NEEDED = 2} removed_t;

/**
 * Target queue information.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class QueuedTarget:public rts2db::QueueEntry
{
	public:
		QueuedTarget (unsigned int queue_id, rts2db::Target * _target, double _t_start = NAN, double _t_end = NAN, int _rep_n = -1, float _rep_separation = NAN, int _plan_id = -1, bool _hard = false, bool persistent = true);

		QueuedTarget (unsigned int queue_id, unsigned int qid, struct ln_lnlat_posn *observer, double obs_altitude);

		/**
		 * Copy constructor. Used in simulation queue to create copy of QueuedTarget.
		 */
		QueuedTarget (const QueuedTarget &qt):rts2db::QueueEntry (qt)
		{
			target = qt.target;
			unobservable_reported = qt.unobservable_reported;
		}

		QueuedTarget (const QueuedTarget &qt, rts2db::Target *_target):rts2db::QueueEntry (qt)
		{
			target = _target;
			unobservable_reported = false;
		}

		~QueuedTarget () {}

		/**
		 * Return true if target observation times does not expired - e.g. t_start is nan or t_start <= now.
		 */
		bool notExpired (double now);

		rts2db::Target *target;

		bool hard;

		bool unobservable_reported;
};

/**
 * Queue of QueuedTarget entries. Abstarct class, provides generic method to 
 * filter already observed/expired targets,..
 *
 * Parent of ExecutorQueue and SimulQueueTargets.
 *
 * @see ExecutorQueue
 * @see SimulQueueTargets
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class TargetQueue:public std::list <QueuedTarget>
{
	public:  
		TargetQueue (rts2db::DeviceDb *_master, struct ln_lnlat_posn **_observer, double _altitude):std::list <QueuedTarget> ()
		{
			observer = _observer;
			obs_altitude = _altitude;
			master = _master;
			cameras = NULL;
		}

		~TargetQueue ()
		{
			delete cameras;
		}

		/**
		 * Find target represented by given class.
		 */
		const TargetQueue::iterator findTarget (rts2db::Target *tar);

		/**
		 * Find the first target in the queue by by target ID.
		 *
		 * @param tar_id  ID of target searched in the queue
		 *
		 * @return iterator pointing to the first target with the given
		 * targer ID in the queue, or end() if target cannot be found.
		 */
		const TargetQueue::iterator findTarget (int tar_id);

		// order by given target list
		void orderByTargetList (std::list <rts2db::Target *> tl);

		double getMaximalDuration (rts2db::Target *tar, struct ln_equ_posn *currentp = NULL, int runnum = 0);

		/**
		 * Put next target on front of the queue.
		 */
		void beforeChange (double now);

		/**
		 * Sort targets by current queue ordering. Assumes that observations will
		 * start at now time.
		 *
		 * @param now  time (seconds from 1/1/1970) when observations should start
		 */
		void sortQueue (double now);

		/**
		 * Runs queue filter, remove expired observations.
		 *
		 * @param maxLength   maximal allowed length of observation
		 *
		 * @return true if front queue target can be observed, false otherwise
		 */
		bool filter (double now, double maxLength = NAN, bool removeObserved = true, double *next_time = NULL);

		/**
		 * Update values from the target list. Must be called after queue content changed.
		 */
		virtual void updateVals () {}

		/**
		 * Load target queue from database.
		 *
		 * @param queue_id     ID of queue (its number).
		 */
		void load (int queue_id);

	protected:
		rts2db::DeviceDb *master;
		struct ln_lnlat_posn **observer;
		double obs_altitude;

		virtual int getQueueType () = 0;
		virtual const bool getSkipBelowHorizon () = 0;
		virtual const bool getTestConstraints () = 0;
		virtual const bool getRemoveAfterExecution () = 0;

		/**
		 * If true, queue will not be reordered and will wait until
		 * target becomes visible.
		 */
		virtual const bool getBlockUntilVisible () = 0;

		/**
		 * Sort targets by west-east priority on west, by altitude on
		 * east.
		 *
		 * @param now start time (in JD)
		 */
		void sortWestEastMeridian (double jd);

		/**
		 * Sort targets by out-of-limits criteria. First are targets which
		 * sets below limit as first.
		 *
		 * @param now start time (in JD)
		 */
		void sortOutOfLimits (double jd);

		/**
		 * Remove observation request which expired. Expired request are:
		 *  - in FIFO mode, all request before 
		 *     - request with set start_time or end_time, which expired (is in past)
		 *  - any request with end time in past
		 *  - if remove_after_execution is true, request with started observations
		 */
		void filterExpired (double now);

		/**
		 * Remove target from queue.
		 */
		virtual TargetQueue::iterator removeEntry (TargetQueue::iterator &iter, const removed_t reason) = 0;

		bool isAboveHorizon (QueuedTarget &tar, double &JD);

		// return true if its't time to remove first element from the queue. This is usaully when the
		// second observation next time is before the current time
		bool frontTimeExpires (double now);

	private:
		rts2db::CamList *cameras;

		/*
		 * Filter or skip observations which are not observable at the moment.
		 * if skipBelowHorizon is set to false (default), remove observations which are currently
		 * below horizon. If skipBelowHorizon is true, put them to back of the queue (so they will not be scheduled).
		 */
		void filterUnobservable (double now, double maxLength, std::list <QueuedTarget> &skipped, bool removeObserved = true);
};

class SimulQueueTargets;

enum first_ordering_t
{ ORDER_NONE, ORDER_HA, ORDER_SETFIRST };

/**
 * Executor queue. Used to freely create queue inside executor
 * for queue execution. Allow users to define rules how the queue
 * should be used, provides method to support basic queue operations.
 * 
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ExecutorQueue:public TargetQueue
{
	public:
		/**
		 * If read-only is set, queue cannot be changed.
		 */
		ExecutorQueue (rts2db::DeviceDb *master, const char *name, struct ln_lnlat_posn **_observer, double _altitude, int queue_id, bool read_only = false);
		virtual ~ExecutorQueue ();

		/**
		 * Add new queue entry on front of the queue.
		 *
		 * @return Added queue ID.
		 */
		int addFront (rts2db::Target *nt, double t_start = NAN, double t_end = NAN);

		/**
		 * Add new queue entry at the given position.
		 *
		 * @return Inserted queue ID.
		 */
		int addTarget (rts2db::Target *nt, double t_start = NAN, double t_end = NAN, int index = -1, int rep_n = -1, float rep_separation = NAN, int plan_id = -1, bool hard = false, bool persistent = true);

		/**
		 * Remove entry with given index from the queue.
		 */
		int removeIndex (int index);

		/**
		 * Add target to the first possible position.
		 */
		int addFirst (rts2db::Target *nt, first_ordering_t fo, double n_start, double t_start = NAN, double t_end = NAN, int rep_n = -1, float rep_separation = NAN, int plan_id = -1, bool hard = false);

		/**
		 * Do not delete pointer to this target, as it is used somewhere else.
		 */
		void setCurrentTarget (rts2db::Target *ct) { currentTarget = ct; }
		
		void clearNext ();

		/**
		 * Select next valid observation target. As queues might hold targets which are invalid (e.g. start date of observation
		 * is in future, object is below horizon,..), can return failure even if queue is not empty.
		 *
		 * @param next_time returns start time of the first observation in the queue, if it is in future.
		 * 
		 * @return -1 on failure, indicating that the queue does not hold any valid targets, otherwise target id of selected observation.
		 */
		int selectNextObservation (int &pid, int &qid, bool &hard, double &next_time, double next_length, bool removeObserved = true);

		/**
		 * Simulate selection of next observation from the queue. Adjust sq list if 
		 * observation is selected and will not be repeated.
		 */
		int selectNextSimulation (SimulQueueTargets &sq, double from, double to, double &e_end, struct ln_equ_posn *currentp, struct ln_equ_posn *nextp);

		/**
		 *
		 * @param tryFirstPossible     try to set observation on the first possible place
		 * @param n_start              start of the night
		 * @param withNRep             accept number of repetitions and separations as extra arguments
		 */
		int queueFromConn (rts2core::Connection *conn, int index, bool withTimes, bool tryFirstPossible, double n_start, bool withNRep);

		/**
		 * Updates queue. Each queue entry record consists of four parameters:
		 *   -# queue ID
		 *   -# target ID
		 *   -# start time
		 *   -# end time
		 *
		 * If queud ID is 0, new entry is created. Negative GUIds are
		 * for removal of given QUID from the queue.
		 */
		int queueFromConnQids (rts2core::Connection *conn);

		void setSkipBelowHorizon (bool skip) { skipBelowHorizon->setValueBool (skip); master->sendValueAll (skipBelowHorizon); }

		// prints queue configuration into stream
		friend std::ostream & operator << (std::ostream &os, const ExecutorQueue *eq)
		{
			os << "type " << eq->queueType->getDisplayValue () << " skip below " << eq->skipBelowHorizon->getValueBool () << " test constraints " << eq->testConstraints->getValueBool () << " remove after execution " << eq->removeAfterExecution->getValueBool () << " block until visible " << eq->blockUntilVisible->getValueBool () << " check target lenght " << eq->checkTargetLength->getValueBool () << " enabled " << eq->queueEnabled->getValueBool ();
			if (!isnan (eq->queueWindow->getValueFloat ()))
				os << " with window " << eq->queueWindow->getValueFloat () << "s";
			os << " contains";
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

		/**
		 * Update values from the target list. Must be called after queue content changed.
		 */
		virtual void updateVals ();

		/**
		 * Propagates change of an array value (hard, rep_n or rep_separation) to underlying
		 * QueueEntry.
		 */
		void valueChanged (rts2core::Value *value);

	protected:
		virtual int getQueueType () { return queueType->getValueInteger (); }
		virtual const bool getSkipBelowHorizon () { return skipBelowHorizon->getValueBool (); }
		virtual const bool getTestConstraints () { return testConstraints->getValueBool (); }
		virtual const bool getRemoveAfterExecution () { return removeAfterExecution->getValueBool (); }
		virtual const bool getBlockUntilVisible () { return blockUntilVisible->getValueBool (); }
		virtual const bool getCheckTargetLength () { return checkTargetLength->getValueBool (); }

	private:
		rts2core::IntegerArray *nextIds;
		rts2core::StringArray *nextNames;
		rts2core::TimeArray *nextStartTimes;
		rts2core::TimeArray *nextEndTimes;
		rts2core::IntegerArray *nextPlanIds;
		rts2core::BoolArray *nextHard;
		rts2core::IntegerArray *queueEntry;
		rts2core::IntegerArray *repN;
		rts2core::DoubleArray *repSeparation;

		rts2core::IntegerArray *removedIds;
		rts2core::StringArray *removedNames;
		rts2core::TimeArray *removedTimes;
		rts2core::IntegerArray *removedWhy;
		rts2core::IntegerArray *removedQueueEntry;

		rts2core::IntegerArray *executedIds;
		rts2core::StringArray *executedNames;
		rts2core::TimeArray *executedTimes;
		rts2core::IntegerArray *executedQueueEntry;

		rts2core::ValueSelection *queueType;
		rts2core::ValueBool *skipBelowHorizon;
		rts2core::ValueBool *testConstraints;
		rts2core::ValueBool *removeAfterExecution;
		rts2core::ValueBool *blockUntilVisible;
		rts2core::ValueBool *checkTargetLength;
		rts2core::ValueBool *queueEnabled;
		rts2core::ValueFloat *queueWindow;

		rts2core::ValueFloat *sumWest;
		rts2core::ValueFloat *sumEast;

		double timerAdded;
		int queue_id;

		// remove target with debug entry why it was removed from the queue
		ExecutorQueue::iterator removeEntry (ExecutorQueue::iterator &iter, const removed_t reason);

		// find target with given index
		ExecutorQueue::iterator findIndex (int index);

		rts2db::Target *currentTarget;
		rts2db::Queue queue;

		// to allow SimulQueueTargets access to protected methods
		friend class SimulQueueTargets;
};

class Queues: public std::deque <ExecutorQueue>
{
	public:
		Queues ():std::deque <ExecutorQueue> () {}
};

}

#endif // !__RTS2_EXECUTORQUEUE__
