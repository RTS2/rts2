/* 
 * Abstract rotator class.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "rotad.h"

using namespace rts2rotad;

Rotator::Rotator (int argc, char **argv, const char *defname):rts2core::Device (argc, argv, DEVICE_TYPE_ROTATOR, defname)
{
	createValue (currentPosition, "CUR_POS", "[deg] current rotator position", true, RTS2_DT_DEGREES);
	createValue (targetPosition, "TAR_POS", "[deg] target rotator position", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (toGo, "TAR_DIFF", "[deg] difference between target and current position", false, RTS2_DT_DEG_DIST);
}


int Rotator::idle ()
{
	if ((getState () & ROT_MASK_ROTATING) == ROT_ROTATING)
	{
		long ret = isRotating ();
		if (ret < 0)
		{
			if (ret == -2)
			{
				maskState (ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_IDLE, "rotation finished");
			}
			else
			{
				// ends with error
				maskState (ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_IDLE, "rotation finished with error");
			}
			endRotation ();
			setIdleInfoInterval (60);
		}
		else
		{
			setIdleInfoInterval (((float) ret) / USEC_SEC);
		}
		updateToGo ();
		sendValueAll (currentPosition);
		sendValueAll (toGo);
	}
	return rts2core::Device::idle ();
}

int Rotator::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == targetPosition)
	{
		setTarget (new_value->getValueDouble ());
		targetPosition->setValueDouble (new_value->getValueDouble ());
		updateToGo ();
		maskState (ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_ROTATING | BOP_EXPOSURE, "rotator rotation started");
		return 0;
	}
	return rts2core::Device::setValue (old_value, new_value);
}

void Rotator::setCurrentPosition (double cp)
{
	currentPosition->setValueDouble (cp);
	updateToGo ();
}


void Rotator::updateToGo ()
{
	toGo->setValueDouble (getCurrentPosition () - getTargetPosition ());
}
