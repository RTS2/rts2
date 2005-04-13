#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/libnova.h>

#include "rts2image.h"
#include "imghdr.h"

Rts2Image::Rts2Image (char *in_filename, struct timeval *exposureStart)
{
  struct imghdr *im_h;

  fits_status = 0;
  flags = IMAGE_NOT_SAVE;
  // make filename
  fits_create_file (&ffile, in_filename, &fits_status);
  if (fits_status)
    {
      return;
    }
  // write exposure
  setValue ("CTIME", exposureStart->tv_sec,
	    "exposure start (seconds since 1.1.1970)");
  setValue ("USEC", exposureStart->tv_usec, "exposure start micro seconds");
  setValue ("JD", ln_get_julian_from_timet (&exposureStart->tv_sec),
	    "exposure JD");
  data = dataConn->getData ();
  // we have to copy data to FITS anyway, so let's do it right now..

  im_h = dataConn->getImageHeader ();
  if (im_h->naxes < 0 || im_h->naxes > 5)
    {
      return;
    }
  fits_create_img (&ffile, USHORT_IMG, 2, im_h->sizes, &status);
  writeImgHeader (im_h);
}

Rts2Image::Rts2Image (char *in_filename)
{
  fits_status = 0;
}

Rts2Image::~Rts2Image (void)
{
  if (flags & IMAGE_SAVE)
    {
      fits_close_file (&ffile, &fits_status);
    }
  free (filename);
}

void
Rts2Image::setValue (char *name, int value, char *comment)
{
  fits_update_key (&ffile, TINT, name, &value, comment, &fits_status);
}

void
Rts2Image::setValue (char *name, long value, char *comment)
{
  fits_update_key (&ffile, TLONG, name, &value, comment, &fits_status);
}

void
Rts2Image::setValue (char *name, double value, char *comment)
{
  fits_update_key (&ffile, TDOUBLE, name, &value, comment, &fits_status);
}

void
Rts2Image::setValue (char *name, char *value, char *comment)
{
  fits_update_key (&ffile, TSTRING, name, &value, comment, &fits_status);
}

/**void
Rts2Image::createFilename (struct timeval *exposureStart)
{
  char *filename;
  struct tm *expT;
  expT = gmtime (exposureStart->tv_sec);
  asprintf (&filename, "/images/%03i/%08i/%04i%02i%02i%02i%02i%02i-%4i", target, 
      expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_hour, expT->tm_min, expT->tm_sec, exposureStart->tv_usec / 1000);
}*/

void
Rts2Image::writeImgHeader (struct imghdr *im_h)
{
  setValue ("X", imghdr->x, "detector X coordinate");
  setValue ("Y", imghdr->y, "detector Y coordinate");
  setValue ("BINN_V", imghdr->binnings[0], "X axis binning");
  setValue ("BINN_H", imghdr->binnings[1], "Y axis binning");
  setValue ("FILTER", imghdr->filter, "filter used for image");
  setValue ("SHUTTER", imghdr->shutter,
	    "shutter state (1 - open, 2 - closed, 3 - synchro)");
}
