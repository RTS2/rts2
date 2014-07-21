/*
 * Provides Image astrometry (position of pixels on image) functions.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include <math.h>
#include <libnova/libnova.h>

#include "rts2fits/image.h"

using namespace rts2image;

void Image::getCoord (struct ln_equ_posn &radec, const char *ra_name, const char *dec_name)
{
 	
	getValueRa (ra_name, radec.ra, true);
	getValueDec (dec_name, radec.dec, true);
}

LibnovaRaDec Image::getCoord (const char *prefix)
{
	struct ln_equ_posn pos;
	std::string p = std::string (prefix) + "RA";
	getValueRa (p.c_str (), pos.ra, true);
	p = std::string (prefix) + "DEC";
	getValueDec (p.c_str (), pos.dec, true);
	return LibnovaRaDec (pos.ra, pos.dec);
}

void Image::getCoordObject (struct ln_equ_posn &radec)
{
	getCoord (radec, "OBJRA", "OBJDEC");
}

void Image::getCoordTarget (struct ln_equ_posn &radec)
{
	getCoord (radec, "TARRA", "TARDEC");
}

void Image::getCoordAstrometry (struct ln_equ_posn &radec)
{
	getCoord (radec, "CRVAL1", "CRVAL2");
}

void Image::getCoordMount (struct ln_equ_posn &radec)
{
	getCoord (radec, "TELRA", "TELDEC");
}

void Image::getCoordBest (struct ln_equ_posn &radec)
{
	try
	{
		getCoordAstrometry (radec);
	}
	catch (rts2core::Error &er)
	{
		getCoordTarget (radec);
	}
}

void Image::getCoordBestAltAz (struct ln_hrz_posn &hrz, struct ln_lnlat_posn *observer)
{
	struct ln_equ_posn equ;
	getCoordBest (equ);
	ln_get_hrz_from_equ (&equ, observer, getMidExposureJD (), &hrz);
}

void Image::getCoord (LibnovaRaDec & radec, const char *ra_name, const char *dec_name)
{
	struct ln_equ_posn pos;
	getCoord (pos, ra_name, dec_name);
	radec.setPos (&pos);
}

void Image::getCoordTarget (LibnovaRaDec & radec)
{
	getCoord (radec, "TARRA", "TARDEC");
}

void Image::getCoordAstrometry (LibnovaRaDec & radec)
{
	getCoord (radec, "CRVAL1", "CRVAL2");
}


void Image::getCoordMount (LibnovaRaDec & radec)
{
	getCoord (radec, "TELRA", "TELDEC");
}

void Image::getCoordBest (LibnovaRaDec & radec)
{
	try
	{
		getCoordAstrometry (radec);
	}
	catch (rts2core::Error &er)
	{
		getCoordTarget (radec);
	}
}

void Image::getCoordMountRawUn (LibnovaRaDec & radec)
{
	getCoord (radec, "U_TELRA", "U_TELDEC");
}

void Image::getCoordMountRawComputed (LibnovaRaDec & radec)
{
	int flip = 0;
	double lat = 50.0;

	getValue ("MNT_FLIP", flip);
	getValue ("LATITUDE", lat);
	getCoord (radec, "TELRA", "TELDEC");

	if (flip)
	{
		radec.setRa (radec.getRa () + 180.0);
		if (lat > 0)
			radec.setDec (180.0 - radec.getDec ());
		else
			radec.setDec (-180.0 - radec.getDec ());
	}
}

void Image::getCoordMountRawBest (LibnovaRaDec & radec)
{
	try
	{
		getCoordMountRawUn (radec);
	}
	catch (rts2core::Error &er)
	{
		getCoordMountRawComputed (radec);
	}
}

