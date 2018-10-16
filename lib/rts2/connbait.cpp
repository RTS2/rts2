/* 
 * BAIT interface.
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

#include "connection/bait.h"

#include <algorithm>
#include <iomanip>
#include <netdb.h>
#include <fcntl.h>
#include <sstream>
#include <sys/socket.h>

using namespace rts2core;

BAIT::BAIT (Block *_master, std::string _hostname, int _port):ConnTCP (_master, _hostname.c_str(), _port)
{
}

BAIT::~BAIT ()
{
}

int BAIT::idle ()
{
	return 0;
}

int BAIT::receive (__attribute__ ((unused)) Block *block)
{
	return -1;
}

void BAIT::writeRead (const char *cmd, char *reply, size_t buf_len)
{
	int cmdlen = strlen (cmd) + 1;
	int ret = send (sock, cmd, cmdlen, 0);
	if (ret != cmdlen)
		throw Error ("cannot send BAIT command");
	char *buftop = reply;
	size_t bufused = 0;
	while (sock > 0)
	{
		struct timeval read_tout;

		read_tout.tv_sec = 180;
		read_tout.tv_usec = 0;
			
		fd_set read_set;
		fd_set write_set;
		fd_set exp_set;

		FD_ZERO (&read_set);
		FD_ZERO (&write_set);
		FD_ZERO (&exp_set);

		FD_SET (sock, &read_set);

		ret = select (FD_SETSIZE, &read_set, &write_set, &exp_set, &read_tout);
		if (ret <= 0)
		{
			logStream (MESSAGE_ERROR) << "cannot receive reply from socket within 30 seconds, reinitiliazing" << sendLog;
			connectionError (-1);
			throw Error ("cannot read from connection");
		}


		cmdlen = recv (sock, buftop, buf_len - bufused, 0);
		if (cmdlen < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			throw Error ("cannot receive reply from BAIT");
		}
		if (cmdlen == 0)
			continue;
		buftop += cmdlen;
		bufused += cmdlen;
		if (*(buftop - 1) == '\0')
		{
			std::cout << "buf : " << reply << " len " << bufused << " cmdlen " << cmdlen << std::endl;
			return;
		}
		if (bufused >= buf_len)
			throw Error ("too long server reply");
	}
}
