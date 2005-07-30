#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
  rotang = 0;

  config->getDouble (connection->getName (), "xplate", xplate);
  config->getDouble (connection->getName (), "yplate", yplate);
  config->getDouble (connection->getName (), "xoa", xoa);
  config->getDouble (connection->getName (), "yoa", yoa);
  config->getDouble (connection->getName (), "rotang", rotang);
  config->getInteger (connection->getName (), "flip", flip);
  config->getString (connection->getName (), "filter", filter, 200);
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
  int camFilter = images->getFilter ();
  if (camFilter < 1)
    camFilter = 1;
  char *flt = filter;
  char *flt_end;
  char imageFilter[4];
  int flt_len;
  while (*flt)
    {
      while (isspace (*flt))
	flt++;
      if (camFilter == 1)	// got filter..
	break;
      while (*flt && !isspace (*flt))
	flt++;
      if (!*flt)
	break;
      camFilter--;
    }
  if (camFilter == 1 && *flt)
    {
      flt_end = flt;
      flt_len = 0;
      while (!isspace (*flt_end) && *flt_end && flt_len < 3)
	{
	  flt_end++;
	  flt_len++;
	}
      strncpy (imageFilter, flt, flt_len);
      imageFilter[3] = '\0';
    }
  else
    {
      strcpy (imageFilter, "UNK");
    }
  images->setValue ("FILTER", imageFilter, "camera filter as string");
}

void
Rts2DevClientCameraImage::dataReceived (Rts2ClientTCPDataConn * dataConn)
{
  Rts2DevClientCamera::dataReceived (dataConn);
  if (images)
    {
      if (saveImage)
	{
	  images->writeDate (dataConn);
	  writeFilter ();
	  // set filter..
	}
      // save us to the disk..
      images->saveImage ();
      // do basic processing
      processImage (images);
      delete images;
      images = NULL;
    }
}

Rts2Image *
Rts2DevClientCameraImage::createImage (const struct timeval *expStart)
{
  struct tm expT;
  char fn[20];
  gmtime_r (&expStart->tv_sec, &expT);
  strftime (fn, 20, "%H%M%S.fits", &expT);
  return new Rts2Image (fn, expStart);
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
  connection->postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE, images));
  images->setValue ("CCD_TEMP", getValueChar ("ccd_temperature"),
		    "CCD temperature");
  images->setValue ("EXPOSURE", exposureTime, "exposure time");
  images->setValue ("CAM_FAN", getValueInteger ("fan"),
		    "fan on (1) / off (0)");
  images->setValue ("XPLATE", xplate,
		    "xplate (scale in X axis; divide by binning (BIN_H)!)");
  images->setValue ("YPLATE", yplate,
		    "yplate (scale in Y axis; divide by binning (BIN_V)!)");
  images->setValue ("CAM_XOA", xoa,
		    "center in X axis (divide by binning (BIN_H)!)");
  images->setValue ("CAM_YOA", yoa,
		    "center in Y axis (divide by binning (BIN_V)!)");
  images->setValue ("ROTANG", rotang, "camera rotation over X axis");
  images->setValue ("FLIP", flip,
		    "camera flip (since most astrometry devices works as mirrors");
  focuser = getValueChar ("focuser");
  if (focuser)
    {
      images->setFocuserName (focuser);
    }
  Rts2DevClientCamera::exposureStarted ();
}

void
Rts2DevClientCameraImage::exposureEnd ()
{
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
  switch (event->getType ())
    {
    case EVENT_WRITE_TO_IMAGE:
      Rts2Image * image;
      struct ln_equ_posn object;
      struct ln_lnlat_posn obs;
      struct ln_hrz_posn hrz;
      double siderealtime;
      image = (Rts2Image *) event->getArg ();
      image->setMountName (connection->getName ());
      image->setValue ("MNT_TYPE", getValueChar ("type"), "mount telescope");
      image->setValue ("MNT_MARK", getValueInteger ("correction_mark"),
		       "mark used for mount corretion");
      getEqu (&object);
      getObs (&obs);
      image->setValue ("RASC", object.ra, "mount RA");
      image->setValue ("DECL", object.dec, "mount DEC");
      image->setValue ("LONG", obs.lng, "mount longtitude");
      image->setValue ("LAT", obs.lat, "mount latitude");
      siderealtime = getValueDouble ("siderealtime");
      ln_get_hrz_from_equ_sidereal_time (&object, &obs, siderealtime, &hrz);
      image->setValue ("ALT", hrz.alt, "mount altitude");
      image->setValue ("AZ", hrz.az, "mount azimut");
      image->setValue ("MNT_FLIP", getValueInteger ("flip"), "mount flip");
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
      image->setValue ("DOME_NAME", connection->getName (),
		       "name of the dome");
      image->setValue ("RAIN", getValueInteger ("rain"), "is it raining");
      image->setValue ("WINDSPED", getValueDouble ("windspeed"), "windspeed");
      image->setValue ("DOME_TEMP", getValueDouble ("temperature"),
		       "temperature in degrees C");
      image->setValue ("DOME_HUM", getValueDouble ("humidity"), "humidity");
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
      image->setValue ("FOC_POS", getValueInteger ("pos"),
		       "focuser position");
      image->setValue ("FOC_TEMP", getValueInteger ("temp"),
		       "focuser temperature");
      break;
    }
  Rts2DevClientFocus::postEvent (event);
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
