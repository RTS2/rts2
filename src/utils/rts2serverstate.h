/* 
 * Server state support.
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SERVER_STATE__
#define __RTS2_SERVER_STATE__

#include <string.h>
#include <status.h>

/**
 * Server state class. Holds information about current server state.
 * It also holds old server state value, so it can be used to check if
 * server state has really changed.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ServerState
{
	public:
		Rts2ServerState ()
		{
			lastUpdate = 0;
			oldValue = 0;
			value = DEVICE_NOT_READY;
		}

		virtual ~ Rts2ServerState (void) {}

		void setValue (int new_value)
		{
			time (&lastUpdate);
			oldValue = value;
			value = new_value;
		}

		int getValue () { return value; }

		int getOldValue () { return oldValue; }

		bool maskValueChanged (int q_mask) { return (getValue () & q_mask) != (getOldValue () & q_mask); }
	private:
		int value;
		int oldValue;
		time_t lastUpdate;
};
#endif							 /* !__RTS2_SERVER_STATE__ */
