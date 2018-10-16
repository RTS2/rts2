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

#include "connection/opentpl.h"

#include <algorithm>
#include <iomanip>
#include <netdb.h>
#include <fcntl.h>
#include <sstream>
#include <sys/socket.h>

using namespace rts2core;

int OpenTpl::sendCommand (const char *cmd, const char *p1, bool wait)
{
	std::ostringstream _os;
	_os << tpl_command_no << " " << cmd << " " << p1 << '\n';
	int ret = send (sock, _os.str().c_str (), _os.str().length (), 0);
	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "send " << _os.str () << " ret " << ret << sendLog;

	if (ret > 0)
	{
		used_command_ids.push_back (tpl_command_no);
		tpl_command_no++;
	}
	else if (ret < 0)
	{
	 	connectionError (-1);
		logStream (MESSAGE_ERROR) << "error " << strerror(errno) << " " << errno << " sock " << sock << sendLog;
		return -1;
	}
	if (wait == false)
		return ret == (int) _os.str().length() ? 0 : -1;
	return waitReply ();
}

int OpenTpl::waitReply ()
{
	try
	{
		char tpl_buf[500];
		char *tpl_buf_top = tpl_buf;
		// read from socket till end of command is reached..
		while (sock > 0)
		{
			int ret;
			struct timeval read_tout;

			read_tout.tv_sec = 30;
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
				return -1;
			}
			int data_size = recv (sock, tpl_buf_top, 499 - (tpl_buf_top - tpl_buf), 0);
			if (data_size <= 0)
			{
				connectionError (-1);
				return -1;
			}
			// print which new data were received..
			*(tpl_buf_top + data_size) = '\0';
			if (getDebug ())
				logStream (MESSAGE_DEBUG) << "new data: '" << tpl_buf_top << "'" << sendLog;
			// now parse reply, look for '\n'
			char *bt = tpl_buf_top;
			while (bt < tpl_buf_top + data_size)
			{
				while (bt < tpl_buf_top + data_size && *bt != '\n')
					bt++;
				if (bt >= tpl_buf_top + data_size)
				{
					tpl_buf_top += data_size;
					break;
				}
				*bt = '\0';
				// logStream (MESSAGE_DEBUG) << "received: " << tpl_buf << sendLog;
				// parse line - get first character..
				char *lp = tpl_buf;
				while (*lp != '\0' && !isspace (*lp))
					lp++;
				// try to convert to number
				char *ce;
				long int cmd_num = strtol (tpl_buf, &ce, 10);
				if (lp != ce)
				{
				  	if (getDebug ())
						logStream (MESSAGE_DEBUG) << "received info message " << tpl_buf << sendLog;
				}
				else 
				{
					int cmd_ret = handleCommand (lp + 1, cmd_num == *(--used_command_ids.end ()));
					// command sucessfully ended..
					if (cmd_ret == 1)
					{
						// remove command from used_command_ids
						std::vector <int>::iterator iter = std::find (used_command_ids.begin (), used_command_ids.end (), cmd_num);
						if (iter == --(used_command_ids.end ()))
						{
							if (iter == used_command_ids.begin ())
							{
								tpl_command_no = 1;
							}
							else
							{
								tpl_command_no = *(iter - 1) + 1;
							}
							used_command_ids.erase (iter);
							return 0;
						}
						if (iter != used_command_ids.end ())
							used_command_ids.erase (iter);
					}
				}
				// cpy buffer
				data_size = tpl_buf_top + data_size - bt;
				memmove (tpl_buf, bt + 1, data_size);
				tpl_buf_top = tpl_buf + data_size - 1;
				data_size = 0;
				bt = tpl_buf;
			}
		}
	}
	catch (OpenTplError &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return 0;
}

OpenTpl::OpenTpl (Block *_master, std::string _hostname, int _port):ConnTCP (_master, _hostname.c_str(), _port)
{
	tpl_command_no = 1;
}

