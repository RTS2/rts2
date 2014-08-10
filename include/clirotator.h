/* 
 * Connection for rotator.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CLI_ROTATOR__
#define __RTS2_CLI_ROTATOR__

#include "devclient.h"

#define EVENT_CUP_START_SYNC    RTS2_LOCAL_EVENT + 550
#define EVENT_CUP_SYNCED        RTS2_LOCAL_EVENT + 551
#define EVENT_CUP_ENDED         RTS2_LOCAL_EVENT + 552
#define EVENT_CUP_NOT_MOVE      RTS2_LOCAL_EVENT + 553

namespace rts2teld
{

class ClientRotator:public rts2core::DevClientRotator
{
	public:
		ClientRotator (rts2core::Connection * conn);
		virtual ~ ClientRotator ();
		virtual void postEvent (rts2core::Event * event);
};

}

#endif							 /* !__RTS2_CLI_ROTATOR__ */
