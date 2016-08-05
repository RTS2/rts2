/* 
 * Container holding multiple daemons.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics AS CR  <kubanek@fzu.cz>
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

#include "multidev.h"

#include <signal.h>

using namespace rts2core;

int MultiDev::run ()
{
	MultiDev::iterator iter;
	for (iter = begin (); iter != end (); iter++)
	{
		optind = 1;
		(*iter)->initDaemon ();
		(*iter)->beforeRun ();
	}

	multiLoop ();
	return -1;
}

void MultiDev::multiLoop ()
{
	MultiDev::iterator iter;
	while (true)
	{
		struct timespec read_tout;
		read_tout.tv_sec = 10;
		read_tout.tv_nsec = 0;

		nfds_t polls = 0;

		for (iter = begin (); iter != end (); iter++)
		{
			(*iter)->addPollSocks ();
			polls += (*iter)->npolls;
		}

		struct pollfd allpolls[polls + 1];
		int pollsa[size ()];
		int i = 0, j = 0;
		for (iter = begin (); iter != end (); iter++, j++)
		{
			memcpy (allpolls + i, (*iter)->fds, sizeof (struct pollfd) * (*iter)->npolls);
			pollsa[j] = (*iter)->npolls;
			i += (*iter)->npolls;
		}

		sigset_t sigmask;
		sigemptyset (&sigmask);

		if (ppoll (allpolls, polls, &read_tout, &sigmask) > 0)
		{
			j = 0;
			int polloff = 0;
			for (iter = begin (); iter != end (); iter++, j++)
			{
				struct pollfd *oldfd = (*iter)->fds;
				(*iter)->fds = allpolls + polloff;
				(*iter)->pollSuccess ();
				(*iter)->fds = oldfd;
				polloff += pollsa[j];
			}
		}

		for (iter = begin (); iter != end (); iter++)
		{
			(*iter)->callIdle ();
		}
	}
}
