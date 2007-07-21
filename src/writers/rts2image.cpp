#include <libnova/libnova.h>
#include <malloc.h>

#include "rts2image.h"
#include "imghdr.h"

#include "../utils/mkpath.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2config.h"
#include "../utils/timestamp.h"
#include "../utilsdb/target.h"

#include "imgdisplay.h"

#include <iomanip>
#include <sstream>

void
Rts2Image::initData ()
{
  exposureLength = nan ("f");

  imageName = NULL;
  ffile = NULL;
  targetId = -1;
  targetIdSel = -1;
  targetType = TYPE_UNKNOW;
  targetName = NULL;
  epochId = -1;
  cameraName = NULL;
  mountName = NULL;
  focName = NULL;
  filter_i = -1;
  filter = NULL;
  average = 0;
  stdev = 0;
  bg_stdev = 0;
  min = max = mean = 0;
  histogram = NULL;
  imageData = NULL;
  imageType = RTS2_DATA_USHORT;
  sexResults = NULL;
  sexResultNum = 0;
  focPos = -1;
  signalNoise = 17;
  getFailed = 0;

  pos_astr.ra = nan ("f");
  pos_astr.dec = nan ("f");
  ra_err = nan ("f");
  dec_err = nan ("f");
  img_err = nan ("f");

  isAcquiring = 0;

  config_rotang = nan ("f");

  exposureStart.tv_sec = 0;
  exposureStart.tv_usec = 0;

  shutter = SHUT_UNKNOW;

  flags = 0;

  xoa = nan ("f");
  yoa = nan ("f");

  mnt_flip = 0;
}

Rts2Image::Rts2Image ()
{
  initData ();
  flags = IMAGE_KEEP_DATA;
}

Rts2Image::Rts2Image (Rts2Image * in_image)
{
  ffile = in_image->ffile;
  in_image->ffile = NULL;
  fits_status = in_image->fits_status;
  flags = in_image->flags;
  filter_i = in_image->filter_i;
  filter = in_image->filter;
  in_image->filter = NULL;
  setExposureStart (&in_image->exposureStart);
  exposureLength = in_image->exposureLength;
  imageData = in_image->imageData;
  imageType = in_image->imageType;
  in_image->imageData = NULL;
  focPos = in_image->focPos;
  naxis[0] = in_image->naxis[0];
  naxis[1] = in_image->naxis[1];
  signalNoise = in_image->signalNoise;
  getFailed = in_image->getFailed;
  average = in_image->average;
  stdev = in_image->stdev;
  bg_stdev = in_image->bg_stdev;
  min = in_image->min;
  max = in_image->max;
  mean = in_image->mean;
  histogram = in_image->histogram;
  in_image->histogram = NULL;
  isAcquiring = in_image->isAcquiring;
  config_rotang = in_image->config_rotang;

  epochId = in_image->epochId;
  targetId = in_image->targetId;
  targetIdSel = in_image->targetIdSel;
  targetType = in_image->targetType;
  targetName = in_image->targetName;
  in_image->targetName = NULL;
  obsId = in_image->obsId;
  imgId = in_image->imgId;
  cameraName = in_image->cameraName;
  in_image->cameraName = NULL;
  mountName = in_image->mountName;
  in_image->mountName = NULL;
  focName = in_image->focName;
  in_image->focName = NULL;
  imageName = in_image->imageName;
  in_image->imageName = NULL;

  pos_astr.ra = in_image->pos_astr.ra;
  pos_astr.dec = in_image->pos_astr.dec;
  ra_err = in_image->ra_err;
  dec_err = in_image->dec_err;
  img_err = in_image->img_err;

  sexResults = in_image->sexResults;
  in_image->sexResults = NULL;
  sexResultNum = in_image->sexResultNum;

  shutter = in_image->getShutter ();

  // other image will be saved!
  flags = in_image->flags;
  in_image->flags &= ~IMAGE_SAVE;

  xoa = in_image->xoa;
  yoa = in_image->yoa;

  mnt_flip = in_image->mnt_flip;
}

Rts2Image::Rts2Image (const struct timeval *in_exposureStart)
{
  initData ();
  flags = IMAGE_KEEP_DATA;
  setExposureStart (in_exposureStart);
  writeExposureStart ();
}

Rts2Image::Rts2Image (long in_img_date, int in_img_usec,
		      float in_img_exposure)
{
  initData ();
  setExposureStart (&in_img_date, in_img_usec);
  exposureLength = in_img_exposure;
}

Rts2Image::Rts2Image (char *in_filename,
		      const struct timeval *in_exposureStart)
{
  initData ();

  createImage (in_filename);
  setExposureStart (in_exposureStart);
  writeExposureStart ();
}

Rts2Image::Rts2Image (Rts2Target * currTarget, Rts2DevClientCamera * camera,
		      const struct timeval *in_exposureStart)
{
  std::string in_filename;

  initData ();

  epochId = currTarget->getEpoch ();
  targetId = currTarget->getTargetID ();
  targetIdSel = currTarget->getObsTargetID ();
  targetType = currTarget->getTargetType ();
  obsId = currTarget->getObsId ();
  imgId = currTarget->getNextImgId ();
  setExposureStart (in_exposureStart);

  cameraName = new char[DEVICE_NAME_SIZE + 1];
  strcpy (cameraName, camera->getName ());

  mountName = NULL;
  focName = NULL;

  isAcquiring = currTarget->isAcquiring ();
  if (isAcquiring)
    {
      // put acqusition images to acqusition que
      in_filename = expandPath ("%b/acqusition/que/%c/%f");
    }
  else
    {
      in_filename = expandPath ("%b/que/%c/%f");
    }

  createImage (in_filename);

  writeExposureStart ();

  setValue ("EPOCH_ID", epochId, "image epoch ID of observation");
  setValue ("TARGET", getTargetId (), "target id");
  setValue ("TARSEL", getTargetIdSel (), "selector target id");
  setValue ("TARTYPE", targetType, "target type");
  setValue ("OBSID", obsId, "observation id");
  setValue ("IMGID", imgId, "image id");
  setValue ("PROC", 0, "image processing status");

  if (currTarget->getTargetName ())
    {
      targetName = new char[strlen (currTarget->getTargetName ()) + 1];
      strcpy (targetName, currTarget->getTargetName ());
      setValue ("OBJECT", currTarget->getTargetName (), "target name");
    }
  else
    {
      setValue ("OBJECT", "(null)", "target name was null");
    }

  setValue ("CCD_NAME", camera->getName (), "camera name");

  currTarget->writeToImage (this);
}

