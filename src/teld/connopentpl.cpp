/* 
 * OpenTpl interface.
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

#include "connopentpl.h"

#include <iomanip>
#include <netdb.h>
#include <fcntl.h>
#include <sstream>

using namespace rts2core;

int
OpenTpl::sendCommand (char *cmd, char *p1)
{
	std::ostringstream _os;
	_os << tpl_command_no << " " << cmd << " " << p1 << '\n';
	tpl_command_no++;
	int ret = send (sock, _os.str().c_str (), _os.str().length (), 0);
	logStream (MESSAGE_DEBUG) << "send " << _os.str () << " ret " << ret << sendLog;
	return ret == _os.str().length() ? 0 : -1;
}


int
OpenTpl::waitReply ()
{
	char buf[500];
        char *buf_top = buf;
	// read from socket till end of command is reached..
	int rs = 0;
	while (true)
	{
		int ret;
		struct timeval read_tout;

		read_tout.tv_sec = 5;
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
			connectionError (-1);
			return -1;
		}
        	int data_size = recv (sock, buf_top, 499 - (buf_top - buf), 0);
		if (data_size < 0)
		{
			connectionError (-1);
			return -1;
		}
		// now parse reply, look for '\n'
		char *bt = buf_top;
		while (bt < buf_top + data_size)
		{
			while (bt < buf_top + data_size && *bt != '\n')
				bt++;
			if (bt >= buf_top + data_size)
				break;
			*bt = '\0';
			// parse line - get first character..
			char *lp = buf;
			while (*lp != '\0' && !isspace (*lp))
				lp++;
			// try to convert to number
			char *ce;
			long int cmd_num = strtol (buf, &ce, 10);
			if (lp != ce)
			{
				logStream (MESSAGE_DEBUG) << "received info message " << buf << sendLog;
			}
			else if (cmd_num != tpl_command_no - 1)
			{
				handleEvent (buf);
			}
			else
			{
				int cmd_ret = handleCommand (lp + 1);
				if (cmd_ret == 1)
					return 0;
				if (cmd_ret == -1)
					return -1;
			}
			// cpy buffer
			memcpy (buf, bt + 1, data_size - (bt - buf));
			data_size -= bt + 1 - buf;
			buf_top = buf;
			bt = buf_top;
			std::cout << "data_size " << data_size << " buf " << buf << std::endl;
		}
	}
}

OpenTpl::OpenTpl (Rts2Block *_master, std::string _hostname, int _port)
:Rts2ConnNoSend (_master)
{
	hostname = _hostname;
	port = _port;

	tpl_command_no = 1;
}


OpenTpl::~OpenTpl ()
{
}


int
OpenTpl::idle ()
{

}


int
OpenTpl::init ()
{
        int ret;
        struct  sockaddr_in modbus_addr;
        struct  hostent *hp;

        sock = socket (AF_INET,SOCK_STREAM,0);
        if (sock == -1)
        {
                logStream (MESSAGE_ERROR) << "Cannot create socket for a Modbus TCP/IP connection, error: " << strerror (errno) << sendLog;
                return -1;
        }

        modbus_addr.sin_family = AF_INET;
        hp = gethostbyname(hostname.c_str ());
        bcopy ( hp->h_addr, &(modbus_addr.sin_addr.s_addr), hp->h_length);
        modbus_addr.sin_port = htons(port);

        ret = connect (sock, (struct sockaddr *) &modbus_addr, sizeof(modbus_addr));
        if (ret == -1)
        {
                logStream (MESSAGE_ERROR) << "Cannot connect socket, error: " << strerror (errno) << sendLog;
                return -1;
        }

        ret = fcntl (sock, F_SETFL, O_NONBLOCK);
        if (ret)
                return -1;
	return 0;
}

		
int
OpenTpl::receive (fd_set *set)
{

}


int
OpenTpl::tpl_set (const char *name, double value, int *status)
{
	std::ostringstream _os;
	_os << name << '=' << value;
	return sendCommand ("SET", _os.str().c_str());
}


int
OpenTpl::tpl_get (const char *name, double &value, int *status)
{
	sendCommand ("GET", name);
	waitReply ();
	value = atof (valReply);
}


int
OpenTpl::tpl_set (const char *name, int value, int *status)
{
	std::ostringstream _os;
	_os << name << '=' << value;
	return sendCommand ("SET", _os.str().c_str());
}


int
OpenTpl::tpl_get (const char *_name, int &value, int *status)
{
	sendCommand ("GET", _name);
	waitReply ();
	value = atoi (valReply);
}


int
OpenTpl::tpl_set (const char *_name, std::string value, int *tpl_status)
{
	std::ostringstream _os;
	_os << _name << '=' << value;
	return sendCommand ("SET", _os.str().c_str());
}


int
OpenTpl::tpl_get (const char *_name, std::string &value, int *tpl_status)
{
	sendCommand ("GET", _name);
	waitReply ();
	value = valReply;
}


void
OpenTpl::handleEvent (const char *buffer)
{
	logStream (MESSAGE_DEBUG) << "EVENT: " << buffer << sendLog;
	return 0;
}


int
OpenTpl::handleCommand (const char *buffer)
{
	// it is end of command sequence
	if (!strcmp (buffer, "COMMAND COMPLETE\r"))
	{
		logStream (MESSAGE_DEBUG) << "cmd cmplt" << sendLog;
		return 1;
	}
	// get command
	char *ce = buffer;
	while (*ce != '\0' && *ce != ' ')
		ce++;
	logStream (MESSAGE_DEBUG) << "command " << buffer << sendLog;
	return 0;
}
