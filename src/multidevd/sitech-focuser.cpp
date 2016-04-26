/**
 * Driver for Sitech focuser
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

#include "sitech-focuser.h"

using namespace rts2focusd;

SitechFocuser::SitechFocuser (const char *name, rts2core::ConnSitech *sitech_c):Focusd (0, NULL), SitechMultidev ()
{
	setDeviceName (name);
	setSitechConnection (sitech_c);
}

int SitechFocuser::info ()
{
	sitech->getAxisStatus ('X', axisStatus);

	setPosition (axisStatus.y_pos);

	return Focusd::info ();	
}

int SitechFocuser::setTo (double num)
{
	requestX.y_dest = num;

	requestX.y_speed = 200000;

	sitech->sendXAxisRequest (requestX);

	return 0;
}

double SitechFocuser::tcOffset ()
{
	return 0;
}

bool SitechFocuser::isAtStartPosition ()
{
	return false;
}