Rts2Image::Rts2Image (const char *in_filename, bool verbose)
{
  int ret;

  initData ();

  openImage (in_filename);
  // get info..
  getValue ("EPOCH_ID", epochId, verbose);
  getValue ("TARGET", targetId, verbose);
  getValue ("TARSEL", targetIdSel, verbose);
  getValue ("TARTYPE", targetType, verbose);
  targetName = new char[FLEN_VALUE];
  getValue ("OBJECT", targetName, FLEN_VALUE, verbose);
  getValue ("OBSID", obsId, verbose);
  getValue ("IMGID", imgId, verbose);
  getValue ("CTIME", exposureStart.tv_sec, verbose);
  getValue ("USEC", exposureStart.tv_usec, verbose);
  setExposureStart ();
  // if EXPTIM fails..
  ret = getValue ("EXPTIME", exposureLength, false);
  if (ret)
    getValue ("EXPOSURE", exposureLength, verbose);
  ret = getValues ("NAXIS", naxis, 2, true);
  if (ret)
    {
      naxis[0] = 0;
      naxis[1] = 0;
    }
  cameraName = new char[DEVICE_NAME_SIZE + 1];
  ret = getValue ("CCD_NAME", cameraName, DEVICE_NAME_SIZE);
  if (ret)
    strcpy (cameraName, "UNK");
  mountName = new char[DEVICE_NAME_SIZE + 1];
  ret = getValue ("MNT_NAME", mountName, DEVICE_NAME_SIZE);
  if (ret)
    strcpy (mountName, "UNK");
  focName = new char[DEVICE_NAME_SIZE + 1];
  ret = getValue ("FOC_NAME", focName, DEVICE_NAME_SIZE);
  if (ret)
    strcpy (focName, "UNK");
  ret = getValue ("FOC_POS", focPos);
  if (ret)
    focPos = -1;
  getValue ("CAM_FILT", filter_i, verbose);
  filter = new char[5];
  getValue ("FILTER", filter, 5, verbose);
  getValue ("AVERAGE", average, verbose);
  getValue ("STDEV", stdev, false);
  getValue ("BGSTDEV", bg_stdev, false);
  getValue ("RA_ERR", ra_err, false);
  getValue ("DEC_ERR", dec_err, false);
  getValue ("POS_ERR", img_err, false);
  // astrometry get !!
  getValue ("CRVAL1", pos_astr.ra, false);
  getValue ("CRVAL2", pos_astr.dec, false);
  // xoa..
  xoa = yoa = 0;
  getValue ("CAM_XOA", xoa, false);
  getValue ("CAM_YOA", yoa, false);

  mnt_flip = 0;
  getValue ("MNT_FLIP", mnt_flip, false);
}

Rts2Image::~Rts2Image (void)
{
  saveImage ();
  delete[]imageName;
  delete[]targetName;
  delete[]cameraName;
  delete[]mountName;
  delete[]focName;
  if (!(flags & IMAGE_DONT_DELETE_DATA))
    delete imageData;
  imageData = NULL;
  imageType = RTS2_DATA_USHORT;
  delete[]filter;
  if (sexResults)
    free (sexResults);
}

