/* 
 * Abstract class for server implementation.
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

#include <string>

#include <sys/types.h>
#include <sys/socket.h>

namespace rts2json
{

/**
 * Interface for HTTP server. Declares methods needed by user authorization.
 */
class HTTPServer
{
	public:
		HTTPServer () {}
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

		/**
		 * Verify user credentials.
		 */
		virtual bool verifyUser (std::string username, std::string pass, bool &executePermission) = 0;
};

}
