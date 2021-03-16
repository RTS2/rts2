/* 
 * Infrastructure for UDP connections.
 * Copyright (C) 2005-2015 Petr Kubanek <petr@kubanek.net>
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

#include "connection/udp.h"
#include "libnova_cpp.h"

#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace rts2core;

ConnUDP::ConnUDP (int _port, rts2core::Block * _master, const char *_hostname, size_t _maxSize):ConnNoSend (_master)
{
	setPort (_port);
	maxSize = _maxSize;
	hostname = _hostname;
}

int ConnUDP::init ()
{
	int ret;

	bzero (&servaddr, sizeof (servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons (getLocalPort ());
	if (hostname == NULL)
	{
		servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	}
	else
	{
		servaddr.sin_addr.s_addr = inet_addr (hostname);
	}

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
	if (hostname == NULL)
	{
		ret = bind (sock, (struct sockaddr *) &servaddr, sizeof (servaddr));
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "ConnUPD::init bind: " << strerror (errno) << sendLog;
		}
	}
	if (maxSize >= buf_size)
	{
		delete[] buf;
		buf = new char[maxSize + 1];
	}
	return ret;
}

int ConnUDP::sendReceive (const char * in_message, char * ret_message, unsigned int length, int noreceive, float rectimeout)
{
	int ret;

	if (noreceive == 0)
	{
		// flush the recv buffer prior sending - to get relevant response only
		ret = 1;
		unsigned int slen = sizeof (servaddr);
		char buff[80];
		while (ret > 0)
			ret = recvfrom (sock, buff, 80, 0, (struct sockaddr *) &servaddr, &slen);
	}

	ret = sendto (sock, in_message, strlen(in_message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

	if (noreceive == 0)
	{
		ret = receiveMessage (ret_message, length, rectimeout);
	}

	return ret;
}

int ConnUDP::receiveMessage (char * ret_message, unsigned int length, float rectimeout)
{
	fd_set read_set;
	struct timeval read_tout;

	read_tout.tv_sec = rectimeout;
	read_tout.tv_usec = (rectimeout - floor (rectimeout)) * USEC_SEC;

	FD_ZERO (&read_set);
	FD_SET (sock, &read_set);

	int ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
	if (ret <= 0)
		// timeout..
		return -1;

	unsigned int slen = sizeof (servaddr);

	return recvfrom (sock, ret_message, length, 0, (struct sockaddr *) &servaddr, &slen);
}

int ConnUDP::receive (Block *block)
{
	int data_size = 0;
	if (sock >= 0 && block->isForRead (sock))
	{
		socklen_t size = sizeof (servaddr);
		data_size = recvfrom (sock, buf, maxSize, 0, (struct sockaddr *) &servaddr, &size);
		if (data_size < 0)
		{
			logStream (MESSAGE_DEBUG) << "error in receiving data over UDP: %m" << strerror (errno) << sendLog;
			return 1;
		}
		buf[data_size] = 0;
		command_buf_top = buf;
		process (data_size, servaddr);
	}
	return data_size;
}
