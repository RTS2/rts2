/* 
 * State changes triggering infrastructure. 
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

#include "httpd.h"

#include "connection/fork.h"
#include "timestamp.h"

using namespace rts2xmlrpc;

#ifndef RTS2_HAVE_PGSQL

void StateChangeRecord::run (HttpD *_master, rts2core::Connection *_conn, double validTime)
{
	std::cout << Timestamp (validTime) << " state of device: " << _conn->getName () << " " << _conn->getStateString () << std::endl;
}

#endif /* RTS2_HAVE_PGSQL */

void StateChangeCommand::run (HttpD *_master, rts2core::Connection *_conn, double validTime)
{
	int ret;
	rts2core::ConnFork *cf = new rts2core::ConnFork (_master, commandName.c_str (), true, false, 100);
	cf->addArg (_conn->getName ());
	cf->addArg (_conn->getStateString ());
	ret = cf->init ();
	if (ret)
	{
		delete cf;
		return;
	}

	_master->addConnection (cf);
}
