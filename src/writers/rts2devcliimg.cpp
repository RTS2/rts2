#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>

#include "rts2devcliimg.h"
#include "../utils/rts2config.h"

Rts2DevClientCameraImage::Rts2DevClientCameraImage (Rts2Conn * in_connection):Rts2DevClientCamera
  (in_connection)
{
  images = NULL;
  chipNumbers = 0;
  saveImage = 1;

  exposureTime = 1.0;
  exposureT = EXP_LIGHT;
  exposureChip = 0;
  exposureCount = 0;

  isExposing = 0;

  Rts2Config *
    config;
  config = Rts2Config::instance ();

  xplate = 1;
  yplate = 1;
  xoa = 0;
  yoa = 0;
  ter_xoa = nan ("f");
  ter_yoa = nan ("f");
  flip = 1;
  config_rotang = 0;

  config->getDouble (connection->getName (), "xplate", xplate);
  config->getDouble (connection->getName (), "yplate", yplate);
  config->getDouble (connection->getName (), "xoa", xoa);
  config->getDouble (connection->getName (), "yoa", yoa);
  config->getDouble (connection->getName (), "ter_xoa", ter_xoa);
  config->getDouble (connection->getName (), "ter_yoa", ter_yoa);
  config->getDouble (connection->getName (), "rotang", config_rotang);
  config->getInteger (connection->getName (), "flip", flip);
  config->getString (connection->getName (), "filter", filter, 200);

  telescop[0] = '\0';
  instrume[0] = '\0';
  origin[0] = '\0';

  config->getString (connection->getName (), "instrume", instrume, 70);
  config->getString (connection->getName (), "telescop", telescop, 70);
  config->getString (connection->getName (), "origin", origin, 70);
}

Rts2DevClientCameraImage::~Rts2DevClientCameraImage (void)
{
  if (images)
    delete images;
}

void
Rts2DevClientCameraImage::queExposure ()
{
  if (isExposing || !exposureCount)
    return;
  if (exposureCount > 0)
    exposureCount--;
  connection->
    queCommand (new
		Rts2CommandExposure (connection->getMaster (), this,
				     exposureT, exposureTime));
  isExposing = 1;
  if (exposureT != EXP_DARK)
    blockWait ();
}

void
Rts2DevClientCameraImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SET_TARGET_ID:
      activeTargetId = (int) event->getArg ();
      break;
    }
  Rts2DevClientCamera::postEvent (event);
}

void
Rts2DevClientCameraImage::writeFilter ()
{
  int camFilter = images->getFilterNum ();
  char *flt = filter;
  char *flt_end;
  char imageFilter[4];
  int flt_len;
  while (camFilter >= 0 && *flt)
    {
      while (isspace (*flt))
	flt++;
      if (camFilter == 0)	// got filter..
	break;
      while (*flt && !isspace (*flt))
	flt++;
      if (!*flt)
	break;
      camFilter--;
    }
  if (camFilter == 0 && *flt)
    {
      flt_end = flt;
      flt_len = 0;
      while (!isspace (*flt_end) && *flt_end && flt_len < 3)
	{
	  flt_end++;
	  flt_len++;
	}
      strncpy (imageFilter, flt, flt_len);
      imageFilter[flt_len] = '\0';
    }
  else
    {
      strcpy (imageFilter, "UNK");
    }
  images->setFilter (imageFilter);
}

void
Rts2DevClientCameraImage::dataReceived (Rts2ClientTCPDataConn * dataConn)
{
  Rts2DevClientCamera::dataReceived (dataConn);
  if (images)
    {
      images->writeDate (dataConn);
      // create new image of requsted type
      beforeProcess (images);
      if (saveImage)
	{
	  writeFilter ();
	  // set filter..
	  // save us to the disk..
	  images->saveImage ();
	}
      // do basic processing
      processImage (images);
    }
}

Rts2Image *
Rts2DevClientCameraImage::createImage (const struct timeval *expStart)
{
  struct tm expT;
  char fn[50];
  int camlen;
  camlen = strlen (connection->getName ());
  strcpy (fn, connection->getName ());
  strcat (fn, "_");
  gmtime_r (&expStart->tv_sec, &expT);
  sprintf (fn + camlen + 1, "%i%02i%02i-%02i%02i%02i-%04i.fits",
	   expT.tm_year + 1900, expT.tm_mon + 1, expT.tm_mday,
	   expT.tm_hour, expT.tm_min, expT.tm_sec,
	   int (expStart->tv_usec / 1000));
  return new Rts2Image (fn, expStart);
}

void
Rts2DevClientCameraImage::beforeProcess (Rts2Image * image)
{
}

void
Rts2DevClientCameraImage::processImage (Rts2Image * image)
{
}

void
Rts2DevClientCameraImage::exposureFailed (int status)
{
  isExposing = 0;
  readoutEnd ();		// pretend we can expose new image after failure..
}

