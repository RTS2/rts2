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

#include "bb.h"
#include "bbtasks.h"
#include "app.h"

using namespace rts2bb;

int BBTaskSchedule::run ()
{
	switch (state)
	{
		case BB_SCHEDULE_CREATED:
			state = BB_SCHEDULE_REQUESTED;
			return 1;
		case BB_SCHEDULE_REQUESTED:
			return 1;
		default:
			logStream (MESSAGE_WARNING) << "unknow BBTaskSchedule state: " << state << sendLog;
			return 0;
	}
}

BBTasks::BBTasks (BB *_server):TSQueue <BBTask *> ()
{
	send_thread = 0;
	server = _server;
}

BBTasks::~BBTasks ()
{
	while (!empty ())
	{
		BBTask *task = pop ();
		delete task;
	}
}

void BBTasks::run ()
{
	BBTask *t = pop ();
	int ret = t->run ();
	if (ret)
	{
		server->addTimer (ret, new rts2core::Event (EVENT_TASK_SCHEDULE, (void *) t));
	}
	else
	{
		delete t;
	}
}

void *processTasks (void *arg)
{
	while (true)
	{
		((BBTasks *) arg)->run ();
	}
	return NULL;
}

void BBTasks::queueTask (BBTask *t)
{
	if (send_thread == 0)
	{
		pthread_create (&send_thread, NULL, processTasks, (void *) this);
	}
	push (t);
}
