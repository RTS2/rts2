/* 
 * API and pages for adding new target.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "httpreq.h"

namespace rts2json
{

/**
 * Add target to the database. Consists of those steps:
 *

 * @author Petr Kubanek <petr@kubanek.net>
 */
class AddTarget: public GetRequestAuthorized
{
	public:
		AddTarget (const char *prefix, HTTPServer *_http_server, XmlRpc::XmlRpcServer *s):GetRequestAuthorized (prefix, _http_server, "add new target", s) {};

		virtual void authorizedExecute (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

	private:
		void askForTarget (const char* &response_type, char* &response, size_t &response_length);
		void confimTarget (const char *tar, const char* &response_type, char* &response, size_t &response_length);
		void newTarget (const char *oriname, const char *name, int tarid, double ra, double dec, const char* &response_type, char* &response, size_t &response_length);
		void schedule (int tarid, const char* &response_type, char* &response, size_t &response_length);
};

}
