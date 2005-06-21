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
  imageName = NULL;
  ffile = NULL;
  cameraName = NULL;

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
  char *in_filename;
  struct tm *expT;

  imageName = NULL;
  ffile = NULL;

  epochId = in_epoch_id;
  targetId = in_targetId;
  obsId = in_obsId;
  exposureStart = *in_exposureStart;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&in_filename, "%s/que/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (in_epoch_id), camera->getName (),
	    expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart.tv_usec / 1000);

  createImage (in_filename);

  free (in_filename);

  writeExposureStart ();

  setValue ("EPOCH", epochId, "image epoch of observation");
  setValue ("TARGET", targetId, "target id");
  setValue ("OBSID", obsId, "observation id");
  setValue ("PROCES", 0, "no processing done");

  cameraName = new char[DEVICE_NAME_SIZE + 1];
  setValue ("CCD_NAME", camera->getName (), "camera name");
  strcpy (cameraName, camera->getName ());
}

Rts2Image::Rts2Image (char *in_filename)
{
  imageName = NULL;
  ffile = NULL;

  openImage (in_filename);
  // get info..
  getValue ("EPOCH", epochId);
  getValue ("TARGET", targetId);
  getValue ("OBSID", obsId);
  getValue ("CTIME", exposureStart.tv_sec);
  getValue ("USEC", exposureStart.tv_usec);
  cameraName = new char[DEVICE_NAME_SIZE + 1];
  getValue ("CCD_NAME", cameraName);
}

Rts2Image::~Rts2Image (void)
{
  saveImage ();
  if (imageName)
    delete imageName;
  if (cameraName)
    delete cameraName;
}

void
Rts2Image::setImageName (char *in_filename)
{
  if (imageName)
    delete imageName;

  imageName = new char[strlen (in_filename) + 1];
  strcpy (imageName, in_filename);
}

int
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
    return -1;
  fits_create_file (&ffile, in_filename, &fits_status);
  fits_create_img (ffile, USHORT_IMG, 1, &naxes, &fits_status);
  if (fits_status)
    {
      fits_report_error (stderr, fits_status);
      return -1;
    }
  setImageName (in_filename);

  flags = IMAGE_SAVE;
  return 0;
}

int
Rts2Image::openImage (char *in_filename)
{
  fits_status = 0;

  fits_open_file (&ffile, in_filename, READWRITE, &fits_status);
  if (fits_status)
    {
      fits_report_error (stderr, fits_status);
      return -1;
    }

  setImageName (in_filename);

  flags = IMAGE_SAVE;

  return 0;
}

int
Rts2Image::toQue ()
{
  char *new_filename;
  int ret = 0;
  struct tm *expT;

  if (!imageName)
    return -1;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&new_filename, "%s/que/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), cameraName, expT->tm_year + 1900,
	    expT->tm_mon + 1, expT->tm_mday, expT->tm_hour, expT->tm_min,
	    expT->tm_sec, exposureStart.tv_usec / 1000);

  ret = renameImage (new_filename);

  free (new_filename);
  return ret;
}

int
Rts2Image::toAcquisition ()
{
  char *new_filename;
  int ret = 0;
  struct tm *expT;

  if (!imageName)
    return -1;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&new_filename,
	    "%s/acqusition/%05i/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), targetId, cameraName,
	    expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart.tv_usec / 1000);

  ret = renameImage (new_filename);

  free (new_filename);
  return ret;
}

int
Rts2Image::toArchive ()
{
  char *new_filename;
  int ret = 0;
  struct tm *expT;

  if (!imageName)
    return -1;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&new_filename,
	    "%s/archive/%05i/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), targetId, cameraName,
	    expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart.tv_usec / 1000);

  ret = renameImage (new_filename);

  free (new_filename);
  return ret;
}

int
Rts2Image::toTrash ()
{
  char *new_filename;
  int ret = 0;
  struct tm *expT;

  if (!imageName)
    return -1;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&new_filename,
	    "%s/trash/%05i/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), targetId, cameraName,
	    expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart.tv_usec / 1000);

  ret = renameImage (new_filename);

  free (new_filename);
  return ret;
}

int
Rts2Image::renameImage (char *new_filename)
{
  int ret = 0;
  if (strcmp (new_filename, imageName))
    {
      saveImage ();
      ret = rename (imageName, new_filename);
      if (!ret)
	{
	  delete imageName;
	  openImage (new_filename);
	}
    }
  return ret;
}


int
Rts2Image::writeExposureStart ()
{
  time_t t = exposureStart.tv_sec;
  setValue ("CTIME", exposureStart.tv_sec,
	    "exposure start (seconds since 1.1.1970)");
  setValue ("USEC", exposureStart.tv_usec, "exposure start micro seconds");
  setValue ("JD", ln_get_julian_from_timet (&t), "exposure JD");
  return 0;
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

int
Rts2Image::saveImage ()
{
  if (flags & IMAGE_SAVE)
    {
      fits_close_file (ffile, &fits_status);
    }
  ffile = NULL;
}
