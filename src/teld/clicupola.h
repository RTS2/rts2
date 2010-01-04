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

#ifndef __RTS2_CLI_CUP__
#define __RTS2_CLI_CUP__

#include "../utils/rts2devclient.h"

#define EVENT_CUP_START_SYNC  RTS2_LOCAL_EVENT + 550
#define EVENT_CUP_SYNCED  RTS2_LOCAL_EVENT + 551

namespace rts2teld
{

class ClientCupola:public Rts2DevClientCupola
{
	protected:
		virtual void syncEnded ();
	public:
		ClientCupola (Rts2Conn * conn);
		virtual ~ ClientCupola ();
		virtual void syncFailed (int status);
		virtual void postEvent (Rts2Event * event);
};

}

#endif							 /* !__RTS2_CLI_CUP__ */
