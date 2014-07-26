/* 
 * API access for RTS2.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "block.h"
#include "rts2json/asyncapi.h"
#include "rts2json/httpreq.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2json/jsondb.h"
#endif

/** @file api.h
 *
 * This header file declares classes for support of various JSON API calls. It
 * also includes documentation of those API calls.
 */

namespace rts2xmlrpc
{
/**
 * Class for API requests.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
#ifdef RTS2_HAVE_PGSQL
class API:public rts2json::JSONDBRequest
#else
class API:public rts2json::JSONRequest
#endif
{
	public:
		API (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s);

		void sendOwnValues (std::ostringstream & os, XmlRpc::HttpParams *params, double from, bool extended);

	protected:
		virtual void executeJSON (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	
	private:
		void getWidgets (const std::vector <std::string> &vals, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
};

}
