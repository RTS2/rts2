/* 
 * Connection class.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "radecparser.h"

#include "connection.h"
#include "centralstate.h"
#include "block.h"
#include "command.h"

#include "valuestat.h"
#include "valueminmax.h"
#include "valuerectangle.h"
#include "valuearray.h"

#include <iostream>

#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace rts2core;

ConnError::ConnError (Connection *conn, const char *_msg): Error (_msg)
{
	conn->connectionError (-1);
}

ConnError::ConnError (Connection *conn, const char *_msg, int _errn): Error ()
{
	setMsg (std::string ("connection error ") + _msg + ": " + strerror (_errn));
	conn->connectionError (-1);
}

Connection::Connection (Block * in_master):Object ()
{
	buf = new char[MAX_DATA + 1];
	buf_size = MAX_DATA;

	master = in_master;
	buf_top = buf;
}

Connection::~Connection (void)
{
}

int Connection::add (Block *block)
{
	return 0;
}

void Connection::postMaster (Event * event)
{
	master->postEvent (event);
}

int Connection::idle ()
{
	return 0;
}

void Connection::checkBufferSize ()
{
	// increase buffer if it's too small
	if (((int) buf_size) == (buf_top - buf))
	{
		char *new_buf = new char[buf_size + MAX_DATA + 1];
		memcpy (new_buf, buf, buf_size);
		buf_top = new_buf + (buf_top - buf);
		buf_size += MAX_DATA;
		delete[]buf;
		buf = new_buf;
	}
}

void Connection::processLine ()
{
}

bool Connection::receivedData (Block *block)
{
	return -1;
}

int Connection::receive (Block *block)
{
	return -1;
}

int Connection::writable (Block *block)
{
	return -1;
}

void Connection::deleteConnection (Connection *conn)
{
}

void Connection::successfullSend ()
{
	time (&lastGoodSend);
}

void Connection::getSuccessSend (time_t * in_t)
{
	*in_t = lastGoodSend;
}

void Connection::successfullRead ()
{
	time (&lastData);
}

void Connection::connConnected ()
{
}

void Connection::connectionError (int last_data_size)
{
}
