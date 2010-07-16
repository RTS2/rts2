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

int Rts2Image::getRaDec (double x, double y, double &ra, double &dec)
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

int Rts2Image::getOffset (double x, double y, double &chng_ra, double &chng_dec, double &sep_angle)
{
	return getOffset (x, y, getXoA (), getYoA (), chng_ra, chng_dec, sep_angle);
}

int Rts2Image::getOffset (double x1, double y1, double x2, double y2, double &chng_ra, double &chng_dec, double &sep_angle)
{
	int ret;
	struct ln_equ_posn pos1, pos2;
	ret = getRaDec (x1, y1, pos1.ra, pos1.dec);
	if (ret)
		return ret;
	ret = getRaDec (x2, y2, pos2.ra, pos2.dec);
	if (ret)
		return ret;
	chng_ra = pos1.ra - pos2.ra;
	chng_dec = pos1.dec - pos2.dec;
	sep_angle = ln_get_angular_separation (&pos1, &pos2);
	return ret;
}

double Rts2Image::getXoA ()
{
	return xoa;
}

double Rts2Image::getYoA ()
{
	return yoa;
}

void Rts2Image::setXoA (double in_xoa)
{
	xoa = in_xoa;
	setValue ("CAM_XOA", in_xoa,
		"center in X axis (divide by binning (BIN_H)!)");
}

void Rts2Image::setYoA (double in_yoa)
{
	yoa = in_yoa;
	setValue ("CAM_YOA", in_yoa,
		"center in Y axis (divide by binning (BIN_V)!)");
}

double Rts2Image::getRotang ()
{
	//  return rotang;
	double val = 0;
	try
	{
		getValue ("ROTANG", val, true);
	}
	catch (rts2core::Error &er)
	{
		getFailed++;
	}
	return ln_deg_to_rad (val);
}

double Rts2Image::getCenterRa ()
{
	double val = 0;
	try
	{
		getValue ("TARRA", val, true);
	}
	catch (rts2core::Error &er)
	{
		getFailed++;
	}
	return val;
}

double Rts2Image::getCenterDec ()
{
	double val = 0;
	try
	{
		getValue ("TARDEC", val, true);
	}
	catch (rts2core::Error &er)
	{
		getFailed++;
	}
	return val;
}

double Rts2Image::getXPlate ()
{
	double val = 0;
	try
	{
		getValue ("XPLATE", val, true);
	}
	catch (rts2core::Error &er)
	{
		getFailed++;
	}
	return val / 3600.0;
}

double Rts2Image::getYPlate ()
{
	double val = 0;
	try 
	{
		getValue ("YPLATE", val, true);
	}
	catch (rts2core::Error &er)
	{
		getFailed++;
	}
	return val / 3600.0;
}

int Rts2Image::getMountFlip ()
{
	return mnt_flip;
}

int Rts2Image::getFlip ()
{
	int val;
	try
	{
		getValue ("FLIP", val, true);
	}
	catch (rts2core::Error &er)
	{
		getFailed++;
	}
	return val;
}

void Rts2Image::getCoord (struct ln_equ_posn &radec, const char *ra_name, const char *dec_name)
{
 	
	getValue (ra_name, radec.ra, true);
	getValue (dec_name, radec.dec, true);
}

LibnovaRaDec Rts2Image::getCoord (const char *prefix)
{
	struct ln_equ_posn pos;
	std::string p = std::string (prefix) + "RA";
	getValue (p.c_str (), pos.ra, true);
	p = std::string (prefix) + "DEC";
	getValue (p.c_str (), pos.dec, true);
	return LibnovaRaDec (pos.ra, pos.dec);
}

void Rts2Image::getCoordObject (struct ln_equ_posn &radec)
{
	getCoord (radec, "OBJRA", "OBJDEC");
}

void Rts2Image::getCoordTarget (struct ln_equ_posn &radec)
{
	getCoord (radec, "TARRA", "TARDEC");
}

void Rts2Image::getCoordAstrometry (struct ln_equ_posn &radec)
{
	getCoord (radec, "CRVAL1", "CRVAL2");
}

void Rts2Image::getCoordMount (struct ln_equ_posn &radec)
{
	getCoord (radec, "TELRA", "TELDEC");
}

void Rts2Image::getCoordBest (struct ln_equ_posn &radec)
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

void Rts2Image::getCoordBestAltAz (struct ln_hrz_posn &hrz, struct ln_lnlat_posn *observer)
{
	struct ln_equ_posn equ;
	getCoordBest (equ);
	ln_get_hrz_from_equ (&equ, observer, getMidExposureJD (), &hrz);
}

void Rts2Image::getCoord (LibnovaRaDec & radec, const char *ra_name, const char *dec_name)
{
	struct ln_equ_posn pos;
	getCoord (pos, ra_name, dec_name);
	radec.setPos (&pos);
}

void Rts2Image::getCoordTarget (LibnovaRaDec & radec)
{
	getCoord (radec, "TARRA", "TARDEC");
}

void Rts2Image::getCoordAstrometry (LibnovaRaDec & radec)
{
	getCoord (radec, "CRVAL1", "CRVAL2");
}


void Rts2Image::getCoordMount (LibnovaRaDec & radec)
{
	getCoord (radec, "TELRA", "TELDEC");
}

void Rts2Image::getCoordBest (LibnovaRaDec & radec)
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

int Rts2Image::createWCS (double x_off, double y_off)
{
	LibnovaRaDec radec;

	getFailed = 0;

	getCoordTarget (radec);
	if (getFailed != 0)
		return -1;

	double rotang = ln_rad_to_deg (getRotang ());

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

	setValue ("CRPIX1", getChannelWidth (0) / 2.0 + x_off, "X center");
	setValue ("CRPIX2", getChannelHeight (0) / 2.0 + y_off, "Y center");

	setValue ("EPOCH", 2000.0, "WCS equinox");

	return getFailed == 0 ? 0 : -1;
}
