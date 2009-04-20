/*
 * Provides Rts2Image astrometry (position of pixels on image) functions.
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

#include "rts2image.h"

using namespace rts2image;

/*
 *
 * Most probably will converge to WCS class solution (or even WCSlib incorporation) in future.
 *
 * @author Petr Kubanek <pkubanek@asu.cas.cz>
 */
int
Rts2Image::getRaDec (double x, double y, double &ra, double &dec)
{
	double ra_t, dec_t;
	double rotang;
	int startGetFailed = getFailed;
	ra_t = (x - getXoA ());
	dec_t = (y - getYoA ());
	rotang = getRotang ();
	double cos_r = cos (rotang);
	double sin_r = sin (rotang);
	// transform to new coordinates, rotated by clockwise rotang..
	ra = cos_r * ra_t - sin_r * dec_t;
	dec = cos_r * dec_t + sin_r * ra_t;
	// we are at new coordinates..apply offsets
	dec *= getYPlate ();
	dec += getCenterDec ();
	// transform ra offset due to sphere
	ra *= getXPlate ();
	// EW swap when there is a mirror
	if (!getFlip ())
		ra *= -1;
	if (fabs (dec) < 89)
		ra /= cos (ln_deg_to_rad (dec));
	ra += getCenterRa ();
	if (getFailed != startGetFailed)
		return -1;
	return 0;
}


int
Rts2Image::getOffset (double x, double y, double &chng_ra, double &chng_dec,
double &sep_angle)
{
	return getOffset (x, y, getXoA (), getYoA (), chng_ra, chng_dec, sep_angle);
}


int
Rts2Image::getOffset (double x1, double y1, double x2, double y2,
double &chng_ra, double &chng_dec, double &sep_angle)
{
	int ret;
	struct ln_equ_posn pos1, pos2;
	logStream (MESSAGE_DEBUG) << "Rts2Image::getOffset " << x1 << " " << y1 <<
		" " << x2 << " " << y2 << sendLog;
	ret = getRaDec (x1, y1, pos1.ra, pos1.dec);
	if (ret)
		return ret;
	ret = getRaDec (x2, y2, pos2.ra, pos2.dec);
	if (ret)
		return ret;
	chng_ra = pos1.ra - pos2.ra;
	chng_dec = pos1.dec - pos2.dec;
	sep_angle = ln_get_angular_separation (&pos1, &pos2);
	logStream (MESSAGE_DEBUG) << "Rts2Image::getOffset " <<
		pos1.ra << " " << pos1.dec << " " << pos2.ra << " " << pos2.
		dec << " change " << chng_ra << " " << chng_dec << " sep " << sep_angle <<
		sendLog;
	return ret;
}


double
Rts2Image::getXoA ()
{
	return xoa;
}


double
Rts2Image::getYoA ()
{
	return yoa;
}


void
Rts2Image::setXoA (double in_xoa)
{
	xoa = in_xoa;
	setValue ("CAM_XOA", in_xoa,
		"center in X axis (divide by binning (BIN_H)!)");
}


void
Rts2Image::setYoA (double in_yoa)
{
	yoa = in_yoa;
	setValue ("CAM_YOA", in_yoa,
		"center in Y axis (divide by binning (BIN_V)!)");
}


double
Rts2Image::getRotang ()
{
	//  return rotang;
	int ret;
	double val;
	ret = getValue ("ROTANG", val, true);
	if (ret)
	{
		getFailed++;
		return 0;
	}
	return ln_deg_to_rad (val);
}


double
Rts2Image::getCenterRa ()
{
	int ret;
	double val;
	ret = getValue ("TARRA", val, true);
	if (ret)
	{
		getFailed++;
		return 0;
	}
	return val;
}


double
Rts2Image::getCenterDec ()
{
	int ret;
	double val;
	ret = getValue ("TARDEC", val, true);
	if (ret)
	{
		getFailed++;
		return 0;
	}
	return val;
}


double
Rts2Image::getXPlate ()
{
	int ret;
	double val;
	ret = getValue ("XPLATE", val, true);
	if (ret)
	{
		getFailed++;
		return 0;
	}
	return val / 3600.0;
}


double
Rts2Image::getYPlate ()
{
	int ret;
	double val;
	ret = getValue ("YPLATE", val, true);
	if (ret)
	{
		getFailed++;
		return 0;
	}
	return val / 3600.0;
}


