/* 
 * Copnnection for inotify events.
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

#include "connnotify.h"
#include <sys/ioctl.h>

using namespace rts2core;

ConnNotify::ConnNotify (rts2core::Block *_master):Rts2ConnNoSend (_master)
{
	debug = false;
}

int ConnNotify::init ()
{
	sock = inotify_init1 (IN_NONBLOCK);
        if (sock == -1)
        {
		throw Error ("cannot initialize notify FD");
        }
        return 0;
}

int ConnNotify::receive (fd_set * readset)
{
	if (sock >= 0 && FD_ISSET (sock, readset))
	{
		int len;
		if (ioctl (sock, FIONREAD, &len))
		{
			throw Error ("cannot get next read size");
		}
		if (len > 0)
		{
			struct inotify_event *event = (struct inotify_event*) (malloc (len));
			ssize_t ret = read (sock, event, len);
			if (ret != len)
			{
				logStream (MESSAGE_ERROR) << "invalid return while reading from notify stream: " << ret << " " << strerror (errno) << sendLog;
				return 0;
			}
			// handle multiple events
			struct inotify_event *ep = event;
			while (ep < (event + len))
			{
				getMaster ()->fileModified (ep);
				if (getDebug ())
				{
					if (ep->len > 1)
					{
						logStream (MESSAGE_DEBUG) << "notified update of file " << ep->name << sendLog;
					}
					else
					{
						logStream (MESSAGE_DEBUG) << "notified update of file with ID " << ep->wd << sendLog;
					}
				}
				ep += sizeof (struct inotify_event) + ep->len;
			}
			free (event);
		}
		return len;
	}
	return 0;
}
