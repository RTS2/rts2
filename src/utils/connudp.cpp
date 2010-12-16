/* 
 * Infrastructure for Pierre Auger UDP connection.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "connudp.h"
#include "../utils/libnova_cpp.h"

#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace rts2core;

ConnUDP::ConnUDP (int _port, Rts2Block * _master, size_t _maxSize):Rts2ConnNoSend (_master)
{
	setPort (_port);
	maxSize = _maxSize;
}

int ConnUDP::init ()
{
	struct sockaddr_in bind_addr;
	int ret;

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons (getLocalPort ());
	bind_addr.sin_addr.s_addr = htonl (INADDR_ANY);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) << "ConnUPD::init socket: " << strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnUPD::init fcntl: " << strerror (errno) << sendLog;
		return -1;
	}
	ret = bind (sock, (struct sockaddr *) &bind_addr, sizeof (bind_addr));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnUPD::init bind: " << strerror (errno) << sendLog;
	}
	return ret;
}

int ConnUDP::receive (fd_set * set)
{
	char Wbuf[maxSize + 1];
	int data_size = 0;
	if (sock >= 0 && FD_ISSET (sock, set))
	{
		struct sockaddr_in from;
		socklen_t size = sizeof (from);
		data_size = recvfrom (sock, Wbuf, 80, 0, (struct sockaddr *) &from, &size);
		if (data_size < 0)
		{
			logStream (MESSAGE_DEBUG) << "error in receiving weather data: %m" << strerror (errno) << sendLog;
			return 1;
		}
		Wbuf[data_size] = 0;
		process (Wbuf, data_size, from);
	}
	return data_size;
}