void
Rts2DevClientCameraImage::exposureStarted ()
{
  if (images)
    delete images;
  exposureTime = getValueDouble ("exposure");
  struct timeval expStart;
  char *focuser;
  gettimeofday (&expStart, NULL);
  images = createImage (&expStart);
  images->setValue ("CCD_TEMP", getValueChar ("ccd_temperature"),
		    "CCD temperature");
  images->setExposureLength (exposureTime);

  images->setValue ("GAIN", getValueDouble ("gain"), "CCD gain");
  images->setValue ("RNOISE", getValueDouble ("rnoise"), "CCD readout noise");

  images->setValue ("CAM_FAN", getValueInteger ("fan"),
		    "fan on (1) / off (0)");
  images->setValue ("XPLATE", xplate,
		    "xplate (scale in X axis; divide by binning (BIN_H)!)");
  images->setValue ("YPLATE", yplate,
		    "yplate (scale in Y axis; divide by binning (BIN_V)!)");

  images->setInstrument (instrume);
  images->setTelescope (telescop);
  images->setOrigin (origin);

  if (images->getTargetType () == TYPE_TERESTIAL
      && !isnan (ter_xoa) && !isnan (ter_yoa))
    {
      images->setValue ("CAM_XOA", ter_xoa,
			"ter center in X axis (divide by binning (BIN_H)!)");
      images->setValue ("CAM_YOA", ter_yoa,
			"ter center in Y axis (divide by binning (BIN_V)!)");
    }
  else
    {
      images->setValue ("CAM_XOA", xoa,
			"center in X axis (divide by binning (BIN_H)!)");
      images->setValue ("CAM_YOA", yoa,
			"center in Y axis (divide by binning (BIN_V)!)");
    }
  images->setConfigRotang (config_rotang);
  images->setValue ("FLIP", flip,
		    "camera flip (since most astrometry devices works as mirrors");
  focuser = getValueChar ("focuser");
  if (focuser)
    {
      images->setFocuserName (focuser);
    }
  connection->postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE, images));
  Rts2DevClientCamera::exposureStarted ();
}

void
Rts2DevClientCameraImage::exposureEnd ()
{
  connection->getMaster ()->
    logStream (MESSAGE_DEBUG) << "Rts2DevClientCameraImage::exposureEnd " <<
    connection->getName () << sendLog;
  if (!isExposing)
    return Rts2DevClientCamera::exposureEnd ();
  isExposing = 0;
  if (exposureT != EXP_DARK)
    unblockWait ();
  connection->
    queCommand (new Rts2Command (connection->getMaster (), "readout 0"));
  Rts2DevClientCamera::exposureEnd ();
}

void
Rts2DevClientCameraImage::readoutEnd ()
{
  queExposure ();
  Rts2DevClientCamera::readoutEnd ();
}

Rts2DevClientTelescopeImage::Rts2DevClientTelescopeImage (Rts2Conn * in_connection):Rts2DevClientTelescope
  (in_connection)
{
}

void
Rts2DevClientTelescopeImage::postEvent (Rts2Event * event)
{
  struct ln_equ_posn *change;	// change in degrees
  switch (event->getType ())
    {
    case EVENT_WRITE_TO_IMAGE:
      Rts2Image * image;
      struct ln_equ_posn object;
      struct ln_lnlat_posn obs;
      struct ln_hrz_posn hrz;
      double gst;
      image = (Rts2Image *) event->getArg ();
      image->setMountName (connection->getName ());
      image->setValue ("MNT_TYPE", getValueChar ("type"), "mount telescope");
      image->setValue ("MNT_MARK", getValueInteger ("correction_mark"),
		       "mark used for mount corretion");
      getEqu (&object);
      getAltAz (&hrz);
      getObs (&obs);
      image->setValue ("RASC", getValueDouble ("ra_tar"), "target RA");
      image->setValue ("DECL", getValueDouble ("dec_tar"), "target DEC");
      image->setValue ("CUR_RA", object.ra,
		       "current RA (real or from astrometry)");
      image->setValue ("CUR_DEC", object.dec,
		       "current DEC (real or from astrometry)");
      image->setValue ("MNT_RA", getValueDouble ("ra_tel"),
		       "mount RA (read from sensors)");
      image->setValue ("MNT_DEC", getValueDouble ("dec_tel"),
		       "mount DEC (read from sensors)");
      image->setValue ("MNT_AX0", getValueDouble ("axis0_counts"),
		       "mount axis 0 counts");
      image->setValue ("MNT_AX1", getValueDouble ("axis1_counts"),
		       "mount axis 1 counts");
      image->setValue ("LONG", obs.lng, "mount longtitude");
      image->setValue ("LAT", obs.lat, "mount latitude");
      gst = getValueDouble ("siderealtime") * 15.0 - obs.lng;
      gst = ln_range_degrees (gst) / 15.0;
      image->setValue ("GST", gst, "Global Sidereal Time");
      image->setValue ("ALT", hrz.alt, "mount altitude");
      image->setValue ("AZ", hrz.az, "mount azimut");
      image->setValue ("MNT_FLIP", getValueInteger ("flip"), "mount flip");
      break;
    case EVENT_GET_RADEC:
      getEqu ((struct ln_equ_posn *) event->getArg ());
      break;
    case EVENT_MOUNT_CHANGE:
      change = (struct ln_equ_posn *) event->getArg ();
      queCommand (new Rts2CommandChange (this, change->ra, change->dec));
      break;
    }

  Rts2DevClientTelescope::postEvent (event);
}

