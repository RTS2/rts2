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

#include "mirror.h"

using namespace rts2mirror;

Mirror::Mirror (int in_argc, char **in_argv):rts2core::Device (in_argc, in_argv, DEVICE_TYPE_MIRROR, "M0")
{
	createValue (mirrPos, "MIRP", "mirror position", true, RTS2_VALUE_WRITABLE);
}

Mirror::~Mirror (void)
{

}

int Mirror::idle ()
{
	int ret;
	ret = rts2core::Device::idle ();
	switch (getState () & MIRROR_MASK)
	{
		case MIRROR_MOVE:
			ret = isMoving ();
			if (ret >= 0)
			{
				setTimeoutMin (ret);
				return 0;
			}
			if (ret == -1)
			{
				maskState (DEVICE_ERROR_MASK | MIRROR_MASK, DEVICE_ERROR_HW | MIRROR_NOTMOVE, "move finished with error");
			}
			else
			{
				maskState (MIRROR_MASK, MIRROR_NOTMOVE, "move finished without error");
			}
			break;
	}
	// set timeouts..
	setTimeoutMin (100000);
	return ret;
}

int Mirror::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == mirrPos)
	{
		int ret = movePosition (new_value->getValueInteger ());
		if (ret)
			return -2;
		maskState (MIRROR_MASK, MIRROR_MOVE, "moving to new position");
		return 0;
	}
	return Device::setValue (old_value, new_value);
}

void Mirror::addPosition (const char *position)
{
	mirrPos->addSelVal (position);
}
