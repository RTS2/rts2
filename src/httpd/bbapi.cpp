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

#include "httpd.h"
#include "rts2db/constraints.h"
#include "rts2db/plan.h"
#include "rts2json/jsondb.h"
#include "rts2json/jsonvalue.h"

#ifdef RTS2_JSONSOUP
#include <glib-object.h>
#include <json-glib/json-glib.h>
#endif // RTS2_JSONSOUP

using namespace rts2xmlrpc;

BBAPI::BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::JSONRequest (prefix, _http_server, s)
{
}

void BBAPI::executeJSON (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;

	os.precision (8);
	os << std::fixed;

	// calls returning binary data
	if (vals.size () == 1)
	{
		// return time the observatory might be able to schedule the request
		if (vals[0] == "schedule" || vals[0] == "confirm")
		{
			time_t t;
			std::vector < std::pair <double, double> > free_time;
			int ret = scheduleTarget (t, params, vals[0] == "confirm", free_time);
			os << "\"ret\":" << ret;
			if (ret == 0)
			{
				os << ",\"from\":" << t << ret;
			}
			else
			{
				switch (ret)
				{
					case -1:
						os << ",\"reason\":\"no free time available\"";
						break;
					case -2:
						os << ",\"reason\":\"cannot fit inside free time\"";
						break;

				}
			}
			os << ",\"free_time\":[";
			for (std::vector <std::pair <double, double> >::iterator iter = free_time.begin (); iter != free_time.end (); iter++)
			{
				if (iter != free_time.begin ())
					os << ",";
				os << "[" << iter->first << "," << iter->second << "]";
			}
			os << "]";

		}
		else if (vals[0] == "cancel")
		{
			int observatory_id = params->getInteger ("observatory_id", -1);
			int schedule_id = params->getInteger ("schedule_id", -1);

			rts2db::Plan p;
			if (p.loadBBSchedule (observatory_id, schedule_id))
				throw JSONException ("invalid schedule id");
			p.setPlanStatus (PLAN_STATUS_CANCELLED);
			if (p.save ())
				throw JSONException ("cannot update schedule request");
			os << "0";
		}
		else
		{
			throw JSONException ("invalid request" + path);
		}

	}
	returnJSON (os, response_type, response, response_length);
}

int BBAPI::scheduleTarget (time_t &ret_from, XmlRpc::HttpParams *params, bool only_confirm, std::vector <std::pair <double, double> > &free_time)
{
	rts2db::Target *tar = rts2json::getTarget (params);
	double from = params->getDouble ("from", getNow ());
	double to = params->getDouble ("to", from + 86400);
	if (to < from)
	{
		delete tar;
		throw JSONException ("to time is before from time");
	}

	int observatory_id = -1;
	int schedule_id = -1;
	if (only_confirm)
	{
		observatory_id = params->getInteger ("observatory_id", -1);
		schedule_id = params->getInteger ("schedule_id", -1);
		if (observatory_id == -1 || schedule_id == -1)
		{
			delete tar;
			throw JSONException ("missing schedule ID");
		}
	}

	// find free spots
	XmlRpcd *master = (XmlRpcd*) getMasterApp ();
	connections_t::iterator iter = master->getConnections ()->begin ();

	master->getOpenConnectionType (DEVICE_TYPE_SELECTOR, iter);
			
	rts2core::TimeArray *free_start = NULL;
	rts2core::TimeArray *free_end = NULL;

	std::vector <double>::iterator iter_fstart;
	std::vector <double>::iterator iter_fend;

	if (iter != master->getConnections ()->end ())
	{
		rts2core::Value *f_start = (*iter)->getValue ("free_start");
		rts2core::Value *f_end = (*iter)->getValue ("free_end");
		if (f_start == NULL || f_end == NULL)
		{
			logStream (MESSAGE_WARNING) << "cannot find free_start or free_end variables in " << (*iter)->getName () << sendLog;
		}
		else if (f_start->getValueType () == (RTS2_VALUE_TIME | RTS2_VALUE_ARRAY) && f_end->getValueType () == (RTS2_VALUE_TIME | RTS2_VALUE_ARRAY))
		{
			free_start = (rts2core::TimeArray*) f_start;
			free_end = (rts2core::TimeArray*) f_end;

			iter_fstart = free_start->valueBegin ();
			iter_fend = free_end->valueBegin ();
		}
		else
		{
			logStream (MESSAGE_WARNING) << "invalid free_start or free_end types: " << std::hex << f_start->getValueType () << " " << std::hex << f_end->getValueType () << sendLog;
		}
	}

	// get target observability
	rts2db::ConstraintsList violated;
	time_t f = from;
	double JD = ln_get_julian_from_timet (&f);
	time_t tto = to;
	double JD_end = ln_get_julian_from_timet (&tto);
	double dur = rts2script::getMaximalScriptDuration (tar, ((XmlRpcd *) getMasterApp ())->cameras);
	if (dur < 60)
		dur = 60;
	dur /= 86400.0;

	double t;

	// go through nights
	double fstart_JD = NAN;
	double fend_JD = NAN;

	if (free_start && iter_fstart != free_start->valueEnd () && iter_fend != free_end->valueEnd ())
	{
		f = *iter_fstart;
		fstart_JD = ln_get_julian_from_timet (&f);

		f = *iter_fend;
		fend_JD = ln_get_julian_from_timet (&f);

		if (JD < fstart_JD)
			JD = fstart_JD;
	}
	else
	{
		delete tar;
		return -1;
	}

	for (t = JD; t < JD_end; t += dur)
	{
		if (t > fend_JD)
		{
			iter_fstart++;
			iter_fend++;
			if (iter_fstart == free_start->valueEnd () || iter_fend == free_end->valueEnd ())
			{
				t = JD_end;
				break;
			}
			else
			{
				time_t tt = *iter_fstart;
				fstart_JD = ln_get_julian_from_timet (&tt);

				tt = *iter_fend;
				fend_JD = ln_get_julian_from_timet (&tt);
			}
		}
		if (tar->isGood (t))
		{
			double t2;
			// check full duration interval
			for (t2 = t; t2 < t + dur; t2 += 60 / 86400.0)
			{
				if (!(tar->isGood (t2)))
				{
					t = t2;
					continue;
				}
			}
			if (t2 >= t + dur)
			{
				ln_get_timet_from_julian (t, &ret_from);
				if (only_confirm)
				{
					confirmSchedule (tar, from, observatory_id, schedule_id);
				}
				break;
			}
		}
	}
	if (t >= JD_end)
		return -2;
	delete tar;
	return 0;
}


void BBAPI::confirmSchedule (rts2db::Target *tar, double f, int observatory_id, int schedule_id)
{
	rts2db::Plan p;
	if (p.loadBBSchedule (observatory_id, schedule_id) == 0)
		throw JSONException ("cannot confirm already created schedule");

	p.setTargetId (tar->getTargetID ());
	p.setPlanStart (f);
	p.setBBScheduleId (observatory_id, schedule_id);

	if (p.save ())
		throw JSONException ("cannot create plan schedule");

	((XmlRpcd*) getMasterApp ())->confirmSchedule (p);
}
