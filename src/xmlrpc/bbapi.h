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

#include "rts2db/target.h"
#include "rts2json/httpreq.h"
#include "asyncapi.h"
#include "bbapidb.h"

/** @file bbapi.h
 *
 * This header file declares classes for support of various JSON API calls. It
 * also includes documentation of those API calls.
 */

namespace rts2xmlrpc
{
/**
 * Class for BB API requests.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class BBAPI:public rts2json::JSONRequest
{
	public:
		BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s);

	protected:
		virtual void executeJSON (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);
	
	private:
		/**
		 * Confirm observation schedule.
		 */
		void confirmSchedule (rts2db::Target *tar, double f, const char *schedule_id);

		BBSchedules schedules;
};

}
