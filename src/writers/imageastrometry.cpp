#include <math.h>
#include <libnova/libnova.h>

#include "rts2image.h"

/**
 * Provides Rts2Image astrometry (position of pixels on image) functions.
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
  int ret;
  double val;
  ret = getValue ("CAM_XOA", val);
  if (ret)
    {
      getFailed++;
      return getWidth () / 2.0;
    }
  return val;
}

double
Rts2Image::getYoA ()
{
  int ret;
  double val;
  ret = getValue ("CAM_YOA", val);
  if (ret)
    {
      getFailed++;
      return getHeight () / 2.0;
    }
  return val;
}

double
Rts2Image::getRotang ()
{
  int ret;
  double val;
  ret = getValue ("ROTANG", val);
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
  ret = getValue ("RASC", val);
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
  ret = getValue ("DECL", val);
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
  ret = getValue ("XPLATE", val);
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
  ret = getValue ("YPLATE", val);
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
  int ret;
  int val;
  ret = getValue ("MNT_FLIP", val);
  if (ret)
    {
      getFailed++;
      return -1;
    }
  return val;
}

int
Rts2Image::getFlip ()
{
  int ret;
  int val;
  ret = getValue ("FLIP", val);
  if (ret)
    {
      getFailed++;
      return 1;
    }
  return val;
}

int
Rts2Image::getCoord (struct ln_equ_posn &radec, char *ra_name, char *dec_name)
{
  int ret;
  ret = getValue (ra_name, radec.ra);
  if (ret)
    return ret;
  ret = getValue (dec_name, radec.dec);
  if (ret)
    return ret;
  return 0;
}

int
Rts2Image::getCoordTarget (struct ln_equ_posn &radec)
{
  return getCoord (radec, "TAR_RA", "TAR_DEC");
}

int
Rts2Image::getCoordAstrometry (struct ln_equ_posn &radec)
{
  return getCoord (radec, "CRVAL1", "CRVAL2");
}

int
Rts2Image::getCoordMount (struct ln_equ_posn &radec)
{
  return getCoord (radec, "MNT_RA", "MNT_DEC");
}

int
Rts2Image::getCoordBest (struct ln_equ_posn &radec)
{
  return getCoord (radec, "RASC", "DECL");
}

int
Rts2Image::getCoord (LibnovaRaDec & radec, char *ra_name, char *dec_name)
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
  return getCoord (radec, "TAR_RA", "TAR_DEC");
}

int
Rts2Image::getCoordAstrometry (LibnovaRaDec & radec)
{
  return getCoord (radec, "CRVAL1", "CRVAL2");
}

int
Rts2Image::getCoordMount (LibnovaRaDec & radec)
{
  return getCoord (radec, "MNT_RA", "MNT_DEC");
}

int
Rts2Image::getCoordBest (LibnovaRaDec & radec)
{
  return getCoord (radec, "RASC", "DECL");
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
