/* 
 * XML-RPC session class.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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
#include <time.h>

namespace rts2xmlrpc
{
/**
 * This class represents single session in XML-RPC interface.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Session
{
	private:
		std::string username;
		time_t expires;

		std::string sessionId;
	public:
		/**
		 * Creates new session. Fills in sessionId for the session.
		 *
		 * @param _username  Name of user creating session.  
		 * @param _expires   Time when session will expire.
		 */
		Session (std::string _username, time_t _expires);

		/**
		 * Retrieve session ID.
		 *
		 * @return Session ID associated with the session.
		 */
		std::string getSessionId ()
		{
			return sessionId;
		}

		/**
		 * Check if session is valid for a given session ID.
		 *
		 * @param _sessionId Session ID to check.
		 * @return True if session ID is still valid.
		 */
		bool isValid (std::string _sessionId)
		{
			if (hasExpired ())
				return false;
			return _sessionId == sessionId;
		}

		/**
		 * Check if session has expired.
		 *
		 * @return True if session has exprired.
		 */
		bool hasExpired ()
		{
			time_t now = time (NULL);
			return now > expires;
		}
};

}
