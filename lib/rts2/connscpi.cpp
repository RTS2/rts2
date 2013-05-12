/* 
 * Connection for SCPI-raw devices.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "error.h"
#include "block.h"
#include "connection/connscpi.h"

using namespace rts2core;

void ConnSCPI::gpibWriteBuffer (const char *cmd, int len)
{
	if (debug)
		logStream (MESSAGE_DEBUG) << "ConnSCPI::gpibWriteBuffer " << cmd << sendLog;
	while (len > 0)
	{
		ssize_t ret = write (devfile, cmd, len);
		if (ret < 0)
			throw Error (std::string ("cannot write to device:") + strerror (errno));
		cmd += ret;
		len -= ret;
	}
}

void ConnSCPI::gpibRead (void *reply, int &blen)
{
	char *r = (char *) reply;
	*r = '\0';
	int len = 0;
	while (len < blen)
	{
		ssize_t ret = read (devfile, r, 1);
		if (ret != 1)
			throw Error (std::string ("cannot read from device:") + strerror (errno));
		if (*r == endDelimiter)
			break;
		r++;
		len++;
	}
	*r = '\0';
	if (debug)
		logStream (MESSAGE_DEBUG) << "ConnSCPI::gpibRead " << reply << sendLog;
}

void ConnSCPI::gpibWriteRead (const char *cmd, char *reply, int blen)
{
	gpibWriteBuffer (cmd, strlen (cmd));
	gpibRead (reply, blen);
}

void ConnSCPI::initGpib ()
{
	devfile = open (device, O_RDWR);
	if (devfile < 0)
		throw Error (std::string ("cannot open ") + device + ":" + strerror (errno));
}
