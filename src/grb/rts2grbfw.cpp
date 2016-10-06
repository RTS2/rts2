/* 
 * Forwarding GRB connection.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "grbd.h"
#include "rts2grbfw.h"
#include "grbconst.h"

#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

Rts2GrbForwardConnection::Rts2GrbForwardConnection (rts2core::Block * in_master, int in_forwardPort):rts2core::ConnNoSend (in_master)
{
	forwardPort = in_forwardPort;
}

int Rts2GrbForwardConnection::init ()
{
	int ret;
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2GrbForwardConnection cannot create listen socket " << strerror (errno) << sendLog;
		return -1;
	}
	// do some listen stuff..
	const int so_reuseaddr = 1;
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (so_reuseaddr));
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons (forwardPort);
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2GrbForwardConnection::init bind " << strerror (errno) << sendLog;
		return -1;
	}
	ret = listen (sock, 5);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2GrbForwardConnection::init cannot listen: " << strerror (errno)
			<< sendLog;
		close (sock);
		sock = -1;
		return -1;
	}
	return 0;
}

int Rts2GrbForwardConnection::receive (rts2core::Block *block)
{
	int new_sock;
	struct sockaddr_in other_side;
	socklen_t addr_size = sizeof (struct sockaddr_in);
	if (block->isForRead (sock))
	{
		new_sock = accept (sock, (struct sockaddr *) &other_side, &addr_size);
		if (new_sock == -1)
		{
			logStream (MESSAGE_ERROR) << "Rts2GrbForwardConnection::receive accept " << strerror (errno) << sendLog;
			return 0;
		}
		logStream (MESSAGE_INFO)
			<< "Rts2GrbForwardClientConn::accept connection from "
			<< inet_ntoa (other_side.sin_addr) << " port " << ntohs (other_side.
			sin_port) <<
			sendLog;
		Rts2GrbForwardClientConn *newConn = new Rts2GrbForwardClientConn (new_sock, getMaster ());
		getMaster ()->addConnection (newConn);
	}
	return 0;
}


/**
 * Takes cares of client connections.
 */

Rts2GrbForwardClientConn::Rts2GrbForwardClientConn (int in_sock, rts2core::Block * in_master):rts2core::ConnNoSend (in_sock, in_master)
{
}

void Rts2GrbForwardClientConn::forwardPacket (int32_t *nbuf)
{
	int ret;
	ret = write (sock, nbuf, SIZ_PKT * sizeof (nbuf[0]));
	if (ret != SIZ_PKT * sizeof (nbuf[0]))
	{
		logStream (MESSAGE_ERROR)
			<< "Rts2GrbForwardClientConn::forwardPacket cannot forward "
			<< strerror (errno) << " (" << errno << ", " << ret << ")" << sendLog;
		connectionError (-1);
	}
}

void Rts2GrbForwardClientConn::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case RTS2_EVENT_GRB_PACKET:
			forwardPacket ((int32_t *) event->getArg ());
			break;
	}
	rts2core::ConnNoSend::postEvent (event);
}

int Rts2GrbForwardClientConn::receive (rts2core::Block *block)
{
	static int32_t loc_buf[SIZ_PKT];
	if (sock < 0)
		return -1;

	if (block->isForRead (sock))
	{
		int ret;
		ret = read (sock, loc_buf, SIZ_PKT);
		if (ret != SIZ_PKT)
		{
			logStream (MESSAGE_ERROR) << "Rts2GrbForwardClientConn::receive " << strerror (errno) << " (" << errno << ", " << ret << ")" << sendLog;
			connectionError (ret);
			return -1;
		}
		// get some data back..
		successfullRead ();
		return ret;
	}
	return 0;
}
