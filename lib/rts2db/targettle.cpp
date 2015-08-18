/*
 * Class for two line elements (Satellites) targets.
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/targettle.h"

#include "infoval.h"
#include "libnova_cpp.h"
#include "rts2fits/image.h"

using namespace rts2db;

TLETarget::TLETarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude):Target (in_tar_id, in_obs, in_altitude)
{
}

void TLETarget::load ()
{
	Target::load ();
	// try to parse TLE string..
}

void TLETarget::getPosition (struct ln_equ_posn *pos, double JD)
{
}

int TLETarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
}

void TLETarget::printExtra (Rts2InfoValStream & _os, double JD)
{
	Target::printExtra (_os, JD);
}

void TLETarget::writeToImage (rts2image::Image * image, double JD)
{
	Target::writeToImage (image, JD);
}

double TLETarget::getEarthDistance (double JD)
{
}

double TLETarget::getSolarDistance (double JD)
{
}
