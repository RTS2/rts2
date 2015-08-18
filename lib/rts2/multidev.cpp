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

	while (true)
	{
		fd_set read_set;
		fd_set write_set;
		fd_set exp_set;

		FD_ZERO (&read_set);
		FD_ZERO (&write_set);
		FD_ZERO (&exp_set);

		struct timeval read_tout;
		read_tout.tv_sec = 10;
		read_tout.tv_usec = 0;

		for (iter = begin (); iter != end (); iter++)
		{
			(*iter)->addSelectSocks (read_set, write_set, exp_set);
		}

		if (select (FD_SETSIZE, &read_set, &write_set, &exp_set, &read_tout) > 0)
		{
			for (iter = begin (); iter != end (); iter++)
				(*iter)->selectSuccess (read_set, write_set, exp_set);
		}

		for (iter = begin (); iter != end (); iter++)
		{
			(*iter)->callIdle ();
		}
	}
	return 0;
}
