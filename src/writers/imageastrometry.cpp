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
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  ra_t = (x - getXoA ()) * getXPlate ();
  dec_t = (y - getYoA ()) * getYPlate ();
  if (getFlip ())
    ra_t *= -1;
  rotang = getRotang ();
  // transform to new coordinates, rotated by clokwise rotang..
  ra = cos (rotang) * ra_t - sin (rotang) * dec_t;
  dec = cos (rotang) * dec_t + sin (rotang) * ra_t;
  // we are obsering sky..so EW swap (unless there is mirror)
  // we are at new coordinates..apply offsets
  dec += getCenterDec ();
  // transoform ra offset due to sphere
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
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  return getOffset (x, y, getXoA (), getYoA (), chng_ra, chng_dec, sep_angle);
}

int
Rts2Image::getOffset (double x1, double y1, double x2, double y2,
		      double &chng_ra, double &chng_dec, double &sep_angle)
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
  // should be only 0 or 1
  ret = getMountFlip ();
  // mount flip correction
  val += ret * 180.0;
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
      return 0;
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
