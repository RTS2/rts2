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
#ifndef RTS2_HAVE_INOTIFY_INIT1
#include <fcntl.h>
#endif

using namespace rts2core;

ConnNotify::ConnNotify (rts2core::Block *_master):ConnNoSend (_master)
{
	debug = false;
}

int ConnNotify::init ()
{
#ifdef RTS2_HAVE_INOTIFY_INIT1
	sock = inotify_init1 (IN_NONBLOCK);
#elif defined(RTS2_HAVE_SYS_INOTIFY_H)
	sock = inotify_init ();
	fcntl (sock, O_NONBLOCK);
#endif

#if defined(RTS2_HAVE_INOTIFY_INIT1) || defined(RTS2_HAVE_SYS_INOTIFY_H)
        if (sock == -1)
        {
//		throw Error ("cannot initialize notify FD");
        }
        return 0;
#else
	sock = -1;
	return 0;
#endif
}

int ConnNotify::receive (fd_set * readset)
{
	if (sock >= 0 && FD_ISSET (sock, readset))
	{
#ifdef RTS2_HAVE_SYS_INOTIFY_H
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
				free (event);
				return 0;
			}
			// handle multiple events
			struct inotify_event *ep = event;
			while (((char *) ep) < (((char *) event) + len))
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
#endif
	}
	return 0;
}


int ConnNotify::addWatch (const char *filename, uint32_t mask)
{
#ifdef RTS2_HAVE_SYS_INOTIFY_H
	return inotify_add_watch (sock, filename, mask);
#else
	return -1;
#endif
}
