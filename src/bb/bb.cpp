/**
 * RTS2 Big Brother server.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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
#include "rts2json/directory.h"

#define OPT_WWW_DIR    OPT_LOCAL + 1

using namespace XmlRpc;
using namespace rts2bb;

BB::BB (int argc, char ** argv):
	rts2db::DeviceDb (argc, argv, DEVICE_TYPE_BB, "BB"),
	bbApi ("/api", this, this, &task_queue),
	javaScriptRequests ("/js", this, this),
	task_queue (this)
{
	rpcPort = 8889;

	createValue (queueSize, "queue_size", "task queue size", false);

	createValue (debugConn, "debug_conn", "debug connections calls", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	debugConn->setValueBool (false);

	addOption ('p', NULL, 1, "RPC listening port");
	addOption (OPT_WWW_DIR, "www-directory", 1, "default directory for BB requests");
}

void BB::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_TASK_SCHEDULE:
			task_queue.push ((BBTask *) event->getArg ());
			break;
		case EVENT_SCHEDULING_DONE:
			processSchedule ((ObservatorySchedule *) event->getArg ());
			break;
	}
	rts2db::DeviceDb::postEvent (event);
}

bool BB::verifyDBUser (std::string username, std::string pass, bool &executePermission)
{
	return verifyUser (username, pass, executePermission);
}

int BB::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			rpcPort = atoi (optarg);
			break;
		case OPT_WWW_DIR:
			XmlRpcServer::setDefaultGetRequest (new rts2json::Directory (NULL, this, optarg, "index.html", NULL));
			break;
		default:
			return rts2db::DeviceDb::processOption (opt);
	}
	return 0;
}

int BB::init ()
{
	int ret;

	ret = rts2db::DeviceDb::init ();
	if (ret)
		return ret;

	if (printDebug ())
		XmlRpc::setVerbosity (5);

	XmlRpcServer::bindAndListen (rpcPort);
	XmlRpcServer::enableIntrospection (true);

	return ret;
}

int BB::info ()
{
	queueSize->setValueInteger (task_queue.size ());
	return rts2db::DeviceDb::info ();
}

void BB::addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	rts2db::DeviceDb::addSelectSocks (read_set, write_set, exp_set);
	XmlRpcServer::addToFd (&read_set, &write_set, &exp_set);
}

void BB::selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	rts2db::DeviceDb::selectSuccess (read_set, write_set, exp_set);
	XmlRpcServer::checkFd (&read_set, &write_set, &exp_set);
}

void BB::processSchedule (ObservatorySchedule *obs_sched)
{
	try
	{
		BBSchedules all (obs_sched->getScheduleId ());
		all.load ();

		BBSchedules::iterator iter;

		double min_time = NAN;
		int min_observatory = -1;

		for (iter = all.begin (); iter != all.end (); iter++)
		{
			if (iter->getState () == BB_SCHEDULE_FAILED)
			{
				continue;
			}
			else if (iter->getState () == BB_SCHEDULE_OBSERVABLE)
			{
				if (!isnan (iter->getFrom ()) && (isnan (min_time) || iter->getFrom () < min_time))
				{
					min_time = iter->getFrom ();
					min_observatory = iter->getObservatoryId ();
				}
			}
			else if (iter->getState () == BB_SCHEDULE_CREATED)
			{
				break;
			}
			else
			{
				logStream (MESSAGE_WARNING) << "wrong state of schedule entry with id " << iter->getScheduleId () << " for observatory " << iter->getObservatoryId () << sendLog;
				break;
			}
		}

		if (iter != all.end ())
		{
			// some requests weren't processed
			return;
		}

		// inform selected observatory..
		task_queue.queueTask (new BBConfirmTask (obs_sched->getScheduleId (), min_observatory));
	}
	catch (rts2core::Error)
	{
		logStream (MESSAGE_ERROR) << "cannot process schedule requests " << obs_sched->getScheduleId () << sendLog;
	}
}

int main (int argc, char **argv)
{
	BB device (argc, argv);
	return device.run ();
}
