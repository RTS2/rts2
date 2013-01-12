/*
 * Tasks BB server performs on backgroud.
 * Copyright (C) 2013 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_BB_TASKS__
#define __RTS2_BB_TASKS__

#include "tsqueue.h"

#include "bbdb.h"
#include "bbconn.h"

#include <pthread.h>

namespace rts2bb
{

class BB;

/**
 * Abstract class for tasks scheduled inside BB.
 */
class BBTask
{
	public:
		BBTask () {};

		virtual ~BBTask () {}

		/**
		 * Run queue tasks.
		 *
		 * @return 0 if task should not be re-run. > 0 specifies seconds after which task should be rescheduled.
		 */
		virtual int run () = 0;
};

/**
 * Task to schedule observation on a single observatory.
 */
class BBTaskSchedule:public BBTask
{
	public:
		BBTaskSchedule (ObservatorySchedule *_schedule, int _tar_id, int _observatory_id)
		{
			obs_sched = _schedule;
			tar_id = _tar_id;
			sched_process = NULL;
		}

		virtual ~BBTaskSchedule ()
		{
		}

		virtual int run ();

	private:
		ObservatorySchedule *obs_sched;
		int tar_id;
		ConnBBQueue *sched_process;
};

/**
 * Confirm observatory selection.
 */
class BBConfirmTask:public BBTask
{
	public:
		BBConfirmTask (int _schedule_id, int _observatory_id)
		{
			schedule_id = _schedule_id;
			observatory_id = _observatory_id;
		}
		virtual ~BBConfirmTask ()
		{

		}

		virtual int run ();

	private:
		int schedule_id;
		int observatory_id;
};


/**
 * Queue holding all tasks.
 */
class BBTasks:public TSQueue <BBTask *>
{
	public:
		BBTasks (BB *_server);
		~BBTasks ();

		void run ();

		void queueTask (BBTask *t);
	
	private:
		pthread_t send_thread;
		BB *server;
};

}

#endif //! __RTS2_BB_TASKS__
