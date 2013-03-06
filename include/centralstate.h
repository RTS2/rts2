/* 
 * Central server state.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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


#ifndef __RTS2_CENTRALSTATE__
#define __RTS2_CENTRALSTATE__

#include <ostream>

#include "serverstate.h"

namespace rts2core
{

/**
 * Class which represents state of the central daemon.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class CentralState:public ServerState
{
	public:
		CentralState (rts2_status_t in_state):ServerState ()
		{
			setValue (in_state);
		}

		/**
		 * Return server state as string.
		 */
		const char *getStringShort ()
		{
			return getStringShort (getValue ());
		}

		std::string getString ()
		{
			return getString (getValue ());
		}

		/**
		 * Function to retrieve state character description.
		 */
		static const char *getStringShort (rts2_status_t _state);
		static std::string getString (rts2_status_t _state);

		friend std::ostream & operator << (std::ostream & _os, CentralState c_state)
		{
			_os << c_state.getStringShort ();
			return _os;
		}
};

}

#endif							 /* !__RTS2_CENTRALSTATE__ */
