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

}