int
Rts2Image::getMountFlip ()
{
	return mnt_flip;
}


int
Rts2Image::getFlip ()
{
	int ret;
	int val;
	ret = getValue ("FLIP", val, true);
	if (ret)
	{
		getFailed++;
		return 1;
	}
	return val;
}


int
Rts2Image::getCoord (struct ln_equ_posn &radec, const char *ra_name, const char *dec_name)
{
	int ret;
	ret = getValue (ra_name, radec.ra, true);
	if (ret)
		return ret;
	ret = getValue (dec_name, radec.dec, true);
	if (ret)
		return ret;
	return 0;
}


LibnovaRaDec
Rts2Image::getCoord (const char *prefix)
{
	struct ln_equ_posn pos;
	int ret;
	std::string p = std::string (prefix) + "RA";
	ret = getValue (p.c_str (), pos.ra, true);
	if (ret)
		throw KeyNotFound (this, p.c_str ());
	p = std::string (prefix) + "DEC";
	ret = getValue (p.c_str (), pos.dec, true);
	if (ret)
		throw KeyNotFound (this, p.c_str ());
	return LibnovaRaDec (pos.ra, pos.dec);
}


int
Rts2Image::getCoordObject (struct ln_equ_posn &radec)
{
	return getCoord (radec, "OBJRA", "OBJDEC");
}


int
Rts2Image::getCoordTarget (struct ln_equ_posn &radec)
{
	return getCoord (radec, "TARRA", "TARDEC");
}


int
Rts2Image::getCoordAstrometry (struct ln_equ_posn &radec)
{
	return getCoord (radec, "CRVAL1", "CRVAL2");
}


int
Rts2Image::getCoordMount (struct ln_equ_posn &radec)
{
	return getCoord (radec, "TELRA", "TELDEC");
}


int
Rts2Image::getCoordBest (struct ln_equ_posn &radec)
{
	int ret;
	ret = getCoordAstrometry (radec);
	if (ret)
		return getCoordTarget (radec);
	return ret;
}


int
Rts2Image::getCoord (LibnovaRaDec & radec, const char *ra_name, const char *dec_name)
{
	int ret;
	struct ln_equ_posn pos;
	ret = getCoord (pos, ra_name, dec_name);
	if (ret)
		return ret;
	radec.setPos (&pos);
	return 0;
}


int
Rts2Image::getCoordTarget (LibnovaRaDec & radec)
{
	return getCoord (radec, "TARRA", "TARDEC");
}


int
Rts2Image::getCoordAstrometry (LibnovaRaDec & radec)
{
	return getCoord (radec, "CRVAL1", "CRVAL2");
}


int
Rts2Image::getCoordMount (LibnovaRaDec & radec)
{
	return getCoord (radec, "TELRA", "TELDEC");
}


int
Rts2Image::getCoordBest (LibnovaRaDec & radec)
{
	int ret;
	ret = getCoordAstrometry (radec);
	if (ret)
		return getCoordTarget (radec);
	return ret;
}


int
Rts2Image::createWCS (double x_off, double y_off)
{
	LibnovaRaDec radec;
	int ret;

	getFailed = 0;

	ret = getCoordTarget (radec);
	if (ret)
		return ret;

	double rotang = getRotang ();

	// now create the record
	setValue ("RADECSYS", "FK5", "WCS coordinate system");

	setValue ("CTYPE1", "RA---TAN", "WCS axis 1 coordinates");
	setValue ("CTYPE2", "DEC--TAN", "WCS axis 2 coordinates");

	setValue ("CRVAL1", radec.getRa (), "WCS RA");
	setValue ("CRVAL2", radec.getDec (), "WCS DEC");

	setValue ("CDELT1", getXPlate (), "WCS X pixel size");
	setValue ("CDELT2", getYPlate (), "WCS Y pixel size");

	setValue ("CROTA1", rotang, "WCS rotang");
	setValue ("CROTA2", rotang, "WCS rotang");

	setValue ("CRPIX1", naxis[0] / 2.0 + x_off, "X center");
	setValue ("CRPIX2", naxis[1] / 2.0 + y_off, "Y center");

	setValue ("EPOCH", 2000.0, "WCS equinox");

	return getFailed == 0 ? 0 : -1;
}
