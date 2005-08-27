#include <math.h>
#include <libnova/libnova.h>

#include "rts2image.h"

/**
 * Provides Rts2Image astrometry (position of pixels on image) functions.
 *
 * Most probably will converge to WCS class solution (or even WCSlib incorporation) in future.
 *
 * @author Petr Kubanek <pkubanek@lascaux.asu.cas.cz>
 */

int
Rts2Image::getRaDec (double x, double y, double &ra, double &dec)
{
  double ra_t, dec_t;
  double rotang;
  int startGetFailed = getFailed;
  ra_t = (x - getXoA ()) * getXPlate ();
  dec_t = (y - getYoA ()) * getYPlate ();
  rotang = getRotang ();
  // transform to new coordinates, rotated by clokwise rotang..
  ra = cos (rotang) * ra_t - sin (rotang) * dec_t;
  dec = cos (rotang) * dec_t + sin (rotang) * ra_t;
  // we are at new coordinates..apply offsets
  ra += getCenterRa ();
  dec += getCenterDec ();
  if (getFailed != startGetFailed)
    return -1;
  return 0;
}

int
Rts2Image::getOffset (double x, double y, double &chng_ra, double &chng_dec)
{
  return getOffset (x, y, getXoA (), getYoA (), chng_ra, chng_dec);
}

int
Rts2Image::getOffset (double x1, double y1, double x2, double y2,
		      double &chng_ra, double &chng_dec)
{
  int ret;
  double ra1, ra2, dec1, dec2;
  ret = getRaDec (x1, y1, ra1, dec1);
  if (ret)
    return ret;
  ret = getRaDec (x2, y2, ra2, dec2);
  if (ret)
    return ret;
  chng_ra = ra1 - ra2;
  chng_dec = dec1 - dec2;
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