void
Rts2DevClientTelescopeImage::getEqu (struct ln_equ_posn *tel)
{
  tel->ra = getValueDouble ("ra");
  tel->dec = getValueDouble ("dec");
}

void
Rts2DevClientTelescopeImage::getEquTel (struct ln_equ_posn *tel)
{
  tel->ra = getValueDouble ("ra_tel");
  tel->dec = getValueDouble ("dec_tel");
}

void
Rts2DevClientTelescopeImage::getEquTar (struct ln_equ_posn *tar)
{
  tar->ra = getValueDouble ("ra_tar");
  tar->dec = getValueDouble ("dec_tar");
}

void
Rts2DevClientTelescopeImage::getAltAz (struct ln_hrz_posn *hrz)
{
  struct ln_equ_posn pos;
  struct ln_lnlat_posn obs;
  double gst;

  getEqu (&pos);
  getObs (&obs);
  gst = getLocalSiderealDeg () - obs.lng;
  gst = ln_range_degrees (gst) / 15.0;

  ln_get_hrz_from_equ_sidereal_time (&pos, &obs, gst, hrz);

//  printf ("%f %f %f %f\n", pos.ra, pos.dec, hrz->alt, hrz->az);
}

void
Rts2DevClientTelescopeImage::getObs (struct ln_lnlat_posn *obs)
{
  obs->lng = getValueDouble ("longtitude");
  obs->lat = getValueDouble ("latitude");
}

double
Rts2DevClientTelescopeImage::getLocalSiderealDeg ()
{
  return getValueDouble ("siderealtime") * 15.0;
}

double
Rts2DevClientTelescopeImage::getDistance (struct ln_equ_posn *in_pos)
{
  struct ln_equ_posn tel;
  getEqu (&tel);
  return ln_get_angular_separation (&tel, in_pos);
}

Rts2DevClientDomeImage::Rts2DevClientDomeImage (Rts2Conn * in_connection):Rts2DevClientDome
  (in_connection)
{
}

void
Rts2DevClientDomeImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_WRITE_TO_IMAGE:
      Rts2Image * image;
      image = (Rts2Image *) event->getArg ();
      image->setValue ("DOME", connection->getName (), "name of the dome");
      image->setValue ("RAIN", getValueInteger ("rain"),
		       "whenever is raining");
      image->setValue ("WINDSPED", getValueDouble ("windspeed"), "windspeed");
      image->setValue ("DOME_TMP", getValueDouble ("temperature"),
		       "temperature in degrees C");
      image->setValue ("DOME_HUM", getValueDouble ("humidity"),
		       "(outside) humidity");
      break;
    }
  Rts2DevClientDome::postEvent (event);
}

Rts2DevClientFocusImage::Rts2DevClientFocusImage (Rts2Conn * in_connection):Rts2DevClientFocus
  (in_connection)
{
}

void
Rts2DevClientFocusImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_WRITE_TO_IMAGE:
      Rts2Image * image;
      image = (Rts2Image *) event->getArg ();
      // check if we are correct focuser for given camera
      if (!image->getFocuserName ()
	  || !connection->getName ()
	  || strcmp (image->getFocuserName (), connection->getName ()))
	break;
      image->setValue ("FOC_TYPE", getValueInteger ("type"), "focuser type");
      image->setFocPos (getValueInteger ("pos"));
      image->setValue ("FOC_TEMP", getValueInteger ("temp"),
		       "focuser temperature");
      break;
    }
  Rts2DevClientFocus::postEvent (event);
}

Rts2DevClientMirrorImage::Rts2DevClientMirrorImage (Rts2Conn * in_connection):Rts2DevClientMirror
  (in_connection)
{
}

void
Rts2DevClientMirrorImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_WRITE_TO_IMAGE:
      Rts2Image * image;
      char *mirrVal;
      asprintf (&mirrVal, "MIR_%s", connection->getName ());
      image = (Rts2Image *) event->getArg ();
      image->setValue (mirrVal, connection->getState (), "mirror status");
      free (mirrVal);
      break;
    }
  Rts2DevClientMirror::postEvent (event);
}


Rts2CommandQueImage::Rts2CommandQueImage (Rts2Block * in_owner, Rts2Image * image):Rts2Command
  (in_owner)
{
  char *
    command;
  asprintf (&command, "que_image %s", image->getImageName ());
  setCommand (command);
  free (command);
}

Rts2CommandQueObs::Rts2CommandQueObs (Rts2Block * in_owner, int in_obsId):
Rts2Command (in_owner)
{
  char *command;
  asprintf (&command, "que_obs %i", in_obsId);
  setCommand (command);
  free (command);
}
