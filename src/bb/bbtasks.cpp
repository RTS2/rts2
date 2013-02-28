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
#include "bbdb.h"
#include "bbtasks.h"
#include "app.h"

#include "rts2db/sqlerror.h"

using namespace rts2bb;

static void auth (SoupSession *session, SoupMessage *msg, SoupAuth *_auth, gboolean retrying, gpointer data)
{
	if (retrying)
		return;
	((Observatory *) data)->auth (_auth);
}

JsonParser *BBTask::jsonRequest (int observatory_id, std::string path)
{
	Observatory obs (observatory_id);
	obs.load ();

	std::ostringstream request;
	request << obs.getURL () << path;

	g_type_init ();

	SoupMessage *msg = soup_message_new (SOUP_METHOD_GET, request.str ().c_str ());

	SoupSession *session = soup_session_sync_new_with_options (
		SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
		SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
		SOUP_SESSION_USER_AGENT, "rts2 bb",
		NULL);

	g_signal_connect (session, "authenticate", G_CALLBACK (auth), &obs);

	soup_session_send_message (session, msg);

	if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
	{
		logStream (MESSAGE_ERROR) << "error calling " << path << ": " << msg->status_code << " : " << msg->reason_phrase << sendLog;
		return NULL;
	}

	JsonParser *result = json_parser_new ();

	GError *error = NULL;

	json_parser_load_from_data (result, msg->response_body->data, strlen (msg->response_body->data), &error);
	if (error)
	{
		logStream (MESSAGE_ERROR) << "unable to parse " << path << sendLog;
		g_error_free (error);
		g_object_unref (msg);
		g_object_unref (session);
		return NULL;
	}

	g_object_unref (msg);
	g_object_unref (session);

	return result;
}

int BBTaskSchedule::run ()
{
	switch (obs_sched->getState ())
	{
		case BB_SCHEDULE_CREATED:
			sched_process = scheduleTarget (tar_id, obs_sched->getObservatoryId (), obs_sched);
			return 0;
		default:
			logStream (MESSAGE_WARNING) << "unknow BBTaskSchedule state: " << obs_sched->getState () << sendLog;
			return 0;
	}
}

int BBConfirmTask::run ()
{
	try
	{
		BBSchedules scheds (schedule_id);
		scheds.load ();
		for (BBSchedules::iterator iter = scheds.begin (); iter != scheds.end (); iter++)
		{
			if (iter->getObservatoryId () == observatory_id)
			{
				confirmTarget (scheds, *iter);
				break;
			}
			else
			{
				iter->updateState (BB_SCHEDULE_BACKUP, iter->getFrom (), iter->getTo ());
			}
		}
	}
	catch (rts2db::SqlError &er)
	{
		logStream (MESSAGE_ERROR) << "while confirming schedule " << schedule_id << " occured error " << er << sendLog;
		return 0;
	}
	return 0;
}

void BBConfirmTask::confirmTarget (BBSchedules &bbsch, ObservatorySchedule &schedule)
{
	std::ostringstream url;

	url << "/bbapi/confirm?id=" << findObservatoryMapping (schedule.getObservatoryId (), bbsch.getTargetId ()) << "&schedule_id=" << schedule.getScheduleId () << "&observatory_id=" << schedule.getObservatoryId ();
	JsonParser *ret = jsonRequest (schedule.getObservatoryId (), url.str ().c_str ());
	if (ret)
	{
		schedule.updateState (BB_SCHEDULE_CONFIRMED, json_node_get_int (json_parser_get_root (ret)), schedule.getTo ());
		g_object_unref (ret);
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
	BBTask *t = pop (true);
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

		char *conn_name;
		asprintf (&conn_name, "thread_%d", send_thread);

		((rts2db::DeviceDb *) getMasterApp ())->initDB (conn_name);
	}
	push (t);
}
