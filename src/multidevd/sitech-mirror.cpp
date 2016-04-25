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
}

int SitechMirror::movePosition (int pos)
{

}

int SitechMirror::isMoving ()
{
}
