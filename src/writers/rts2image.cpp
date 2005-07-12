#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/libnova.h>

#include "rts2image.h"
#include "imghdr.h"

#include "../utils/mkpath.h"
#include "../utilsdb/target.h"

Rts2Image::Rts2Image (char *in_filename,
		      const struct timeval *in_exposureStart)
{
  imageName = NULL;
  ffile = NULL;
  cameraName = NULL;
  mountName = NULL;

  createImage (in_filename);
  exposureStart = *in_exposureStart;
  writeExposureStart ();
}

Rts2Image::Rts2Image (int in_epoch_id, int in_targetId,
		      Rts2DevClientCamera * camera, int in_obsId,
		      const struct timeval *in_exposureStart, int in_imgId)
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
  imgId = in_imgId;
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

  setValue ("EPOCH_ID", epochId, "image epoch ID of observation");
  setValue ("TARGET", targetId, "target id");
  setValue ("OBSID", obsId, "observation id");
  setValue ("IMGID", imgId, "image id");
  setValue ("PROCES", 0, "no processing done");

  cameraName = new char[DEVICE_NAME_SIZE + 1];
  setValue ("CCD_NAME", camera->getName (), "camera name");
  strcpy (cameraName, camera->getName ());

  mountName = NULL;
}

Rts2Image::Rts2Image (char *in_filename)
{
  imageName = NULL;
  ffile = NULL;

  openImage (in_filename);
  // get info..
  getValue ("EPOCH_ID", epochId);
  getValue ("TARGET", targetId);
  getValue ("OBSID", obsId);
  getValue ("IMGID", imgId);
  getValue ("CTIME", exposureStart.tv_sec);
  getValue ("USEC", exposureStart.tv_usec);
  cameraName = new char[DEVICE_NAME_SIZE + 1];
  getValue ("CCD_NAME", cameraName);
  mountName = new char[DEVICE_NAME_SIZE + 1];
  getValue ("MNT_NAME", mountName);
  getValueImageType ();
}

Rts2Image::~Rts2Image (void)
{
  saveImage ();
  if (imageName)
    delete imageName;
  if (cameraName)
    delete cameraName;
  if (mountName)
    delete mountName;
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
  imageType = IMGTYPE_UNKNOW;
  ffile = NULL;

  // make path for us..
  ret = mkpath (in_filename, 0777);
  if (ret)
    return -1;
  fits_create_file (&ffile, in_filename, &fits_status);
  fits_create_img (ffile, USHORT_IMG, 1, &naxes, &fits_status);
  syslog (LOG_DEBUG, "Rts2Image::createImage %p %s", this, in_filename);
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

  fits_open_diskfile (&ffile, in_filename, READWRITE, &fits_status);
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
	    "%s/archive/%05i/%s/object/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), targetId, cameraName,
	    expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
	    expT->tm_hour, expT->tm_min, expT->tm_sec,
	    exposureStart.tv_usec / 1000);

  ret = renameImage (new_filename);

  free (new_filename);
  return ret;
}

