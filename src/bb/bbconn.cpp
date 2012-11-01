/*
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "bbconn.h"
#include "bbdb.h"

using namespace rts2bb;

ConnBBQueue::ConnBBQueue (rts2core::Block * _master, const char *_exec):rts2script::ConnExe (_master, _exec, false)
{
}

void ConnBBQueue::processCommand (char *cmd)
{
	// create mapping
	if (!strcasecmp (cmd, "mapping"))
	{
		int observatory_id;
		int tar_id;
		int obs_tar_id;

		if (paramNextInteger (&observatory_id) || paramNextInteger (&tar_id) || paramNextInteger (&obs_tar_id))
			return;
		createMapping (observatory_id, tar_id, obs_tar_id);
	}
	else
	{
		rts2script::ConnExe::processCommand (cmd);
	}
}