std::string Rts2Image::expandPath (std::string expression)
{
  std::string ret = "";
  // FITS keyword cannot have more then 80 chars..
  char
    valueName[200];
  char *
    valueNameTop;
  char *
    valueNameEnd = valueName + 199;
  for (std::string::iterator iter = expression.begin ();
       iter != expression.end (); iter++)
    {
      switch (*iter)
	{
	case '%':
	  iter++;
	  if (iter != expression.end ())
	    {
	      switch (*iter)
		{
		case '%':
		  ret += '%';
		  break;
		case 'b':
		  ret += getImageBase ();
		  break;
		case 'c':
		  if (cameraName)
		    ret += cameraName;
		  else
		    ret += "NULL";
		  break;
		case 'D':
		  ret += getStartYDayString ();
		  break;
		case 'd':
		  ret += getStartDayString ();
		  break;
		case 'E':
		  ret += getExposureLengthString ();
		  break;
		case 'e':
		  ret += getEpochString ();
		  break;
		case 'F':
		  ret += getFilter ();
		  break;
		case 'f':
		  ret += getOnlyFileName ();
		  break;
		case 'H':
		  ret += getStartHourString ();
		  break;
		case 'i':
		  ret += getImgIdString ();
		  break;
		case 'M':
		  ret += getStartMinString ();
		  break;
		case 'm':
		  ret += getStartMonthString ();
		  break;
		case 'o':
		  ret += getObsString ();
		  break;
		case 'p':
		  ret += getProcessingString ();
		  break;
		case 'S':
		  ret += getStartSecString ();
		  break;
		case 's':
		  ret += getStartMSecString ();
		  break;
		case 'T':
		  ret += getTargetString ();
		  break;
		case 't':
		  ret += getTargetSelString ();
		  break;
		case 'y':
		  ret += getStartYearString ();
		  break;
		default:
		  ret += '%';
		  ret += *iter;
		}
	    }
	  break;
	  // that one enables to copy values from image header to expr
	case '$':
	  valueNameTop = valueName;
	  for (iter++;
	       (iter != expression.end ()) && (*iter != '$')
	       && valueNameTop < valueNameEnd; iter++, valueNameTop++)
	    *valueNameTop = *iter;
	  if (iter == expression.end () || valueNameTop >= valueNameEnd)
	    {
	      ret += '$';
	      ret += valueName;
	    }
	  else
	    {
	      char
		valB[200];
	      int
		g_ret;
	      *valueNameTop = '\0';
	      g_ret = getValue (valueName, valB, 200, true);
	      if (g_ret)
		{
		  ret += '$';
		  ret += valueName;
		  ret += '$';
		}
	      else
		{
		  ret += valB;
		}
	    }
	  break;
	default:
	  ret += *iter;
	}
    }
  return ret;
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
Rts2Image::createImage (std::string in_filename)
{
  int ret;

  fits_status = 0;
  flags = IMAGE_NOT_SAVE;
  shutter = SHUT_UNKNOW;
  ffile = NULL;

  setImageName (in_filename.c_str ());

  // make path for us..
  ret = mkpath (getImageName (), 0777);
  if (ret)
    return -1;
  naxis[0] = 1;
  naxis[1] = 1;
  fits_create_file (&ffile, getImageName (), &fits_status);
  fits_create_img (ffile, USHORT_IMG, 2, naxis, &fits_status);
  if (fits_status)
    {
      logStream (MESSAGE_ERROR) << "Rts2Image::createImage " <<
	getFitsErrors () << sendLog;
      return -1;
    }
  logStream (MESSAGE_DEBUG) << "Rts2Image::createImage " << this << " " <<
    getImageName () << sendLog;

  flags = IMAGE_SAVE;
  return 0;
}

int
Rts2Image::createImage (char *in_filename)
{
  int ret;

  fits_status = 0;
  flags = IMAGE_NOT_SAVE;
  shutter = SHUT_UNKNOW;
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
  if (fits_status)
    {
      logStream (MESSAGE_DEBUG) << "Rts2Image::createImage " <<
	getFitsErrors () << sendLog;
      return -1;
    }
  logStream (MESSAGE_DEBUG) << "Rts2Image::createImage " << in_filename <<
    sendLog;

  flags = IMAGE_SAVE;
  return 0;
}

int
Rts2Image::openImage ()
{
  if (imageName)
    return openImage (NULL);
  return -1;
}

int
Rts2Image::openImage (const char *in_filename)
{
  fits_status = 0;

  if (in_filename)
    setImageName (in_filename);

  closeFile ();

#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Opening " << getImageName () << " ffile " <<
    ffile << sendLog;
#endif /* DEBUG_EXTRA */

  fits_open_diskfile (&ffile, imageName, READWRITE, &fits_status);
  if (fits_status == FILE_NOT_OPENED)
    {
      fits_status = 0;
      fits_open_diskfile (&ffile, imageName, READONLY, &fits_status);
    }
  if (fits_status)
    {
      if (!(flags & IMAGE_CANNOT_LOAD))
	{
	  logStream (MESSAGE_ERROR) << "openImage: " << getFitsErrors () <<
	    sendLog;
	  flags |= IMAGE_CANNOT_LOAD;
	}
      return -1;
    }

  flags |= IMAGE_SAVE;

  return 0;
}

int
Rts2Image::toQue ()
{
  return renameImageExpand ("%b/que/%c/%f");
}

int
Rts2Image::toAcquisition ()
{
  return renameImageExpand ("%b/acqusition/%t/%c/%f");
}

int
Rts2Image::toArchive ()
{
  return renameImageExpand ("%b/archive/%t/%c/object/%f");
}

// move to dark images area..
int
Rts2Image::toDark ()
{
  if (getTargetId () == TARGET_DARK)
    {
      return renameImageExpand ("%b/darks/%c/%f");
    }
  // else..
  return renameImageExpand ("%b/archive/%t/%c/darks/%f");
}

int
Rts2Image::toFlat ()
{
  return renameImageExpand ("%b/flat/%c/raw/%f");
}

int
Rts2Image::toMasterFlat ()
{
  return renameImageExpand ("%b/flat/%c/master/%f");
}

int
Rts2Image::toTrash ()
{
  return renameImageExpand ("%b/trash/%t/%c/%f");
}

int
Rts2Image::renameImage (const char *new_filename)
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
Rts2Image::renameImageExpand (std::string new_ex)
{
  std::string new_filename;

  if (!imageName)
    return -1;

  new_filename = expandPath (new_ex);
  return renameImage (new_filename.c_str ());
}

int
Rts2Image::copyImage (const char *copy_filename)
{
  char *cp_filename = new char[strlen (copy_filename) + 1];
  strcpy (cp_filename, copy_filename);
  fitsfile *cp_file;
  fits_status = 0;

  int ret;
  ret = mkpath (cp_filename, 0777);
  if (ret)
    goto err;

  fits_create_file (&cp_file, cp_filename, &fits_status);
  fits_copy_file (ffile, cp_file, true, true, true, &fits_status);
  ret = setCreationDate (cp_file);
  if (ret)
    goto err;
  fits_close_file (cp_file, &fits_status);
  if (fits_status)
    ret = fits_status;
err:
  delete[]cp_filename;
  return ret;
}

int
Rts2Image::copyImageExpand (std::string copy_ex)
{
  std::string copy_filename;

  if (!imageName)
    return -1;

  copy_filename = expandPath (copy_ex);
  return copyImage (copy_filename.c_str ());
}

int
Rts2Image::saveImageData (const char *save_filename, unsigned short *in_data)
{
  fitsfile *fp;
  fits_status = 0;
  long fpixel = 1;

  fits_open_file (&fp, save_filename, READWRITE, &fits_status);
  fits_write_img (fp, TUSHORT, fpixel, naxis[0] * naxis[1], in_data,
		  &fits_status);
  fits_close_file (fp, &fits_status);

  return 0;
}

int
Rts2Image::writeExposureStart ()
{
  setValue ("CTIME", exposureStart.tv_sec,
	    "exposure start (seconds since 1.1.1970)");
  setValue ("USEC", exposureStart.tv_usec, "exposure start micro seconds");
  setValue ("JD", ln_get_julian_from_timet (&exposureStart.tv_sec),
	    "exposure JD");
  setValue ("DATE-OBS", &exposureStart.tv_sec, exposureStart.tv_usec,
	    "start of exposure");
  return 0;
}

char *
Rts2Image::getImageBase ()
{
  static char buf[12];
  sprintf (buf, "/images/%03i", epochId);
  return buf;
}

std::string Rts2Image::getFitsErrors ()
{
  std::ostringstream os;
  char
    buf[30];
  fits_get_errstatus (fits_status, buf);
  os << " file " << getImageName () << " " << buf;
  return os.str ();
}

int
Rts2Image::setValue (char *name, bool value, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_update_key (ffile, TLOGICAL, name, &value, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name, true);
}

int
Rts2Image::setValue (char *name, int value, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_update_key (ffile, TINT, name, &value, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name, true);
}

int
Rts2Image::setValue (char *name, long value, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_update_key (ffile, TLONG, name, &value, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name);
}

int
Rts2Image::setValue (char *name, float value, char *comment)
{
  int ret;
  float val = value;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  if (isnan (val) || isinf (val))
    val = FLOATNULLVALUE;
  fits_update_key (ffile, TFLOAT, name, &val, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name);
}

int
Rts2Image::setValue (char *name, double value, char *comment)
{
  int ret;
  double val = value;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  if (isnan (val) || isinf (val))
    val = DOUBLENULLVALUE;
  fits_update_key (ffile, TDOUBLE, name, &val, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name);
}

int
Rts2Image::setValue (char *name, char value, char *comment)
{
  char val[2];
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  val[0] = value;
  val[1] = '\0';
  fits_update_key (ffile, TSTRING, name, (void *) val, comment, &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name);
}

int
Rts2Image::setValue (char *name, const char *value, char *comment)
{
  int ret;
  // we will not save null values
  if (!value)
    return 0;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_update_key_longstr (ffile, name, (char *) value, comment,
			   &fits_status);
  flags |= IMAGE_SAVE;
  return fitsStatusSetValue (name);
}

int
Rts2Image::setValue (char *name, time_t * sec, long usec, char *comment)
{
  char buf[25];
  struct tm t_tm;
  gmtime_r (sec, &t_tm);
  strftime (buf, 25, "%Y-%m-%dT%H:%M:%S.", &t_tm);
  snprintf (buf + 20, 4, "%03li", usec / 1000);
  return setValue (name, buf, comment);
}

int
Rts2Image::setCreationDate (fitsfile * out_file)
{
  int ret;
  fitsfile *curr_ffile = ffile;

  if (out_file)
    {
      ffile = out_file;
    }

  struct timeval now;
  gettimeofday (&now, NULL);
  ret = setValue ("DATE", &(now.tv_sec), now.tv_usec, "creation date");

  if (out_file)
    {
      ffile = curr_ffile;
    }
  return ret;
}

int
Rts2Image::getValue (char *name, bool & value, bool required, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TLOGICAL, name, (void *) &value, comment,
		 &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, int &value, bool required, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TINT, name, (void *) &value, comment, &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, long &value, bool required, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TLONG, name, (void *) &value, comment, &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, float &value, bool required, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TFLOAT, name, (void *) &value, comment, &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, double &value, bool required, char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TDOUBLE, name, (void *) &value, comment,
		 &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, char &value, bool required, char *comment)
{
  static char val[FLEN_VALUE];
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TSTRING, name, (void *) val, comment, &fits_status);
  value = *val;
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, char *value, int valLen, bool required,
		     char *comment)
{
  static char val[FLEN_VALUE];
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key (ffile, TSTRING, name, (void *) val, comment, &fits_status);
  strncpy (value, val, valLen);
  value[valLen - 1] = '\0';
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValue (char *name, char **value, int valLen, bool required,
		     char *comment)
{
  int ret;
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  fits_read_key_longstr (ffile, name, value, comment, &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValues (char *name, int *values, int num, bool required,
		      int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_log (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValues (char *name, long *values, int num, bool required,
		      int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_lng (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValues (char *name, double *values, int num, bool required,
		      int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_dbl (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusGetValue (name, required);
}

int
Rts2Image::getValues (char *name, char **values, int num, bool required,
		      int nstart)
{
  if (!ffile)
    return -1;
  int nfound;
  fits_read_keys_str (ffile, name, nstart, num, values, &nfound,
		      &fits_status);
  return fitsStatusGetValue (name, required);
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
  filter_i = im_h->filter;
  setValue ("CAM_FILT", filter_i, "filter used for image");
  setValue ("SHUTTER", im_h->shutter,
	    "shutter state (1 - open, 2 - closed, 3 - synchro)");
  setValue ("SUBEXP", im_h->subexp, "subexposure length");
  setValue ("NACC", im_h->nacc, "number of accumulations");
  // dark images don't need to wait till imgprocess will pick them up for reprocessing
  switch (im_h->shutter)
    {
    case 1:
      shutter = SHUT_OPENED;
      break;
    case 2:
      shutter = SHUT_CLOSED;
      break;
    case 3:
      shutter = SHUT_SYNCHRO;
      break;
    default:
      shutter = SHUT_UNKNOW;
    }
  return 0;
}

int
Rts2Image::writeDate (Rts2ClientTCPDataConn * dataConn)
{
  struct imghdr *im_h;
  unsigned short *pixel;
  unsigned short *top;
  int ret;

  average = 0;
  stdev = 0;
  bg_stdev = 0;

  im_h = dataConn->getImageHeader ();
  // we have to copy data to FITS anyway, so let's do it right now..
  if (im_h->naxes != 2)
    {
      logStream (MESSAGE_ERROR) << "Rts2Image::writeDate not 2D image " <<
	im_h->naxes << sendLog;
      return -1;
    }
  flags |= IMAGE_SAVE;
  imageType = im_h->data_type;

  naxis[0] = im_h->sizes[0];
  naxis[1] = im_h->sizes[1];

  delete imageData;
  if (flags & IMAGE_KEEP_DATA)
    {
      imageData = new char[dataConn->getSize ()];
      memcpy (imageData, dataConn->getData (), dataConn->getSize ());
    }
  else
    {
      imageData = NULL;
    }

  // calculate average..
  pixel = (unsigned short *) dataConn->getData ();
  top = dataConn->getTop ();
  while (pixel < top)
    {
      average += *pixel;
      pixel++;
    }

  int bg_size = 0;

  if ((dataConn->getSize () / getPixelByteSize ()) > 0)
    {
      average /= dataConn->getSize () / 2;
      // calculate stdev
      pixel = (unsigned short *) dataConn->getData ();
      while (pixel < top)
	{
	  long double tmp_s = *pixel - average;
	  long double tmp_ss = tmp_s * tmp_s;
	  if (fabs (tmp_s) < average / 10)
	    {
	      bg_stdev += tmp_ss;
	      bg_size++;
	    }
	  stdev += tmp_ss;
	  pixel++;
	}
      stdev = sqrt (stdev / (dataConn->getSize () / 2));
      bg_stdev = sqrt (bg_stdev / bg_size);
    }
  else
    {
      average = 0;
      stdev = 0;
    }

  ret = writeImgHeader (im_h);

  if (!ffile || !(flags & IMAGE_SAVE))
    {
#ifdef DEBUG_EXTRA
      logStream (MESSAGE_DEBUG) << "not saving data " << ffile << " " <<
	(flags & IMAGE_SAVE) << sendLog;
#endif /* DEBUG_EXTRA */
      return 0;
    }

  if (imageType == RTS2_DATA_SBYTE)
    {
      fits_resize_img (ffile, RTS2_DATA_BYTE, im_h->naxes, naxis,
		       &fits_status);
    }
  else
    {
      fits_resize_img (ffile, imageType, im_h->naxes, naxis, &fits_status);
    }
  if (fits_status)
    {
      logStream (MESSAGE_ERROR) << "cannot resize image: " << getFitsErrors ()
	<< "imageType " << imageType << sendLog;
      return -1;
    }
  switch (imageType)
    {
    case RTS2_DATA_BYTE:
      fits_write_img_byt (ffile, 0, 1,
			  dataConn->getSize () / getPixelByteSize (),
			  (unsigned char *) dataConn->getData (),
			  &fits_status);
      break;
    case RTS2_DATA_SHORT:
      fits_write_img_sht (ffile, 0, 1,
			  dataConn->getSize () / getPixelByteSize (),
			  (int16_t *) dataConn->getData (), &fits_status);
      break;
    case RTS2_DATA_LONG:
      fits_write_img_lng (ffile, 0, 1,
			  dataConn->getSize () / getPixelByteSize (),
			  (long int *) dataConn->getData (), &fits_status);
      break;

    case RTS2_DATA_LONGLONG:
      fits_write_img_lnglng (ffile, 0, 1,
			     dataConn->getSize () / getPixelByteSize (),
			     (LONGLONG *) dataConn->getData (), &fits_status);
      break;
    case RTS2_DATA_FLOAT:
      fits_write_img_flt (ffile, 0, 1,
			  dataConn->getSize () / getPixelByteSize (),
			  (float *) dataConn->getData (), &fits_status);
      break;
    case RTS2_DATA_DOUBLE:
      fits_write_img_dbl (ffile, 0, 1,
			  dataConn->getSize () / getPixelByteSize (),
			  (double *) dataConn->getData (), &fits_status);
      break;
    case RTS2_DATA_SBYTE:
      fits_write_img_sbyt (ffile, 0, 1,
			   dataConn->getSize () / getPixelByteSize (),
			   (signed char *) dataConn->getData (),
			   &fits_status);
      break;
    case RTS2_DATA_USHORT:
      fits_write_img_usht (ffile, 0, 1,
			   dataConn->getSize () / getPixelByteSize (),
			   (short unsigned int *) dataConn->getData (),
			   &fits_status);
      break;
    case RTS2_DATA_ULONG:
      fits_write_img_ulng (ffile, 0, 1,
			   dataConn->getSize () / getPixelByteSize (),
			   (unsigned long *) dataConn->getData (),
			   &fits_status);
      break;
    default:
      logStream (MESSAGE_ERROR) << "Unknow imageType " << imageType <<
	sendLog;
      return -1;
    }
  if (fits_status)
    {
      logStream (MESSAGE_ERROR) << "cannot write data: " << getFitsErrors ()
	<< sendLog;
      return -1;
    }
  setValue ("AVERAGE", average, "average value of image");
  setValue ("STDEV", stdev, "standard deviation value of image");
  setValue ("BGSTDEV", stdev, "standard deviation value of background");
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "writeDate returns " << ret << sendLog;
#endif /* DEBUG_EXTRA */
  return ret;
}

int
Rts2Image::fitsStatusValue (char *valname, const char *operation,
			    bool required)
{
  int ret = 0;
  if (fits_status)
    {
      ret = -1;
      if (required)
	{
	  logStream (MESSAGE_ERROR) << operation << " value " << valname <<
	    " error " << getFitsErrors () << sendLog;
	}
    }
  fits_status = 0;
  return ret;
}

double
Rts2Image::getAstrometryErr ()
{
  struct ln_equ_posn pos2;
  pos2.ra = pos_astr.ra + ra_err;
  pos2.dec = pos_astr.dec + dec_err;
  return ln_get_angular_separation (&pos_astr, &pos2);
}

int
Rts2Image::closeFile ()
{
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Closing " << getImageName () << " ffile " <<
    ffile << sendLog;
#endif /* DEBUG_EXTRA */

  if ((flags & IMAGE_SAVE) && ffile)
    {
      // save astrometry error
      if (!isnan (ra_err) && !isnan (dec_err))
	{
	  setValue ("RA_ERR", ra_err, "RA error in position");
	  setValue ("DEC_ERR", dec_err, "DEC error in position");
	  setValue ("POS_ERR", getAstrometryErr (), "error in position");
	}
      if (!isnan (config_rotang))
	{
	  int flip;
	  flip = getMountFlip ();
	  switch (flip)
	    {
	    case 1:
	      config_rotang += 180.0;
	      config_rotang = ln_range_degrees (config_rotang);
	    case 0:
	      setValue ("ROTANG", config_rotang,
			"camera rotation over X axis");
	      config_rotang = nan ("f");
	      break;
	    default:
	      setValue ("ROTANG", config_rotang,
			"CONFIG rotang - not corrected for flip");
	    }
	}
      setCreationDate ();
      fits_close_file (ffile, &fits_status);
      flags &= ~IMAGE_SAVE;
    }
  else if (ffile)
    {
      // close ffile to free resources..
      fits_close_file (ffile, &fits_status);
    }
  ffile = NULL;
  return 0;
}

int
Rts2Image::saveImage ()
{
  return closeFile ();
}

int
Rts2Image::deleteImage ()
{
  int ret;
  fits_close_file (ffile, &fits_status);
  ffile = NULL;
  flags &= ~IMAGE_SAVE;
  ret = unlink (getImageName ());
  return ret;
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

std::string Rts2Image::getEpochString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (3) << epochId;
  return _os.str ();
}

std::string Rts2Image::getTargetString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (5) << getTargetId ();
  return _os.str ();
}

std::string Rts2Image::getTargetSelString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (5) << getTargetIdSel ();
  return _os.str ();
}

std::string Rts2Image::getObsString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (5) << getObsId ();
  return _os.str ();
}

std::string Rts2Image::getImgIdString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (5) << getImgId ();
  return _os.str ();
}

// Rts2Image can be only raw..
std::string Rts2Image::getProcessingString ()
{
  return std::string ("RA");
}

std::string Rts2Image::getStartYearString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (4) << getStartYear ();
  return _os.str ();
}

std::string Rts2Image::getStartMonthString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (2) << getStartMonth ();
  return _os.str ();
}

std::string Rts2Image::getStartDayString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (2) << getStartDay ();
  return _os.str ();
}

std::string Rts2Image::getStartYDayString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (3) << getStartYDay ();
  return _os.str ();
}

