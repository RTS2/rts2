#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/libnova.h>
#include <malloc.h>

#include "rts2image.h"
#include "imghdr.h"

#include "../utils/mkpath.h"
#include "../utilsdb/target.h"

void
Rts2Image::initData ()
{
  imageName = NULL;
  ffile = NULL;
  cameraName = NULL;
  mountName = NULL;
  focName = NULL;
  imageData = NULL;
  filter = -1;
  mean = 0;
  imageData = NULL;
  sexResults = NULL;
  sexResultNum = 0;
  focPos = -1;
  signalNoise = 17;
  getFailed = 0;
}

Rts2Image::Rts2Image (char *in_filename,
		      const struct timeval *in_exposureStart)
{
  initData ();

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

  initData ();

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
  setValue ("TARGET", getTargetId (), "target id");
  setValue ("OBSID", obsId, "observation id");
  setValue ("IMGID", imgId, "image id");
  setValue ("PROCES", 0, "no processing done");

  cameraName = new char[DEVICE_NAME_SIZE + 1];
  setValue ("CCD_NAME", camera->getName (), "camera name");
  strcpy (cameraName, camera->getName ());

  mountName = NULL;
  focName = NULL;
}

Rts2Image::Rts2Image (const char *in_filename)
{
  int ret;

  initData ();

  openImage (in_filename);
  // get info..
  getValue ("EPOCH_ID", epochId);
  getValue ("TARGET", targetId);
  getValue ("OBSID", obsId);
  getValue ("IMGID", imgId);
  getValue ("CTIME", exposureStart.tv_sec);
  getValue ("USEC", exposureStart.tv_usec);
  ret = getValues ("NAXIS", naxis, 2);
  if (ret)
    {
      naxis[0] = 0;
      naxis[1] = 0;
    }
  cameraName = new char[DEVICE_NAME_SIZE + 1];
  ret = getValue ("CCD_NAME", cameraName);
  if (ret)
    strcpy (cameraName, "UNK");
  mountName = new char[DEVICE_NAME_SIZE + 1];
  ret = getValue ("MNT_NAME", mountName);
  if (ret)
    strcpy (mountName, "UNK");
  focName = new char[DEVICE_NAME_SIZE + 1];
  ret = getValue ("FOC_NAME", focName);
  if (ret)
    strcpy (focName, "UNK");
  ret = getValue ("FOC_POS", focPos);
  if (ret)
    focPos = -1;
  getValue ("CAM_FILT", filter);
  getValue ("MEAN", mean);
  getValueImageType ();
}

Rts2Image::~Rts2Image (void)
{
  saveImage ();
  if (imageName)
    delete[]imageName;
  if (cameraName)
    delete[]cameraName;
  if (mountName)
    delete[]mountName;
  if (focName)
    delete[]focName;
  if (imageData)
    delete[]imageData;
  if (sexResults)
    free (sexResults);
}

void
Rts2Image::setImageName (const char *in_filename)
{
  if (imageName)
    delete[]imageName;

  imageName = new char[strlen (in_filename) + 1];
  // ignore special fitsion names..
  if (*in_filename == '!')
    in_filename++;
  strcpy (imageName, in_filename);
}

int
Rts2Image::createImage (char *in_filename)
{
  int ret;

  fits_status = 0;
  flags = IMAGE_NOT_SAVE;
  imageType = IMGTYPE_UNKNOW;
  ffile = NULL;

  setImageName (in_filename);
  // make path for us..
  ret = mkpath (getImageName (), 0777);
  if (ret)
    return -1;
  naxis[0] = 1;
  naxis[1] = 1;
  fits_create_file (&ffile, in_filename, &fits_status);
  fits_create_img (ffile, USHORT_IMG, 2, naxis, &fits_status);
  syslog (LOG_DEBUG, "Rts2Image::createImage %p %s", this, in_filename);
  if (fits_status)
    {
      fits_report_error (stderr, fits_status);
      return -1;
    }

  flags = IMAGE_SAVE;
  return 0;
}

