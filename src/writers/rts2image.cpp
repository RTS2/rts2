#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/libnova.h>

#include "rts2image.h"
#include "imghdr.h"

Rts2Image::Rts2Image (char *in_filename, const struct timeval *exposureStart)
{
  createImage (in_filename, exposureStart);
}

Rts2Image::Rts2Image (int epochId, int targetId, int obsId,
		      const struct timeval *exposureStart)
{
  struct imghdr *im_h;
  long naxes = 1;
  char *filename;
  struct tm *expT;

  expT = gmtime (&exposureStart->tv_sec);
  asprintf (&filename, "/images/%03i/%05i/%04i%02i%02i%02i%02i%02i-%4i",
	    epochId, targetId, expT->tm_year + 1900, expT->tm_mon + 1,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart->tv_usec / 1000);

  createImage (filename, exposureStart);
}

Rts2Image::Rts2Image (char *in_filename)
{
  struct timeval tm;
  gettimeofday (&tm, NULL);
  createImage (in_filename, &tm);
}

Rts2Image::~Rts2Image (void)
{
  if (flags & IMAGE_SAVE)
    {
      fits_close_file (ffile, &fits_status);
    }
}

void
Rts2Image::createImage (char *in_filename,
			const struct timeval *exposureStart)
{
  long naxes = 1;
  time_t t = exposureStart->tv_sec;

  fits_status = 0;
  flags = IMAGE_NOT_SAVE;
  // make filename
  fits_create_file (&ffile, in_filename, &fits_status);
  fits_create_img (ffile, USHORT_IMG, 1, &naxes, &fits_status);
  if (fits_status)
    {
      return;
    }
  // write exposure
  setValue ("CTIME", exposureStart->tv_sec,
	    "exposure start (seconds since 1.1.1970)");
  setValue ("USEC", exposureStart->tv_usec, "exposure start micro seconds");
  setValue ("JD", ln_get_julian_from_timet (&t), "exposure JD");
}

int
Rts2Image::setValue (char *name, int value, char *comment)
{
  fits_update_key (ffile, TINT, name, &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, long value, char *comment)
{
  fits_update_key (ffile, TLONG, name, &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, double value, char *comment)
{
  fits_update_key (ffile, TDOUBLE, name, &value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, const char *value, char *comment)
{
  fits_update_key (ffile, TSTRING, name, (void *) value, comment,
		   &fits_status);
  return fitsStatusValue (name);
}

/**void
Rts2Image::createFilename (struct timeval *exposureStart)
{
}*/

int
Rts2Image::writeImgHeader (struct imghdr *im_h)
{
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
