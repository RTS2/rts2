/* 
 * Client for filter wheel attached to the camera.
 * Copyright (C) 2005-2008,2012 Petr Kubanek <petr@kubanek.net>
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

#include "cliwheel.h"
#include "camd.h"
#include "command.h"

using namespace rts2camd;

ClientFilterCamera::ClientFilterCamera (rts2core::Rts2Connection * conn, FilterVal *fv):rts2core::DevClientFilter (conn)
{
	filterVal = fv;
}

ClientFilterCamera::~ClientFilterCamera (void)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_FILTER_MOVE_END, filterVal));
}

void ClientFilterCamera::filterMoveEnd ()
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_FILTER_MOVE_END, filterVal));
	rts2core::DevClientFilter::filterMoveEnd ();
}

void ClientFilterCamera::filterMoveFailed (int status)
{
	getMaster ()->postEvent (new rts2core::Event (EVENT_FILTER_MOVE_END, filterVal));
	rts2core::DevClientFilter::filterMoveFailed (status);
}

void ClientFilterCamera::postEvent (rts2core::Event * event)
{
	struct filterStart *fs;
	switch (event->getType ())
	{
		case EVENT_FILTER_START_MOVE:
			fs = (filterStart *) event->getArg ();
			if (!strcmp (getName (), fs->filterName) && fs->filter >= 0)
			{
				connection->queCommand (new rts2core::CommandFilter (this, fs->filter));
				fs->filter = -1;
				filterVal->moving->setValueBool (true);
				((rts2core::Daemon *) getMaster ())->sendValueAll (filterVal->moving);
			}
			break;
		case EVENT_FILTER_GET:
			fs = (filterStart *) event->getArg ();
			if (!strcmp (getName (), fs->filterName))
				fs->filter = getConnection ()->getValueInteger ("filter");
			break;
	}
	rts2core::DevClientFilter::postEvent (event);
}

void  ClientFilterCamera::valueChanged (rts2core::Value * value)
{
	if (value->getName () == "filter")
	{
		getMaster ()->postEvent (new rts2core::Event (EVENT_FILTER_MOVE_END, filterVal));
	}
	rts2core::DevClientFilter::valueChanged (value);
}