// move to dark images area..
int
Rts2Image::toDark ()
{
  char *new_filename;
  int ret = 0;
  struct tm *expT;

  if (!imageName)
    return -1;

  expT = gmtime (&exposureStart.tv_sec);
  if (getTargetId () == TARGET_DARK)
    {
      asprintf (&new_filename,
		"%s/darks/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
		getImageBase (epochId), cameraName,
		expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
		expT->tm_hour, expT->tm_min, expT->tm_sec,
		exposureStart.tv_usec / 1000);
    }
  else
    {
      asprintf (&new_filename,
		"%s/archive/%05i/%s/darks/%04i%02i%02i%02i%02i%02i-%04i.fits",
		getImageBase (epochId), targetId, cameraName,
		expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
		expT->tm_hour, expT->tm_min, expT->tm_sec,
		exposureStart.tv_usec / 1000);
    }
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
	    "%s/trash/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), cameraName,
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
      ret = mkpath (new_filename, 0777);
      if (ret)
	return ret;
      ret = rename (imageName, new_filename);
      if (!ret)
	{
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

char *
Rts2Image::getImageBase (int in_epoch_id)
{
  static char buf[6];
  sprintf (buf, "/images/%03i", in_epoch_id);
  return buf;
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
Rts2Image::setValueImageType (int shutter_state)
{
  char *imgTypeText;
  // guess image type..
  switch (getTargetId ())
    {
    case TARGET_DARK:
      imageType = IMGTYPE_DARK;
      break;
    case TARGET_FLAT:
      switch (shutter_state)
	{
	case 1:
	  imageType = IMGTYPE_FLAT;
	  break;
	case 2:
	  imageType = IMGTYPE_DARK;
	  break;
	}
    default:
      switch (shutter_state)
	{
	case 1:
	case 3:
	  imageType = IMGTYPE_OBJECT;
	  break;
	case 2:
	  imageType = IMGTYPE_DARK;
	  break;
	}
    }
  switch (imageType)
    {
    case IMGTYPE_DARK:
      imgTypeText = "dark";
      break;
    case IMGTYPE_FLAT:
      imgTypeText = "flat";
      break;
    case IMGTYPE_OBJECT:
      imgTypeText = "object";
      break;
    case IMGTYPE_ZERO:
      imgTypeText = "zero";
      break;
    case IMGTYPE_COMP:
      imgTypeText = "comp";
      break;
    case IMGTYPE_UNKNOW:
    default:
      imgTypeText = "unknow";
      break;
    }
  return setValue ("IMAGETYP", imgTypeText, "IRAF based image type");
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
  fits_read_key (ffile, TSTRING, name, (void *) value, comment, &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValueImageType ()
{
  int ret;
  char value[20];
  ret = getValue ("IMAGETYP", value);
  if (ret)
    return ret;
  // switch based on IMAGETYPE
  if (!strcasecmp (value, "dark"))
    imageType = IMGTYPE_DARK;
  else if (!strcasecmp (value, "flat"))
    imageType = IMGTYPE_FLAT;
  else if (!strcasecmp (value, "object"))
    imageType = IMGTYPE_OBJECT;
  else if (!strcasecmp (value, "zero"))
    imageType = IMGTYPE_ZERO;
  else if (!strcasecmp (value, "comp"))
    imageType = IMGTYPE_COMP;
  else
    imageType = IMGTYPE_UNKNOW;
  return 0;
}

int
Rts2Image::getValues (char *name, int *values, int num, int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_log (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValues (char *name, long *values, int num, int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_lng (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValues (char *name, double *values, int num, int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_dbl (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::getValues (char *name, char **values, int num, int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_str (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusValue (name);
}

int
Rts2Image::writeImgHeader (struct imghdr *im_h)
{
  if (!ffile)
    return 0;
  setValue ("X", im_h->x, "image beginning - detector X coordinate");
  setValue ("Y", im_h->y, "image beginning - detector Y coordinate");
  setValue ("BIN_V", im_h->binnings[0], "X axis binning");
  setValue ("BIN_H", im_h->binnings[1], "Y axis binning");
  setValue ("FILTER", im_h->filter, "filter used for image");
  setValue ("SHUTTER", im_h->shutter,
	    "shutter state (1 - open, 2 - closed, 3 - synchro)");
  setValueImageType (im_h->shutter);
  // dark images don't need to wait till imgprocess will pick them up for reprocessing
  if (imageType == IMGTYPE_DARK)
    {
      return toDark ();
    }
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
  fits_resize_img (ffile, USHORT_IMG, im_h->naxes, im_h->sizes, &fits_status);
  fits_write_img (ffile, USHORT_IMG, 1, dataConn->getSize () / 2,
		  dataConn->getData (), &fits_status);
  if (fits_status)
    {
      fits_report_error (stderr, fits_status);
      return -1;
    }
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

void
Rts2Image::setMountName (const char *in_mountName)
{
  if (mountName)
    delete mountName;
  mountName = new char[strlen (in_mountName) + 1];
  strcpy (mountName, in_mountName);
  setValue ("MNT_NAME", mountName, "name of mount");
}