OpenTpl::~OpenTpl ()
{
}

int OpenTpl::idle ()
{
	return 0;
}

int OpenTpl::receive (__attribute__ ((unused)) Block *block)
{
	return -1;
}

int OpenTpl::set (const char *_name, double value, __attribute__ ((unused)) int *tpl_status, bool wait)
{
	std::ostringstream _os;
	_os << _name << '=' << std::setprecision(10) << value;
	sendCommand ("SET", _os.str().c_str(), wait);
	return 0;
}

int OpenTpl::get (const char *_name, double &value, __attribute__ ((unused)) int *tpl_status)
{
	sendCommand ("GET", _name);
	value = atof (valReply);
	return 0;
}

int OpenTpl::set (const char *_name, int value, __attribute__ ((unused)) int *tpl_status, bool wait)
{
	std::ostringstream _os;
	_os << _name << '=' << value;
	return sendCommand ("SET", _os.str().c_str(), wait);
}

int OpenTpl::get (const char *_name, int &value, __attribute__ ((unused)) int *tpl_status)
{
	sendCommand ("GET", _name);
	value = atoi (valReply);
	return 0;
}

int OpenTpl::set (const char *_name, std::string value, __attribute__ ((unused)) int *tpl_status)
{
	std::ostringstream _os;
	_os << _name << '=' << value;
	return sendCommand ("SET", _os.str().c_str());
}

int OpenTpl::get (const char *_name, std::string &value, __attribute__ ((unused)) int *tpl_status)
{
	sendCommand ("GET", _name);
	value = valReply;
	return 0;
}

void OpenTpl::handleEvent (const char *buffer)
{
	logStream (MESSAGE_DEBUG) << "EVENT: " << buffer << sendLog;
}

int OpenTpl::handleCommand (char *buffer, bool isActual)
{
	// get command
	char *ce = buffer;
	while (*ce != '\0' && *ce != ' ')
		ce++;
	if (*ce == '\0')
		throw OpenTplError ("Cannot handle command");

	*ce = '\0';
	ce++;
	if (!strcmp (buffer, "COMMAND"))
	{
		while (isspace (*ce))
			ce++;
		if (!strcmp (ce, "COMPLETE\r"))
			return 1;
		if (!strcmp (ce, "OK\r"))
			return 0;
		logStream (MESSAGE_ERROR) << "unknow COMMAND subcommand " << ce << sendLog;
	}
	else if (!strcmp (buffer, "DATA"))
	{
		char *subc = ce;
		while (*ce != '\0' && *ce != ' ')
			ce++;
		if (*ce != ' ')
			throw (OpenTplError ("Cannot find space in data command"));
		*ce = '\0';
		if (!strcmp (subc, "OK"))
		{
			if (isActual)
				strcpy (valReply, "1");
			return 0;
		}
		ce++;
		if (!strcmp (subc, "ERROR"))
		{
			if (isActual)
			{
			  	std::string desc = std::string ("DATA ERROR ") + std::string (ce);
				throw (OpenTplError (desc.c_str ()));
			}
			else
			{
				logStream (MESSAGE_DEBUG) << "Error " << ce << sendLog;
				return 0;
			}
		}
		// look for data = sign..
		while (*ce != '\0' && *ce != '=')
			ce++;
		if (*ce != '=')
			throw OpenTplError ("Cannot find equal sign");
		ce++;
		// if it starts with "..
		if (*ce == '"')
		{
			ce++;
			char *bt = ce;
			while (*bt != '"' && *bt != '\0')
				bt++;
			if (*bt != '"')
				throw OpenTplError ("Cannot find ending \"");
			*bt = '\0';
		}
		if (isActual)
			strcpy (valReply, ce);
	}
	else if (!strcmp (buffer, "EVENT"))
	{
		handleEvent (ce);
	}
	else
	{
		logStream (MESSAGE_ERROR) << "unknow reply type: " << buffer << sendLog;
	}
	return 0;
}
