/* 
 * Pure TCP connection.
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

#include "conntcp.h"

#include <iomanip>
#include <netdb.h>
#include <fcntl.h>
#include <strings.h>

using namespace rts2core;


ConnTCP::ConnTCP (Rts2Block *_master, const char *_hostname, int _port)
:Rts2ConnNoSend (_master)
{
	hostname = _hostname;
	port = _port;
}


int
ConnTCP::init ()
{
	int ret;
	struct sockaddr_in apc_addr;
	struct hostent *hp;

	sock = socket (AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
		throw ConnCreateError ("cannot create socket for TCP/IP connection", errno);
        }

        apc_addr.sin_family = AF_INET;
        hp = gethostbyname(hostname);
        bcopy ( hp->h_addr, &(apc_addr.sin_addr.s_addr), hp->h_length);
        apc_addr.sin_port = htons(port);

        ret = connect (sock, (struct sockaddr *) &apc_addr, sizeof(apc_addr));
        if (ret == -1)
	 	throw ConnCreateError ("cannot connect socket", errno);

        ret = fcntl (sock, F_SETFL, O_NONBLOCK);
        if (ret)
		throw ConnCreateError ("cannot set socket non-blocking", errno);
        return 0;
}



