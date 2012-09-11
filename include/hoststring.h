/* 
 * Hoststring utility class.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_HOSTSTRING__
#define __RTS2_HOSTSTRING__

#include <rts2-config.h>

/**
 * Represents hostname with possible port number.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class HostString
{
	private:
		const char *hostname;
		int port;
	public:
		/**
		 * Construct server name and port from string. Server port is separated by :
		 *
		 * @param hoststring  String which describes server name and possibly port.
		 * @param defaultPort Port number which will be used if port is not specified.
		 */
		HostString (const char *hoststring, const char *defaultPort = RTS2_CENTRALD_PORT);

		const char *getHostname ()
		{
			return hostname;
		}

		int getPort ()
		{
			return port;
		}
};

#endif // !__RTS2_HOSTSTRING__
