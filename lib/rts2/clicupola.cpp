/* 
 * Telescope control daemon.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#include "clicupola.h"
#include "command.h"

#include <math.h>
#include <libnova/libnova.h>

using namespace rts2teld;

ClientCupola::ClientCupola (rts2core::Connection * conn):rts2core::DevClientCupola (conn)
{
}

ClientCupola::~ClientCupola (void)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_CUP_ENDED, this));
}

void ClientCupola::syncEnded ()
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_CUP_SYNCED));
	rts2core::DevClientCupola::syncEnded ();
}

void ClientCupola::syncFailed (int status)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_CUP_SYNCED));
	rts2core::DevClientCupola::syncFailed (status);
}

void ClientCupola::notMoveFailed (int status)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_CUP_NOT_MOVE));
	rts2core::DevClientCupola::notMoveFailed (status);
}

void ClientCupola::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_CUP_NOT_MOVE:
			connection->queCommand (new rts2core::CommandCupolaNotMove (this));
			break;
	}
	rts2core::DevClientCupola::postEvent (event);
}
