/* 
 * Basic sensor class.
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

#include "Axisd.hpp"

using namespace rts2axisd;

Axis::Axis (int argc, char **argv, const char *sn):rts2core::Device (argc, argv, DEVICE_TYPE_AXIS, sn)
{
	createValue(target, "TARGET", "[counts] target position", false, RTS2_VALUE_WRITABLE);
	createValue(centerPoint, "center", "[counts] center position", false);
	centerPoint->setValueDouble(0);
	createValue(scale, "scale", "[counts/mm] movement scale", false);
	scale->setValueDouble(1);
}

Axis::~Axis (void)
{
}

int Axis::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == target)
	{
		return moveTo(new_value->getValueInteger()) == 0 ? 0 : -2;
	}
	return Axis::setValue (old_value, new_value);
}

int Axis::mmToCounts (double mm)
{
	return (mm * scale->getValueDouble()) + centerPoint->getValueDouble();
}

double Axis::countsToMm (int counts)
{
	return (counts - centerPoint->getValueDouble()) / scale->getValueDouble();
}
