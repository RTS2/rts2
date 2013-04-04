/* 
 * BB API access for RTS2.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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
#include "bbapi.h"
#include "bbdb.h"
#include "bbconn.h"

#include "xmlrpc++/XmlRpc.h"
#include "rts2db/target.h"
#include "rts2db/sqlerror.h"
#include "rts2json/jsonvalue.h"
#include "error.h"

#ifdef RTS2_JSONSOUP
#include <glib-object.h>
#include <json-glib/json-glib.h>
#endif // RTS2_JSONSOUP

using namespace rts2bb;

BBAPI::BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s, BBTasks *_queue):rts2json::JSONDBRequest (prefix, _http_server, s)
{
	g_type_init ();
	queue = _queue;
}

BBAPI::~BBAPI ()
{
	for (std::map <int, std::pair <double, JsonParser*> >::iterator iter = observatoriesJsons.begin (); iter != observatoriesJsons.end (); iter++)
	{
		g_object_unref (iter->second.second);
	}
	observatoriesJsons.clear ();
}

void BBAPI::executeJSON (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;

	os.precision (8);

	// calls returning binary data
	if (vals.size () == 1)
	{
		os << "{";

		if (vals[0] == "mapping")
		{
			int observatory_id = params->getInteger ("observatory_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown observatory ID");
			int tar_id = params->getInteger ("tar_id", -1);
			if (tar_id < 0)
				throw XmlRpc::JSONException ("unknown target ID");
			int obs_tar_id = params->getInteger ("obs_tar_id", -1);
			if (obs_tar_id < 0)
				throw XmlRpc::JSONException ("unknown local target ID");

			createMapping (observatory_id, tar_id, obs_tar_id);

			os << "\"ret\":0";
		}
		// report observation on slave node
		else if (vals[0] == "observation")
		{
			int observatory_id = params->getInteger ("observatory_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown observatory ID");
			int schedule_id = params->getInteger ("schedule_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown schedule ID");
			int obs_id = params->getInteger ("obs_id", -1);
			if (obs_id < 0)
				throw XmlRpc::JSONException ("unknown observation ID");
			int obs_tar_id = params->getInteger ("obs_tar_id", -1);
			if (obs_tar_id < 0)
				throw XmlRpc::JSONException ("unknown target ID");
			double obs_ra = params->getDouble ("obs_ra", NAN);
			double obs_dec = params->getDouble ("obs_dec", NAN);
			double obs_slew = params->getDouble ("obs_slew", NAN);
			double obs_start = params->getDouble ("obs_start", NAN);
			double obs_end = params->getDouble ("obs_end", NAN);
			double onsky = params->getDouble ("onsky", NAN);
			int good_images = params->getInteger ("good_images", 0);
			int bad_images = params->getInteger ("bad_images", 0);

			reportObservation (observatory_id, schedule_id, obs_id, obs_tar_id, obs_ra, obs_dec, obs_slew, obs_start, obs_end, onsky, good_images, bad_images);

			os << "\"ret\":0";
		}
		// schedule observation on all nodes
		else if (vals[0] == "schedule_all")
		{
			int tar_id = params->getInteger ("tar_id", -1);
			if (tar_id < 0)
				throw XmlRpc::JSONException ("unknown target ID");

			Observatories obs;
			obs.load ();

			int schedule_id = createSchedule (tar_id);

			for (Observatories::iterator iter = obs.begin (); iter != obs.end (); iter++)
			{
				ObservatorySchedule *oss = new ObservatorySchedule (schedule_id, iter->getId ());
				oss->updateState (BB_SCHEDULE_CREATED);
				// queue targets into scheduling thread
				queue->queueTask (new BBTaskSchedule (oss, tar_id, iter->getId ()));
			}
			os << "\"target_id\":" << tar_id << ",\"schedule_id\":" << schedule_id;
		}
		// schedule status
		else if (vals[0] == "schedule_status")
		{
			int schedule_id = params->getInteger ("id", -1);
			if (schedule_id < 0)
				throw XmlRpc::JSONException ("missing schedule ID");
			int observatory_id = params->getInteger ("observatory_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown observatory ID");

			ObservatorySchedule sched (schedule_id, observatory_id);
			sched.load ();

			sched.toJSON (os);
		}
		// return all schedules
		else if (vals[0] == "get_schedule")
		{
			int schedule_id = params->getInteger ("id", -1);
			if (schedule_id < 0)
				throw XmlRpc::JSONException ("missing schedule ID");

			BBSchedules bb_schedules (schedule_id);
			bb_schedules.load ();

			os << "\"schedule_id\":" << schedule_id << ",\"tar_id\":" << bb_schedules.getTargetId ()
			<< ",\"h\":["
				"{\"n\":\"Observatory\",\"t\":\"n\",\"c\":0},"
				"{\"n\":\"State\",\"t\":\"n\",\"c\":1},"
				"{\"n\":\"From\",\"t\":\"t\",\"c\":2},"
				"{\"n\":\"To\",\"t\":\"t\",\"c\":3},"
				"{\"n\":\"Created\",\"t\":\"t\",\"c\":4},"
				"{\"n\":\"Last update\",\"t\":\"t\",\"c\":5}"
				"]"
			",\"d\":";

			bb_schedules.toJSON (os);
		}
		else if (vals[0] == "get_all_schedules")
		{
			Schedules schedules;
			schedules.load ();

			os << "\"h\":["
				"{\"n\":\"Schedule ID\",\"t\":\"a\",\"prefix\":\"\",\"href\":0,\"c\":0},"
				"{\"n\":\"Target\",\"t\":\"a\",\"prefix\":\"../targets/\",\"href\":1,\"c\":2}"
			"],\"d\":[";

			schedules.toJSON (os);

			os << "]";
		}
		// schedule observation on one observatory
		else if (vals[0] == "schedule")
		{
			int observatory_id = params->getInteger ("observatory_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown observatory ID");
			int tar_id = params->getInteger ("tar_id", -1);
			if (tar_id < 0)
				throw XmlRpc::JSONException ("unknown target ID");
			
			Observatory obs (observatory_id);
			obs.load ();

			rts2db::Target *target = createTarget (tar_id, Configuration::instance ()->getObserver ());
			if (target == NULL)
				throw JSONException ("cannot find target with given ID");

			scheduleTarget (tar_id, observatory_id);

			os << "\"ret\":0";
		}
		// observatory update
		else if (vals[0] == "observatory")
		{
			int observatory_id = params->getInteger ("observatory_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown observatory ID");

			Observatory obs (observatory_id);
			obs.load ();

			JsonParser *newJson = json_parser_new ();
			GError *error = NULL;

			json_parser_load_from_data (newJson, source->getRequest ().c_str (), source->getRequest ().length (), &error);
			if (error)
			{
				logStream (MESSAGE_ERROR) << "unable to parse request from observatory " << observatory_id << error->code << ":" << error->message << sendLog;
				g_error_free (error);
				g_object_unref (newJson);
				throw JSONException ("unable to parse request");
			}
			else
			{
				observatoriesJsons[observatory_id] = std::pair <double, JsonParser *> (getNow (), newJson);
				os << "\"localtime\":" << std::fixed << getNow () << ",\"push\":" << ((obs.getURL ()[0] == '\0') ? "true" : "false");
			}
		}
		else if (vals[0] == "obspush")
		{
			AsyncObsAPI *aa = new AsyncObsAPI (this, NULL, connection, false);
		}
		else
		{
			dbJSON (vals, source, path, params, os);
		}
		os << "}";
	}
	// observatory API - proxy
	else if (vals.size () > 1 && vals[0] == "o")
	{
		if (vals[1] == "list")
		{
			os << "{";
			Observatories obs;
			obs.load ();
			for (Observatories::iterator iter = obs.begin (); iter != obs.end (); iter++)
			{
				if (iter != obs.begin ())
					os << ",";

				double lastup = NAN;

				std::map <int, std::pair <double, JsonParser*> >::iterator obs_iter = observatoriesJsons.find (iter->getId ());
				if (obs_iter != observatoriesJsons.end ())
					lastup = obs_iter->second.first;

				os << "\"" << iter->getId () << "\":[" << rts2json::JsonDouble (lastup)
					<< "," << iter->getPosition ()->lat
					<< "," << iter->getPosition ()->lng
					<< "," << iter->getAltitude ()
					<< ",\"" << iter->getURL () << "\"]";
			}
			os << "}";
			returnJSON (os.str ().c_str (), response_type, response, response_length);
			return;
		}
		if (vals.size () < 3)
			throw JSONException ("insuficient number of subdirs");
		int observatory_id = atoi (vals[1].c_str ());
		std::map <int, std::pair <double, JsonParser*> >::iterator iter = observatoriesJsons.find (observatory_id);
		if (iter == observatoriesJsons.end ())
			throw JSONException ("cannot find data for observatory");
		if (vals[2] == "api")
		{
			if (vals[3] == "devices")
			{
				os << "[";
				GList *devices = json_object_get_members (json_node_get_object (json_parser_get_root (iter->second.second)));
				for (GList *giter = g_list_first (devices); giter != g_list_last (devices); giter = g_list_next (giter))
				{
					if (giter != g_list_first (devices))
						os << ",";
					os << "\"" << ((gchar *) g_list_nth_data (giter, 0)) << "\"";
				}
				g_list_free (devices);
				os << "]";
			}
			else if (vals[3] == "getall")
			{
				JsonGenerator *gen = json_generator_new ();
				json_generator_set_root (gen, json_parser_get_root (iter->second.second));

				gchar *out = json_generator_to_data (gen, NULL);
				os << out;

				g_free (out);
				g_object_unref (gen);
			}
			else if (vals[3] == "get")
			{
				const char *device = params->getString ("d", "");
				JsonNode *node = json_object_get_member (json_node_get_object (json_parser_get_root (iter->second.second)), device);
				if (node == NULL)
					throw JSONException ("cannot find device");

				JsonGenerator *gen = json_generator_new ();
				json_generator_set_root (gen, node);
				gchar *out = json_generator_to_data (gen, NULL);
				os << out;

				g_free (out);
				g_object_unref (gen);
			}
			else
			{
				throw JSONException ("invalid request in observatory API");
			}
		}
		else if (vals[2] == "last_update")
		{
			os << iter->second.first;
		}
		else
		{
			throw JSONException ("invalid request");
		}
	}
	else
	{
		throw JSONException ("invalid path");
	}
	returnJSON (os.str ().c_str (), response_type, response, response_length);
}
