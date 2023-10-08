/*
 * Abstract class for server implementation.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2__HTTPSERVER__
#define __RTS2__HTTPSERVER__

#include <string>

#include <sys/types.h>
#include <sys/socket.h>

#include "block.h"
#include "userpermissions.h"
#include "rts2db/camlist.h"

namespace rts2json
{

class AsyncAPI;

/**
 * Interface for HTTP server. Declares methods needed by user authorization.
 */
class HTTPServer
{
	public:
		HTTPServer ()
		{
			sumAsync = NULL;
			numberAsyncAPIs = NULL;
		}

		/**
		 * If requests from localhost should be authorized.
		 */
		bool authorizeLocalhost () { return auth_localhost; }

		/**
		 * Returns true, if given path is marked as being public - accessible to all.
		 */
		virtual bool isPublic (struct sockaddr_in *saddr, const std::string &path) = 0;

		/**
		 * Returns true if session with a given sessionId exists.
		 *
		 * @param sessionId  Session ID.
		 *
		 * @return True if session with a given session ID exists.
		 */
		virtual bool existsSession (std::string sessionId) = 0;

		virtual void addExecutedPage () = 0;

		/**
		 * Return prefix for generated pages - usefull for pages behind proxy.
		 */
		virtual const char* getPagePrefix () = 0;

		virtual bool getDebug () = 0;

		virtual void sendValueAll (rts2core::Value * value) = 0;
		virtual rts2db::CamList *getCameras () = 0;

		virtual rts2core::connections_t *getConnections () = 0;

		virtual void getOpenConnectionType (int deviceType, rts2core::connections_t::iterator &current) = 0;

		/**
		 * Verify user credentials.
		 */
		virtual bool verifyDBUser (std::string username, std::string pass, rts2core::UserPermissions *userPermissions = NULL) = 0;

		/**
		 * Register asynchronous API call.
		 */
		void registerAPI (AsyncAPI *a);

		void asyncIdle ();

	protected:
		rts2core::ValueInteger *numberAsyncAPIs;
		rts2core::ValueInteger *sumAsync;
		std::list <rts2json::AsyncAPI *> asyncAPIs;

		bool auth_localhost;
};

}

#endif // !__RTS2__HTTPSERVER__
