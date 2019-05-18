/*
 * TCS NG connection.
 * Copyright (C) 2016 Scott Swindel
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#include "connection/tcsng.h"
#include "connection/tcp.h"
#include <string>

using namespace rts2core;

ConnTCSNG::ConnTCSNG (rts2core::Block *_master, const char *_hostname, int _port, const char *_obsID, const char *_subID) : ConnTCP (_master, _hostname, _port)
{
	obsID = _obsID;
	subID = _subID;
	reqCount = 0;
	debug = false;
	
}

const char * ConnTCSNG::runCommand (const char *cmd, const char *req)
{
	char wbuf[200];
	size_t wlen = snprintf (wbuf, 200, "%s %s %d %s %s\n", obsID, subID, reqCount+1, cmd, req);
	init (reqCount == 0);


	if(debug)
		logStream(MESSAGE_INFO) << "Sending: " << wbuf << sendLog;

	sendData (wbuf, wlen, false);
	receiveTillEnd (ngbuf, NGMAXSIZE, 3);
	
	if(debug)
		logStream(MESSAGE_INFO) << "Recieved: " << ngbuf << sendLog;
	close (sock);
	sock = -1;

	wlen = snprintf (wbuf, 200, "%s %s %d ", obsID, subID, reqCount);

	if (strncmp (wbuf, ngbuf, wlen) != 0)
	{
		//throw rts2core::Error ("invalid reply");
	}

	while (isspace (wbuf[wlen]) && wbuf[wlen] != '\0')
		wlen++;
	
	reqCount++;

	return ngbuf + wlen;
}

const char * ConnTCSNG::request (const char *val)
{
	return runCommand ("REQUEST", val);
}

const char * ConnTCSNG::command (const char *cmd)
{
	return runCommand ("COMMAND", cmd);
}

double ConnTCSNG::getSexadecimalHours (const char *req)
{
	const char *ret = request (req);
	if (strlen (ret) < 6)
		throw rts2core::Error (std::string("reply to sexadecimal request must be at least 6 characters long. req:") + req + " ret:" + ret);

	int h,m;
	double sec;
	if (sscanf (ret, "%2d%2d%lf", &h, &m, &sec) != 3)
		throw rts2core::Error (std::string("cannot parse sexadecimal reply: ") + req + " ret:" + ret);
	return 15 * (h + m / 60.0 + sec / 3600.0);
}

double ConnTCSNG::getSexadecimalTime (const char *req)
{
	const char *ret = request (req);
	if (strlen (ret) < 6)
		throw rts2core::Error (std::string("reply to sexadecimal request must be at least 6 characters long. req:") + req + " ret:" + ret);

	int h,m;
	double sec;
	if (sscanf (ret, "%d:%d:%lf", &h, &m, &sec) != 3)
		throw rts2core::Error (std::string("cannot parse sexadecimal reply: ") + req + " ret:" + ret);
	return 15 * (h + m / 60.0 + sec / 3600.0);
}

double ConnTCSNG::getSexadecimalAngle (const char *req)
{
	const char *ret = request (req);
	if (strlen (ret) < 6)
		throw rts2core::Error (std::string("reply to sexadecimal request must be at least 6 characters long. req:") + req + " ret:" + ret);

	int h,m;
	double sec;
	if (sscanf (ret, "%3d%2d%lf", &h, &m, &sec) != 3)
		throw rts2core::Error (std::string("cannot parse sexadecimal reply: ") + req + " ret:" + ret);
	return h + m / 60.0 + sec / 3600.0;
}

int ConnTCSNG::getInteger (const char *req)
{
	const char *ret = request (req);
	return atoi(ret);
}