int
Rts2Image::openImage (const char *in_filename)
{
  fits_status = 0;

  setImageName (in_filename);

  fits_open_diskfile (&ffile, imageName, READWRITE, &fits_status);
  if (fits_status)
    {
      fits_report_error (stderr, fits_status);
      return -1;
    }

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
	    getImageBase (epochId), getTargetId (), cameraName,
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
	    getImageBase (epochId), getTargetId (), cameraName,
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
		getImageBase (epochId), getTargetId (), cameraName,
		expT->tm_year + 1900, expT->tm_mon + 1, expT->tm_mday,
		expT->tm_hour, expT->tm_min, expT->tm_sec,
		exposureStart.tv_usec / 1000);
    }
  ret = renameImage (new_filename);

  free (new_filename);
  return ret;
}

int
Rts2Image::toFlat ()
{
  char *new_filename;
  int ret = 0;
  struct tm *expT;

  if (!imageName)
    return -1;

  expT = gmtime (&exposureStart.tv_sec);
  asprintf (&new_filename,
	    "%s/flat/%s/%04i%02i%02i%02i%02i%02i-%04i.fits",
	    getImageBase (epochId), cameraName,
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
	    getImageBase (epochId), getTargetId (), cameraName,
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
  flags |= IMAGE_SAVE;
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, long value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TLONG, name, &value, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, double value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TDOUBLE, name, &value, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusValue (name);
}

int
Rts2Image::setValue (char *name, const char *value, char *comment)
{
  if (!ffile)
    return -1;
  fits_update_key (ffile, TSTRING, name, (void *) value, comment,
		   &fits_status);
  flags |= IMAGE_SAVE;
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
Rts2Image::getValue (char *name, float &value, char *comment)
{
  if (!ffile)
    return -1;
  fits_read_key (ffile, TFLOAT, name, (void *) &value, comment, &fits_status);
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
  setValue ("CAM_FILT", im_h->filter, "filter used for image");
  filter = im_h->filter;
  setValue ("SHUTTER", im_h->shutter,
	    "shutter state (1 - open, 2 - closed, 3 - synchro)");
  setValueImageType (im_h->shutter);
  // dark images don't need to wait till imgprocess will pick them up for reprocessing
  if (imageType == IMGTYPE_DARK)
    {
      return toDark ();
    }
  if (imageType == IMGTYPE_FLAT)
    {
      return toFlat ();
    }
  return 0;
}

int
Rts2Image::writeDate (Rts2ClientTCPDataConn * dataConn)
{
  struct imghdr *im_h;
  unsigned short *pixel;
  unsigned short *top;

  if (!ffile)
    return 0;

  mean = 0;

  im_h = dataConn->getImageHeader ();
  // we have to copy data to FITS anyway, so let's do it right now..
  if (im_h->naxes != 2)
    {
      syslog (LOG_ERR, "Rts2Image::writeDate not 2D image %i", im_h->naxes);
      return -1;
    }
  flags |= IMAGE_SAVE;
  naxis[0] = im_h->sizes[0];
  naxis[1] = im_h->sizes[1];
  fits_resize_img (ffile, USHORT_IMG, im_h->naxes, naxis, &fits_status);
  fits_write_img (ffile, USHORT_IMG, 1, dataConn->getSize () / 2,
		  dataConn->getData (), &fits_status);
  if (imageData)
    delete[]imageData;
  if (flags & IMAGE_KEEP_DATA)
    {
      imageData = new unsigned short[dataConn->getSize ()];
      memcpy (imageData, dataConn->getData (), dataConn->getSize ());
    }
  else
    {
      imageData = NULL;
    }
  if (fits_status)
    {
      fits_report_error (stderr, fits_status);
      return -1;
    }
  // calculate mean..
  pixel = dataConn->getData ();
  top = dataConn->getTop ();
  while (pixel < top)
    {
      mean += *pixel;
      pixel++;
    }
  if ((dataConn->getSize () / 2) > 0)
    mean /= dataConn->getSize () / 2;
  else
    mean = 0;
  setValue ("AVERAGE", mean, "average value of image");
  return writeImgHeader (im_h);
}

int
Rts2Image::fitsStatusValue (char *valname)
{
  int ret = 0;
  if (fits_status)
    {
      ret = -1;
      fprintf (stderr, "error when setting value '%s'\n", valname);
      fits_report_error (stderr, fits_status);
    }
  fits_status = 0;
  return ret;
}

int
Rts2Image::saveImage ()
{
  if (flags & IMAGE_SAVE && ffile)
    {
      fits_close_file (ffile, &fits_status);
      flags &= !IMAGE_SAVE;
    }
  ffile = NULL;
}

int
Rts2Image::deleteImage ()
{
  int ret;
  fits_close_file (ffile, &fits_status);
  ffile = NULL;
  flags &= !IMAGE_SAVE;
  ret = unlink (getImageName ());
}

void
Rts2Image::setMountName (const char *in_mountName)
{
  if (mountName)
    delete[]mountName;
  mountName = new char[strlen (in_mountName) + 1];
  strcpy (mountName, in_mountName);
  setValue ("MNT_NAME", mountName, "name of mount");
}

void
Rts2Image::setFocuserName (const char *in_focuserName)
{
  if (focName)
    delete[]focName;
  focName = new char[strlen (in_focuserName) + 1];
  strcpy (focName, in_focuserName);
  setValue ("FOC_NAME", focName, "name of focuser");
}

unsigned short *
Rts2Image::getDataUShortInt ()
{
  int ret;
  if (imageData)
    return imageData;
  int nullVal = 0;
  int anyNull = 0;
  imageData = new unsigned short[getWidth () * getHeight ()];
  if (!ffile)
    {
      ret = openImage (getImageName ());
      if (ret)
	return NULL;
    }

  fits_status =
    fits_read_img (ffile, USHORT_IMG, 1, getWidth () * getHeight (), &nullVal,
		   imageData, &anyNull, &fits_status);
  fitsStatusValue ("image");
  return imageData;
}

int
Rts2Image::substractDark (Rts2Image * darkImage)
{
  unsigned short *img_data;
  unsigned short *dark_data;
  if (getWidth () != darkImage->getWidth ()
      || getHeight () != darkImage->getHeight ())
    return -1;
  img_data = getDataUShortInt ();
  dark_data = darkImage->getDataUShortInt ();
  for (long i = 0; i < (getWidth () * getHeight ());
       i++, img_data++, dark_data++)
    {
      if (*img_data <= *dark_data)
	*img_data = 0;
      else
	*img_data = *img_data - *dark_data;
    }
  return 0;
}

int
Rts2Image::setAstroResults (double ra, double dec, double ra_err,
			    double dec_err)
{
  struct ln_equ_posn pos1;
  struct ln_equ_posn pos2;
  pos1.ra = ra;
  pos1.dec = dec;
  pos2.ra = ra + ra_err;
  pos2.dec = dec + dec_err;
  return setValue ("POS_ERR", ln_get_angular_separation (&pos1, &pos2),
		   "error in position");
}

int
Rts2Image::addStarData (struct stardata *sr)
{
  int ret;

  if (sr->F <= 0)		// flux is not significant..
    return -1;

  sexResultNum++;
  if (sexResultNum > 1)
    {
      sexResults =
	(struct stardata *) realloc ((void *) sexResults,
				     sexResultNum * sizeof (struct stardata));
    }
  else
    {
      sexResults = (struct stardata *) malloc (sizeof (struct stardata));
    }
  sexResults[sexResultNum - 1] = *sr;
  return 0;
}

static int
sdcompare (struct stardata *x1, struct stardata *x2)
{
  if (x1->fwhm < x2->fwhm)
    return 1;
  if (x1->fwhm > x2->fwhm)
    return -1;
  return 0;
}

int
Rts2Image::isGoodForFwhm (struct stardata *sr)
{
  return (sr->flags == 0 && sr->F > signalNoise * sr->Fe);
}

double
Rts2Image::getFWHM ()
{
  double avg;
  struct stardata *sr;
  int i;
  if (sexResultNum < 4)
    return nan ("f");

  // qsort (sexResults, sexResultNum, sizeof (struct stardata), sdcompare);
  // get average
  avg = 0;
  sr = sexResults;
  for (i = 0; i < sexResultNum; i++, sr++)
    {
      if (isGoodForFwhm (sr))
	avg += sr->fwhm;
    }
  avg /= sexResultNum;
  return avg;
}

static int
shortcompare (const void *val1, const void *val2)
{
  return (*(unsigned short *) val1 < *(unsigned short *) val2) ? -1 :
    ((*(unsigned short *) val1 == *(unsigned short *) val2) ? 0 : 1);
}

unsigned short
getShortMean (unsigned short *meanData, int sub)
{
  qsort (meanData, sub, sizeof (unsigned short), shortcompare);
  if (sub % 2)
    return meanData[sub / 2];
  return (meanData[(sub / 2) - 1] + meanData[(sub / 2)]) / 2;
}

int
Rts2Image::getCenterRow (long row, int row_width, double &x)
{
  unsigned short *data;
  data = getDataUShortInt ();
  unsigned short *rowData;
  unsigned short *meanData;
  unsigned short max;
  long i;
  long j;
  long k;
  long maxI;
  if (!data)
    return -1;
  // compute mean collumn
  rowData = new unsigned short[getWidth ()];
  meanData = new unsigned short[row_width];
  data += row * getWidth ();
  if (row_width < 2)
    {
      return -1;
    }
  for (j = 0; j < getWidth (); j++)
    {
      for (i = 0; i < row_width; i++)
	{
	  meanData[i] = data[j + i * getWidth ()];
	}
      rowData[j] = getShortMean (meanData, row_width);
    }
  // bin data in row..
  i = j = k = 0;
  while (j < getWidth ())
    {
      // last bit..
      k += row_width;
      rowData[i] =
	getShortMean (rowData + j,
		      (k > getWidth ())? (k - getWidth ()) : row_width);
      i++;
      j = k;
    }
  // decide, which part of slope we are in
  // i hold size of reduced rowData array
  // find maximum..
  max = rowData[0];
  maxI = 0;
  for (j = 1; j < i; j++)
    {
      if (rowData[j] > max)
	{
	  max = rowData[j];
	  maxI = j;
	}
    }
  // close to left border - left
  if (maxI < (i / 10))
    x = -1;
  // close to right border - right
  else if (maxI > (i - (i / 10)))
    x = getWidth () + 1;
  // take value in middle
  else
    x = (maxI * row_width) + row_width / 2;
  delete[]rowData;
  delete[]meanData;
  return 0;
}

int
Rts2Image::getCenterCol (long col, int col_width, double &y)
{
  unsigned short *data;
  data = getDataUShortInt ();
  unsigned short *colData;
  unsigned short *meanData;
  unsigned short max;
  long i;
  long j;
  long k;
  long maxI;
  if (!data)
    return -1;
  // smooth image..
  colData = new unsigned short[getHeight ()];
  meanData = new unsigned short[col_width];
  data += col * getHeight ();
  if (col_width < 2)
    {
      // special handling..
      return -1;
    }
  for (j = 0; j < getHeight (); j++)
    {
      for (i = 0; i < col_width; i++)
	{
	  meanData[i] = data[j + i * getHeight ()];
	}
      colData[j] = getShortMean (meanData, col_width);
    }
  // bin data in row..
  i = j = k = 0;
  while (j < getHeight ())
    {
      // last bit..
      k += col_width;
      colData[i] =
	getShortMean (colData + j,
		      (k > getHeight ())? (k - getHeight ()) : col_width);
      i++;
      j = k;
    }
  // find gauss on image..
  //ret = findGauss (colData, i, y);
  maxI = 0;
  for (j = 1; j < i; j++)
    {
      if (colData[j] > max)
	{
	  max = colData[j];
	  maxI = j;
	}
    }
  // close to left border - left
  if (maxI < (i / 10))
    y = -1;
  // close to right border - right
  else if (maxI > (i - (i / 10)))
    y = getHeight () + 1;
  // take value in middle
  else
    y = (maxI * col_width) + col_width / 2;
  delete[]colData;
  delete[]meanData;
  return 0;
}

int
Rts2Image::getCenter (double &x, double &y, int bin)
{
  int ret;
  long i;
  i = 0;
  while ((i + bin) < getHeight ())
    {
      ret = getCenterRow (i, bin, x);
      if (ret)
	return -1;
      if (x > 0 && x < getWidth ())
	break;
      i += bin;
    }
  i = 0;
  while ((i + bin) < getWidth ())
    {
      ret = getCenterCol (i, bin, y);
      if (ret)
	return -1;
      if (y > 0 && y < getHeight ())
	break;
      i += bin;
    }
  return 0;
}
