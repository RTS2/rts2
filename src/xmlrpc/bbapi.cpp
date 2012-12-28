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

#include "xmlrpcd.h"
#include "rts2db/constraints.h"
#include "rts2json/jsonvalue.h"

#ifdef RTS2_JSONSOUP
#include <glib-object.h>
#include <json-glib/json-glib.h>
#endif // RTS2_JSONSOUP

using namespace rts2xmlrpc;

BBAPI::BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::JSONRequest (prefix, _http_server, s)
{
}

void BBAPI::executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;

	os.precision (8);
	os << std::fixed;

	// calls returning binary data
	if (vals.size () == 1)
	{
		// return time the observatory might be able to schedule the request
		if (vals[0] == "schedule")
		{
			rts2db::Target *tar = getTarget (params);
			double from = params->getDouble ("from", getNow ());
			double to = params->getDouble ("to", from + 86400);
			if (to < from)
				throw JSONException ("to time is before from time");

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
			// go through nights
			double t;
			for (t = JD; t < JD_end; t += dur)
			{
				if (tar->getViolatedConstraints (t).size () == 0)
				{
					double t2;
					// check full duration interval
					for (t2 = t; t2 < t + dur; t2 += 60 / 86400.0)
					{
						if (tar->getViolatedConstraints (t2).size () > 0)
						{
							t = t2;
							continue;
						}
					}
					if (t2 >= t + dur)
					{
						os << t;
						break;
					}
				}
			}
			if (t >= JD_end)
				os << "0";
		}
		else
		{
			throw JSONException ("invalid request" + path);
		}

	}
	returnJSON (os.str ().c_str (), response_type, response, response_length);
}
