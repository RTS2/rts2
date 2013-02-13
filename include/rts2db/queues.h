/* 
 * QueueEntry class.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_QUEUES__
#define __RTS2_QUEUES__

#include <list>

namespace rts2db
{

/**
 * Queue entry class. Represent queue entry, provides methods for its manipulation in
 * the database.
 *
 * Queue content is save into queues_targets table. This way, it can be reloaded from
 * the database.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class QueueEntry
{
	public:
		/**
		 * Construct new QueueEntry, with given qid.
		 */
		QueueEntry (unsigned int qid = 0, int queue_id = 0);

		QueueEntry (const QueueEntry &qt)
		{
			qid = qt.qid;
			queue_id = qt.queue_id;
			tar_id = qt.tar_id;
			plan_id = qt.plan_id;
			t_start = qt.t_start;
			t_end = qt.t_end;

			queue_order = qt.queue_order;
		}

		virtual ~QueueEntry () {}

		/**
		 * Load schedule entry from queues_targets table.
		 *
		 * @throw SqlError
		 */
		void load ();

		/**
		 * Return next available qid.
		 *
		 * @return next available qid
		 * @throw SqlError
		 */
		int nextQid ();

		/**
		 * Create new queue entry in the database.
		 *
		 * @throw SqlError
		 */
		void create ();

		/**
		 * Update queue entry record in the database.
		 *
		 * @throw SqlError
		 */
		void update ();

		/**
		 * Remove queue entry from the database.
		 *
		 * @throw SqlError
		 */
		void remove ();

		unsigned int qid;       //* Queue ID (entry number)
		int queue_id;           //* References to queue entry is hold in. Queues with negative queue_id are virtual queues.
		unsigned int tar_id;    //* Reference target which queue entry contains
		int plan_id;		//* Can reference to entry in plan table
		double t_start;		//* Start time for queue entry, can be NAN
		double t_end;		//* End time for queue entry, can be NAN

		int queue_order;	//* Number inside queue. Used for correct order of queue entries
};

/**
 * Returns qids in the queue, ordered by queue order.
 */
std::list <unsigned int> queueQids (int queue_id);

/**
 * Class representing a queue.
 */
class Queue
{
	public:
		Queue (int queue_id);

		void load ();
		void save ();
		void create ();
		void update ();

		int queue_id;
		int queue_type;
		bool skip_below_horizon;
		bool test_constraints;
		bool remove_after_execution;
		bool block_until_visible;
		bool queue_enabled;
		float queue_window;
};

}

#endif // __RTS2_QUEUES__
