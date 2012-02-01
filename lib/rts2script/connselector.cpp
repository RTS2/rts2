/*
 * Connections handling selection of targets.
 * Copyright (C) 2010 Petr Kubanek, Institute of Physics
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

#include "rts2script/connselector.h"

using namespace rts2plan;

ConnSelector::ConnSelector (rts2core::Block * in_master, const char *in_exe, int in_timeout):ConnProcess (in_master, in_exe, in_timeout)
{
	waitNext = false;
}

void ConnSelector::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_SELECT_NEXT:
			break;
	}
	ConnProcess::postEvent (event);
}

void ConnSelector::processCommand (char *cmd)
{
	if (!strcasecmp (cmd, "wait_next"))
	{
		waitNext = true;
	}
	else
	{
		ConnProcess::processCommand (cmd);
	}
}
