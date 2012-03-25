/* 
 * Conteiner holding multiple daemon.
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
		iter->initDaemon ();
		iter->beforeRun ();
	}

	while (true)
	{
		for (iter = begin (); iter != end (); iter++)
		{
			iter->oneRunLoop ();
		}
	}
	return 0;
}