std::string Rts2Image::getStartHourString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (2) << getStartHour ();
  return _os.str ();
}

std::string Rts2Image::getStartMinString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (2) << getStartMin ();
  return _os.str ();
}

std::string Rts2Image::getStartSecString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (2) << getStartSec ();
  return _os.str ();
}

std::string Rts2Image::getStartMSecString ()
{
  std::ostringstream _os;
  _os.fill ('0');
  _os << std::setw (4) << ((int) (exposureStart.tv_usec / 1000.0));
  return _os.str ();
}

std::string Rts2Image::getExposureLengthString ()
{
  std::ostringstream _os;
  _os << getExposureLength ();
  return _os.str ();
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

void
Rts2Image::setFilter (const char *in_filter)
{
  filter = new char[strlen (in_filter) + 1];
  strcpy (filter, in_filter);
  setValue ("FILTER", filter, "camera filter as string");
}

void
Rts2Image::computeStatistics ()
{

}

int
Rts2Image::loadData ()
{
  // try to load data..
  int anyNull = 0;
  int ret;
  fits_get_img_equivtype (ffile, &imageType, &fits_status);
  fitsStatusGetValue ("image equivType loadData", true);
  imageData = new char[getWidth () * getHeight () * getPixelByteSize ()];
  if (!ffile)
    {
      ret = openImage ();
      if (ret)
	return ret;
    }
  switch (imageType)
    {
    case RTS2_DATA_BYTE:
      fits_read_img_byt (ffile, 0, 1,
			 getWidth () * getHeight (), 0,
			 (unsigned char *) imageData, &anyNull, &fits_status);
      break;
    case RTS2_DATA_SHORT:
      fits_read_img_sht (ffile, 0, 1,
			 getWidth () * getHeight (), 0,
			 (int16_t *) imageData, &anyNull, &fits_status);
      break;
    case RTS2_DATA_LONG:
      fits_read_img_lng (ffile, 0, 1,
			 getWidth () * getHeight (), 0,
			 (long int *) imageData, &anyNull, &fits_status);
      break;

    case RTS2_DATA_LONGLONG:
      fits_read_img_lnglng (ffile, 0, 1,
			    getWidth () * getHeight (), 0,
			    (LONGLONG *) imageData, &anyNull, &fits_status);
      break;
    case RTS2_DATA_FLOAT:
      fits_read_img_flt (ffile, 0, 1,
			 getWidth () * getHeight (), 0,
			 (float *) imageData, &anyNull, &fits_status);
      break;
    case RTS2_DATA_DOUBLE:
      fits_read_img_dbl (ffile, 0, 1,
			 getWidth () * getHeight (), 0,
			 (double *) imageData, &anyNull, &fits_status);
      break;
    case RTS2_DATA_SBYTE:
      fits_read_img_sbyt (ffile, 0, 1,
			  getWidth () * getHeight (), 0,
			  (signed char *) imageData, &anyNull, &fits_status);
      break;
    case RTS2_DATA_USHORT:
      fits_read_img_usht (ffile, 0, 1,
			  getWidth () * getHeight (), 0,
			  (short unsigned int *) imageData, &anyNull,
			  &fits_status);
      break;
    case RTS2_DATA_ULONG:
      fits_read_img_ulng (ffile, 0, 1,
			  getWidth () * getHeight (), 0,
			  (unsigned long *) imageData, &anyNull,
			  &fits_status);
      break;
    default:
      logStream (MESSAGE_ERROR) << "Unknow imageType " << imageType <<
	sendLog;
      delete imageData;
      imageData = NULL;
      return -1;
    }
  if (fits_status)
    {
      delete imageData;
      imageData = NULL;
      imageType = RTS2_DATA_USHORT;
    }
  return fitsStatusGetValue ("image image loadData", true);
}

void *
Rts2Image::getData ()
{
  int ret;
  if (!imageData)
    {
      ret = loadData ();
      if (ret)
	return NULL;
    }
  return (void *) imageData;
}

unsigned short *
Rts2Image::getDataUShortInt ()
{
  if (getData () == NULL)
    return NULL;
  // switch by format
  if (imageType == RTS2_DATA_USHORT)
    return (unsigned short *) imageData;
  // convert type to ushort int
  return NULL;
}

void
Rts2Image::setDataUShortInt (unsigned short *in_data, long in_naxis[2])
{
  delete imageData;
  imageData = (char *) in_data;
  imageType = RTS2_DATA_USHORT;
  naxis[0] = in_naxis[0];
  naxis[1] = in_naxis[1];
  flags |= IMAGE_DONT_DELETE_DATA;
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
Rts2Image::setAstroResults (double in_ra, double in_dec, double in_ra_err,
			    double in_dec_err)
{
  pos_astr.ra = in_ra;
  pos_astr.dec = in_dec;
  ra_err = in_ra_err;
  dec_err = in_dec_err;
  flags |= IMAGE_SAVE;
  return 0;
}

int
Rts2Image::addStarData (struct stardata *sr)
{
  if (sr->F <= 0)		// flux is not significant..
    return -1;
  // do not take stars on edge..
  if (sr->X == 0 || sr->X == getWidth ()
      || sr->Y == 0 || sr->Y == getHeight ())
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

/* static int
sdcompare (struct stardata *x1, struct stardata *x2)
{
  if (x1->fwhm < x2->fwhm)
    return 1;
  if (x1->fwhm > x2->fwhm)
    return -1;
  return 0;
} */

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
  int numStars;
  if (sexResultNum < 4)
    return nan ("f");

  // qsort (sexResults, sexResultNum, sizeof (struct stardata), sdcompare);
  // get average
  avg = 0;
  numStars = 0;
  sr = sexResults;
  for (i = 0; i < sexResultNum; i++, sr++)
    {
      if (isGoodForFwhm (sr))
	{
	  avg += sr->fwhm;
	  numStars++;
	}
    }
  avg /= numStars;
  return avg;
}

static int
shortcompare (const void *val1, const void *val2)
{
  return (*(unsigned short *) val1 < *(unsigned short *) val2) ? -1 :
    ((*(unsigned short *) val1 == *(unsigned short *) val2) ? 0 : 1);
}

unsigned short
getShortMean (unsigned short *averageData, int sub)
{
  qsort (averageData, sub, sizeof (unsigned short), shortcompare);
  if (sub % 2)
    return averageData[sub / 2];
  return (averageData[(sub / 2) - 1] + averageData[(sub / 2)]) / 2;
}

int
Rts2Image::getCenterRow (long row, int row_width, double &x)
{
  unsigned short *tmp_data;
  tmp_data = getDataUShortInt ();
  unsigned short *rowData;
  unsigned short *averageData;
  unsigned short data_max;
  long i;
  long j;
  // long k;
  long maxI;
  if (!tmp_data)
    return -1;
  // compute average collumn
  rowData = new unsigned short[getWidth ()];
  averageData = new unsigned short[row_width];
  tmp_data += row * getWidth ();
  if (row_width < 2)
    {
      return -1;
    }
  for (j = 0; j < getWidth (); j++)
    {
      for (i = 0; i < row_width; i++)
	{
	  averageData[i] = tmp_data[j + i * getWidth ()];
	}
      rowData[j] = getShortMean (averageData, row_width);
    }
  // bin data in row..
  /* i = j = k = 0;
     while (j < getWidth ())
     {
     // last bit..
     k += row_width;
     rowData[i] =
     getShortMean (rowData + j,
     (k > getWidth ())? (k - getWidth ()) : row_width);
     i++;
     j = k;
     } */
  // decide, which part of slope we are in
  // i hold size of reduced rowData array
  // find maximum..
  data_max = rowData[0];
  maxI = 0;
  for (j = 1; j < i; j++)
    {
      if (rowData[j] > data_max)
	{
	  data_max = rowData[j];
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
  delete[]averageData;
  return 0;
}

int
Rts2Image::getCenterCol (long col, int col_width, double &y)
{
  unsigned short *tmp_data;
  tmp_data = getDataUShortInt ();
  unsigned short *colData;
  unsigned short *averageData;
  unsigned short data_max;
  long i;
  long j;
  // long k;
  long maxI;
  if (!tmp_data)
    return -1;
  // smooth image..
  colData = new unsigned short[getHeight ()];
  averageData = new unsigned short[col_width];
  tmp_data += col * getHeight ();
  if (col_width < 2)
    {
      // special handling..
      return -1;
    }
  for (j = 0; j < getHeight (); j++)
    {
      for (i = 0; i < col_width; i++)
	{
	  averageData[i] = tmp_data[j + i * getHeight ()];
	}
      colData[j] = getShortMean (averageData, col_width);
    }
  // bin data in row..
  /* i = j = k = 0;
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
   */
  // find gauss on image..
  //ret = findGauss (colData, i, y);
  data_max = colData[0];
  maxI = 0;
  for (j = 1; j < i; j++)
    {
      if (colData[j] > data_max)
	{
	  data_max = colData[j];
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
  delete[]averageData;
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

int
Rts2Image::getError (double &eRa, double &eDec, double &eRad)
{
  if (isnan (ra_err) || isnan (dec_err) || isnan (img_err))
    return -1;
  eRa = ra_err;
  eDec = dec_err;
  eRad = img_err;
  return 0;
}

std::string Rts2Image::getOnlyFileName ()
{
  return expandPath ("%y%m%d%H%M%S-%s-%p.fits");
}

void
Rts2Image::printFileName (std::ostream & _os)
{
  _os << getImageName ();
}

void
Rts2Image::print (std::ostream & _os, int in_flags)
{
  if (in_flags & DISPLAY_FILENAME)
    {
      printFileName (_os);
      _os << SEP;
    }
  if (in_flags & DISPLAY_SHORT)
    {
      printFileName (_os);
      _os << std::endl;
      return;
    }

  if (in_flags & DISPLAY_OBS)
    _os << std::setw (5) << getObsId () << SEP;

  if (!(in_flags & DISPLAY_ALL))
    return;

  std::ios_base::fmtflags old_settings = _os.flags ();
  int old_precision = _os.precision (2);

  _os
    << std::setw (5) << getCameraName () << SEP
    << std::setw (4) << getImgId () << SEP
    << Timestamp (getExposureSec () +
		  (double) getExposureUsec () /
		  USEC_SEC) << SEP << std::
    setw (3) << getFilter () << SEP << std::
    setw (8) << getExposureLength () << "'" << SEP <<
    LibnovaDegArcMin (ra_err) << SEP << LibnovaDegArcMin (dec_err) << SEP <<
    LibnovaDegArcMin (img_err) << std::endl;

  _os.flags (old_settings);
  _os.precision (old_precision);
}

void
Rts2Image::writeClientValue (Rts2DevClient * client, Rts2Value * val)
{
  char *desc = (char *) val->getDescription ().c_str ();
  char *name = (char *) val->getName ().c_str ();
  char *name_stat;
  char *n_top;
  if (client->getOtherType () == DEVICE_TYPE_SENSOR
      || val->prefixWithDevice ())
    {
      name = new char[strlen (name) + strlen (client->getName ()) + 2];
      strcpy (name, client->getName ());
      strcat (name, ".");
      strcat (name, val->getName ().c_str ());
    }

  switch (val->getValueType ())
    {
    case RTS2_VALUE_STRING:
      setValue (name, val->getValue (), desc);
      break;
    case RTS2_VALUE_INTEGER:
      setValue (name, val->getValueInteger (), desc);
      break;
    case RTS2_VALUE_TIME:
      setValue (name, val->getValueDouble (), desc);
      break;
    case RTS2_VALUE_DOUBLE:
    case RTS2_VALUE_DOUBLE_MMAX:
      setValue (name, val->getValueDouble (), desc);
      break;
    case RTS2_VALUE_DOUBLE_STAT:
      setValue (name, val->getValueDouble (), desc);
      name_stat = new char[strlen (name) + 6];
      n_top = name_stat + strlen (name);
      strcpy (name_stat, name);
      *n_top = '.';
      n_top++;
      strcpy (n_top, "MODE");
      setValue (name_stat, ((Rts2ValueDoubleStat *) val)->getMode (), desc);
      strcpy (n_top, "MIN");
      setValue (name_stat, ((Rts2ValueDoubleStat *) val)->getMin (), desc);
      strcpy (n_top, "MAX");
      setValue (name_stat, ((Rts2ValueDoubleStat *) val)->getMax (), desc);
      strcpy (n_top, "STD");
      setValue (name_stat, ((Rts2ValueDoubleStat *) val)->getStdev (), desc);
      strcpy (n_top, "NUM");
      setValue (name_stat, ((Rts2ValueDoubleStat *) val)->getNumMes (), desc);
      delete[]name_stat;
      break;
    case RTS2_VALUE_FLOAT:
      setValue (name, val->getValueFloat (), desc);
      break;
    case RTS2_VALUE_BOOL:
      setValue (name, ((Rts2ValueBool *) val)->getValueBool (), desc);
      break;
    case RTS2_VALUE_SELECTION:
      setValue (name,
		((Rts2ValueSelection *) val)->getSelVal ().c_str (), desc);
      break;
    default:
      logStream (MESSAGE_ERROR) <<
	"Don't know how to write to FITS file header value '" << name
	<< "' of type " << val->getValueType () << sendLog;
      break;
    }
  if (client->getOtherType () == DEVICE_TYPE_SENSOR
      || val->prefixWithDevice ())
    {
      delete[]name;
    }
}

void
Rts2Image::writeClient (Rts2DevClient * client, imageWriteWhich_t which)
{
  for (std::vector < Rts2Value * >::iterator iter = client->valueBegin ();
       iter != client->valueEnd (); iter++)
    {
      Rts2Value *val = *iter;
      if (val->getWriteToFits ())
	{
	  switch (which)
	    {
	    case EXPOSURE_START:
	      if (val->getValueWriteFlags () == RTS2_VWHEN_BEFORE_EXP)
		writeClientValue (client, val);
	      break;
	    case EXPOSURE_END:
	      if (val->getValueWriteFlags () == RTS2_VWHEN_BEFORE_END)
		writeClientValue (client, val);
	      break;
	    }
	}
    }
}

double
Rts2Image::getLongtitude ()
{
  double lng = nan ("f");
  getValue ("LONG", lng, true);
  return lng;
}


double
Rts2Image::getExposureJD ()
{
  return ln_get_julian_from_timet (&exposureStart.tv_sec) +
    exposureStart.tv_usec / USEC_SEC / 86400.0;
}

double
Rts2Image::getExposureLST ()
{
  double ret;
  ret =
    ln_get_apparent_sidereal_time (getExposureJD () * 15.0 +
				   getLongtitude ());
  return ln_range_degrees (ret) / 15.0;
}

std::ostream & operator << (std::ostream & _os, Rts2Image & image)
{
  _os << "C " << image.getCameraName ()
    << " [" << image.getWidth () << ":"
    << image.getHeight () << "]"
    << " RA " << image.getCenterRa () << " (" << LibnovaDegArcMin (image.
								   ra_err) <<
    ") DEC " << image.getCenterDec () << " (" << LibnovaDegArcMin (image.
								   dec_err)
    << ") IMG_ERR " << LibnovaDegArcMin (image.img_err);
  return _os;
}
