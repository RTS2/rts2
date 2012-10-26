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

#include "xmlrpc++/XmlRpc.h"
#include "rts2json/jsonvalue.h"
#include "error.h"
#include "bbapi.h"
#include "bbdb.h"

#ifdef RTS2_JSONSOUP
#include <glib-object.h>
#include <json-glib/json-glib.h>
#endif // RTS2_JSONSOUP

using namespace rts2bb;

BBAPI::BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):GetRequestAuthorized (prefix, _http_server, NULL, s)
{
}

void BBAPI::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	try
	{
		executeJSON (path, params, response_type, response, response_length);
	}
	catch (rts2core::Error &er)
	{
		throw XmlRpc::JSONException (er.what ());
	}
}

void BBAPI::authorizePage (int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	throw XmlRpc::JSONException ("cannot authorise user", HTTP_UNAUTHORIZED);
}

void BBAPI::executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;

	os.precision (8);

	// calls returning binary data
	if (vals.size () == 1)
	{
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

			os << "{\"ret\":0}";
		}
		// report observation on slave node
		if (vals[0] == "observation")
		{
			int observatory_id = params->getInteger ("observatory_id", -1);
			if (observatory_id < 0)
				throw XmlRpc::JSONException ("unknown observatory ID");
			int obs_id = params->getInteger ("obs_id", -1);
			if (obs_id < 0)
				throw XmlRpc::JSONException ("unknown observation ID");
			int tar_id = params->getInteger ("tar_id", -1);
			if (tar_id < 0)
				throw XmlRpc::JSONException ("unknown target ID");
			double obs_ra = params->getDouble ("obs_ra", NAN);
			double obs_dec = params->getDouble ("obs_dec", NAN);
			double obs_slew = params->getDouble ("obs_slew", NAN);
			double obs_start = params->getDouble ("obs_start", NAN);
			double obs_end = params->getDouble ("obs_end", NAN);
			double onsky = params->getDouble ("onsky", NAN);
			int good_images = params->getInteger ("good_images", 0);
			int bad_images = params->getInteger ("bad_images", 0);

			reportObservation (observatory_id, obs_id, tar_id, obs_ra, obs_dec, obs_slew, obs_start, obs_end, onsky, good_images, bad_images);

			os << "{\"ret\":0}";
		}
	}
	returnJSON (os.str ().c_str (), response_type, response, response_length);
}
