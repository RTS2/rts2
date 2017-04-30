/* 
 * Client for focuser attached to camera.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "clifocuser.h"
#include "command.h"

using namespace rts2camd;

ClientFocusCamera::ClientFocusCamera (rts2core::Connection * in_connection):rts2core::DevClientFocus (in_connection)
{
	activeConn = NULL;
}

ClientFocusCamera::~ClientFocusCamera (void)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_FOCUSER_END_MOVE, activeConn));
	activeConn = NULL;
}

void ClientFocusCamera::postEvent (rts2core::Event * event)
{
	struct focuserMove *fm;
	fm = (focuserMove *) event->getArg ();
	switch (event->getType ())
	{
		case EVENT_FOCUSER_SET:
		case EVENT_FOCUSER_OFFSET:
		case EVENT_FOCUSER_FILTEROFFSET:
			if (!strcmp (getName (), fm->focuserName))
			{
				if (event->getType () == EVENT_FOCUSER_SET)
				{
					if (connection->getValueDouble ("FOC_TAR") == fm->value)
					{
						rts2core::DevClientFocus::postEvent (event);
						return;
					}
					connection->queCommand (new rts2core::CommandChangeValue (getMaster (), "FOC_TAR", '=', fm->value));
				}
				else if (event->getType () == EVENT_FOCUSER_FILTEROFFSET)
				{
					if (connection->getValueDouble ("FOC_FILTEROFF") == fm->value)
					{
						rts2core::DevClientFocus::postEvent (event);
						return;
					}
					connection->queCommand (new rts2core::CommandChangeValue (getMaster (), "FOC_FILTEROFF", '=', fm->value));
				}
				else
				{
					if (connection->getValueDouble ("FOC_TOFF") == fm->value)
					{
						rts2core::DevClientFocus::postEvent (event);
						return;
					}
					connection->queCommand (new rts2core::CommandChangeValue (getMaster (), "FOC_TOFF", '=', fm->value));
				}
				// we process message
				fm->focuserName = NULL;
				activeConn = fm->conn;
			}
			break;
		case EVENT_FOCUSER_GET:
			if (!strcmp (getName (), fm->focuserName))
			{
				fm->value = getConnection ()->getValueInteger ("FOC_POS");
				fm->focuserName = NULL;
			}
			break;
	}
	rts2core::DevClientFocus::postEvent (event);
}

void ClientFocusCamera::focusingEnd ()
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_FOCUSER_END_MOVE, activeConn));
	rts2core::DevClientFocus::focusingEnd ();
}

void ClientFocusCamera::focusingFailed (int status)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_FOCUSER_END_MOVE, activeConn));
	activeConn = NULL;
	rts2core::DevClientFocus::focusingFailed (status);
}
