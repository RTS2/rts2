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
	debug = false;
}


bool
ConnTCP::checkBufferForChar (std::istringstream **_is, char end_char)
{
	// look for endchar in received data..
	for (char *p = buf; p < buf_top; p++)
	{
		if (*p == end_char)
		{
			*p = '\0';
			*_is = new std::istringstream (std::string (buf));
			if (p != buf_top)
			{
				memmove (buf, p + 1, buf_top - p - 1);
				buf_top -= p + 1 - buf;
			}
			return true;
		}
	}
	return false;
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


void
ConnTCP::sendData (void *data, int len, bool binary)
{
	int rest = len;
	while (rest > 0)
	{
		int ret;
		ret = send (sock, (char *) data + (len - rest), rest, 0);
		if (ret == -1)
		{
		  	if (errno == EINTR)
				continue;
			if (debug)
			{
				Rts2LogStream ls = logStream (MESSAGE_DEBUG);
				ls << "failed to send ";
				if (binary)
					ls.logArrAsHex ((char *) data, len);
				else
				  	ls << data;
				ls << sendLog;
			}
			throw ConnSendError ("cannot send data", errno);
		}
		rest -= ret;
	}
	if (debug)
	{
		Rts2LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "send ";
		if (binary)
			ls.logArrAsHex ((char *) data, len);
		else
		  	ls << data;
		ls << sendLog;
	}
}


void
ConnTCP::sendData (const char *data)
{
	sendData ((void *) data, strlen (data), false);
}


void
ConnTCP::receiveData (void *data, size_t len, int wtime, bool binary)
{
	int rest = len;

	fd_set read_set;

	struct timeval read_tout;
	read_tout.tv_sec = wtime;
	read_tout.tv_usec = 0;

	while (rest > 0)
	{
		FD_ZERO (&read_set);

		FD_SET (sock, &read_set);

		int ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
		if (ret < 0)
			throw ConnError ("error calling select function", errno);
		else if (ret == 0)
		  	throw ConnTimeoutError ("timeout during receiving data");

		// read from descriptor
		ret = recv (sock, (char *)data + (len - rest), rest, 0);
		if (ret == -1)
			throw ConnReceivingError ("cannot read from TCP/IP connection", errno);
		rest -= ret;
	}

	if (debug)
	{
		Rts2LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "recv ";
		if (binary)
			ls.logArrAsHex ((char *)data, len);
		else
		  	ls << data;
		ls << sendLog;
	}
}


void
ConnTCP::receiveData (std::istringstream **_is, int wtime, char end_char)
{
	// check if buffer contains end character..
	
	fd_set read_set;

	struct timeval read_tout;
	read_tout.tv_sec = wtime;
	read_tout.tv_usec = 0;

	if (checkBufferForChar (_is, end_char))
		return;

	while (true)
	{
	 	checkBufferSize ();
		FD_ZERO (&read_set);

		FD_SET (sock, &read_set);

		int ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
		if (ret < 0)
			throw ConnError ("error calling select function", errno);
		else if (ret == 0)
		  	throw ConnTimeoutError ("timeout during receiving data");

		// read from descriptor
		ret = recv (sock, buf_top, buf_size - (buf_top - buf), 0);
		if (ret == -1)
			throw ConnReceivingError ("cannot read from TCP/IP connection", errno);
		buf_top += ret;
		if (debug)
		{
			*(buf_top + 1) = '\0';
			logStream (MESSAGE_DEBUG) << "received " << buf << sendLog;
		}

		// look for end character..
		if (checkBufferForChar (_is, end_char))
			return;
	}

}
