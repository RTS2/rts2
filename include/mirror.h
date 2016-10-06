/**
 * Abstract class for mirror rotators.
 * Copyright (C) 2012,2016 Petr Kubanek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RTS2_MIRROR__
#define __RTS2_MIRROR__

#include "device.h"

namespace rts2mirror
{

class Mirror:public rts2core::Device
{
	public:
		Mirror (int argc, char **argv);
		virtual ~Mirror (void);
		virtual int idle ();

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual int movePosition (int pos) = 0;
		// return 1, when mirror is (still) moving
		virtual int isMoving () = 0;

		void addPosition (const char *pos);

		rts2core::ValueSelection *mirrPos;
};

}

#endif /* ! __RTS2_MIRROR__ */
