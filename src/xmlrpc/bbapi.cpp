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
#include "rts2json/jsonvalue.h"

#ifdef RTS2_JSONSOUP
#include <glib-object.h>
#include <json-glib/json-glib.h>
#endif // RTS2_JSONSOUP

using namespace rts2xmlrpc;

BBAPI::BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s):rts2json::GetRequestAuthorized (prefix, _http_server, NULL, s)
{
}

void BBAPI::authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	try
	{
		executeJSON (path, params, response_type, response, response_length);
	}
	catch (rts2core::Error &er)
	{
		throw JSONException (er.what ());
	}
}

void BBAPI::authorizePage (int &http_code, const char* &response_type, char* &response, size_t &response_length)
{
	throw JSONException ("cannot authorise user", HTTP_UNAUTHORIZED);
}

void BBAPI::executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));
  	std::ostringstream os;

	os.precision (8);

	XmlRpcd * master = (XmlRpcd *) getMasterApp ();
	// calls returning binary data
	if (vals.size () == 1 && vals[0] == "obsvalues")
	{
		std::cout << connection->getRequest () << std::endl;
		os << "{\"ret\":0}";
	}
	returnJSON (os.str ().c_str (), response_type, response, response_length);
}
