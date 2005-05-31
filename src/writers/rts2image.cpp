#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/libnova.h>

#include "rts2image.h"
#include "imghdr.h"

#include "../utils/mkpath.h"
#include "../utils/config.h"

Rts2Image::Rts2Image (char *in_filename,
		      const struct timeval *in_exposureStart)
{
  createImage (in_filename);
  exposureStart = *in_exposureStart;
  writeExposureStart ();
}

Rts2Image::Rts2Image (int in_epoch_id, int in_targetId,
		      Rts2DevClientCamera * camera, int in_obsId,
		      const struct timeval *in_exposureStart)
{
  struct imghdr *im_h;
  long naxes = 1;
  char *filename;
  struct tm *expT;

  targetId = in_targetId;
  obsId = in_obsId;
  exposureStart = *in_exposureStart;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&filename, "%s/que/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (in_epoch_id), camera->getName (),
	    expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart.tv_usec / 1000);

  createImage (filename);

  writeExposureStart ();

  setValue ("TARGET", targetId, "target id");
  setValue ("OBSID", obsId, "observation id");
  setValue ("EPOCH", in_epoch_id, "image epoch of observation");
  setValue ("PROCES", 0, "no processing done");
}

Rts2Image::Rts2Image (char *in_filename)
{
  createImage (in_filename);
  // get info..
  getValue ("TARGET", targetId, NULL);
  getValue ("OBSID", obsId, NULL);
  getValue ("CTIME", exposureStart.tv_sec, NULL);
  getValue ("USEC", exposureStart.tv_usec, NULL);
}

Rts2Image::~Rts2Image (void)
{
  if (flags & IMAGE_SAVE)
    {
      fits_close_file (ffile, &fits_status);
    }
}

void
Rts2Image::createImage (char *in_filename)
{
  long naxes = 1;
  int ret;

  fits_status = 0;
  flags = IMAGE_NOT_SAVE;
  ffile = NULL;

  // make path for us..
  ret = mkpath (in_filename, 0777);
  if (ret)
    return;
  fits_create_file (&ffile, in_filename, &fits_status);
  fits_create_img (ffile, USHORT_IMG, 1, &naxes, &fits_status);
  if (fits_status)
    {
      return;
    }
  flags = IMAGE_SAVE;
}

void
Rts2Image::writeExposureStart ()
{
  time_t t = exposureStart.tv_sec;
  setValue ("CTIME", exposureStart.tv_sec,
	    "exposure start (seconds since 1.1.1970)");
  setValue ("USEC", exposureStart.tv_usec, "exposure start micro seconds");
  setValue ("JD", ln_get_julian_from_timet (&t), "exposure JD");
}

int
Rts2Image::setValue (char *name, int value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TINT, name, &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, long value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TLONG, name, &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, double value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TDOUBLE, name, &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, const char *value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TSTRING, name, (void *) value, comment,
		   &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValue (char *name, int &value, char *comment)
{
  if (!ffile)
    return -1;
  fits_read_key (ffile, TINT, name, (void *) &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValue (char *name, long &value, char *comment)
{
  if (!ffile)
    return -1;
  fits_read_key (ffile, TLONG, name, (void *) &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValue (char *name, double &value, char *comment)
{
  if (!ffile)
    return -1;
  fits_read_key (ffile, TDOUBLE, name, (void *) &value, comment,
		 &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValue (char *name, char *value, char *comment)
{
  if (!ffile)
    return -1;
  fits_read_key (ffile, TSTRING, name, (void *) &value, comment,
		 &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::writeImgHeader (struct imghdr *im_h)
{
  if (!ffile)
    return 0;
  setValue ("X", im_h->x, "detector X coordinate");
  setValue ("Y", im_h->y, "detector Y coordinate");
  setValue ("BINN_V", im_h->binnings[0], "X axis binning");
  setValue ("BINN_H", im_h->binnings[1], "Y axis binning");
  setValue ("FILTER", im_h->filter, "filter used for image");
  setValue ("SHUTTER", im_h->shutter,
	    "shutter state (1 - open, 2 - closed, 3 - synchro)");
  return 0;
}

int
Rts2Image::writeDate (Rts2ClientTCPDataConn * dataConn)
{
  struct imghdr *im_h;

  if (!ffile)
    return 0;

  im_h = dataConn->getImageHeader ();
  // we have to copy data to FITS anyway, so let's do it right now..
  if (im_h->naxes < 0 || im_h->naxes > 5)
    {
      return -1;
    }
  fits_resize_img (ffile, TUSHORT, im_h->naxes, im_h->sizes, &fits_status);
  fits_write_img (ffile, TUSHORT, 1, dataConn->getSize () / 2,
		  dataConn->getData (), &fits_status);
  return writeImgHeader (im_h);
}
