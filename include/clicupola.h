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

#include "devclient.h"

#define EVENT_CUP_SYNCED        RTS2_LOCAL_EVENT + 551
#define EVENT_CUP_ENDED         RTS2_LOCAL_EVENT + 552
#define EVENT_CUP_NOT_MOVE      RTS2_LOCAL_EVENT + 553

namespace rts2teld
{

class ClientCupola:public rts2core::DevClientCupola
{
	public:
		ClientCupola (rts2core::Rts2Connection * conn);
		virtual ~ ClientCupola ();
		virtual void syncFailed (int status);
		virtual void notMoveFailed (int status);
		virtual void postEvent (rts2core::Event * event);
	protected:
		virtual void syncEnded ();
};

}

#endif							 /* !__RTS2_CLI_CUP__ */
