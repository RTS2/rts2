/**
 * Driver for sitech mirror rotator
 * Copyright (C) 2016 Petr Kubanek
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

#include "sitech-mirror.h"

using namespace rts2mirror;

/**
 * Driver for Sitech mirror rotator
 *
 * @author Petr Kubanek <petr@kubanek.net>>
 */

SitechMirror::SitechMirror (const char *name, rts2core::ConnSitech *sitech_c):Mirror (0, NULL), SitechMultidev ()
{
	setDeviceName (name);
	setSitechConnection (sitech_c);

	createValue (posA, "posA", "A position", false, RTS2_VALUE_WRITABLE);
	createValue (posB, "posB", "B position", false, RTS2_VALUE_WRITABLE);

	createValue (currPos, "current_pos", "current position", false);
	createValue (tarPos, "target_pos", "target position", false, RTS2_VALUE_WRITABLE);
}

int SitechMirror::info ()
{
	sitech->getAxisStatus ('X', axisStatus);

	currPos->setValueLong (axisStatus.x_pos);

	return Mirror::info ();
}

int SitechMirror::movePosition (int pos)
{
	if (pos == 0)
		requestX.x_dest = posA->getValueLong ();
	else
		requestX.x_dest = posB->getValueLong ();

	requestX.x_speed = 200000;

	sitech->sendXAxisRequest (requestX);

	tarPos->setValueLong (requestX.x_dest);

	return 0;
}

int SitechMirror::isMoving ()
{
	int ret = info ();
	if (ret)
		return -1;
	return abs (axisStatus.x_pos - tarPos->getValueLong ()) < 1000 ? -2: 100;
}
