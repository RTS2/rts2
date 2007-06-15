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

  isExposing = false;

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
  if (getIsExposing () || !exposureCount)
    return;
  if (exposureCount > 0)
    exposureCount--;
  connection->
    queCommand (new
		Rts2CommandExposure (connection->getMaster (), this,
				     0, exposureT, exposureTime));
  setIsExposing (true);
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
  char imageFilter[4];
  strncpy (imageFilter, getValueSelection ("filter", camFilter), 4);
  imageFilter[4] = '\0';
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
      imageProceRes res = processImage (images);
      if (res == IMAGE_KEEP_COPY)
	images = NULL;
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

imageProceRes Rts2DevClientCameraImage::processImage (Rts2Image * image)
{
  return IMAGE_DO_BASIC_PROCESSING;
}

void
Rts2DevClientCameraImage::exposureFailed (int status)
{
  setIsExposing (false);
  readoutEnd ();		// pretend we can expose new image after failure..
}

void
Rts2DevClientCameraImage::exposureStarted ()
{
  if (images)
    delete images;
  exposureTime = getValueDouble ("exposure");
  struct timeval expStart;
  const char *focuser;
  gettimeofday (&expStart, NULL);
  images = createImage (&expStart);
  images->setExposureLength (exposureTime);

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
  if (!getIsExposing ())
    return Rts2DevClientCamera::exposureEnd ();
  images->writeClient (this);
  setIsExposing (false);
  if (exposureT != EXP_DARK)
    unblockWait ();
  connection->
    queCommand (new Rts2CommandReadout (connection->getMaster (), 0));
//  connection->
//    queCommand (new Rts2CommandExposure (connection->getMaster (), this, 0, exposureT, exposureTime, true));
//  setIsExposing (true);
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
      double infotime;
      image = (Rts2Image *) event->getArg ();
      image->setMountName (connection->getName ());
      getEqu (&object);
      getObs (&obs);
      image->writeClient (this);
      infotime = getValueDouble ("infotime");
      image->setValue ("MNT_INFO", infotime,
		       "time when mount informations were collected");
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
  tel->ra = getValueDouble ("CUR_RA");
  tel->dec = getValueDouble ("CUR_DEC");
}

void
Rts2DevClientTelescopeImage::getEquTel (struct ln_equ_posn *tel)
{
  tel->ra = getValueDouble ("MNT_RA");
  tel->dec = getValueDouble ("MNT_DEC");
}

void
Rts2DevClientTelescopeImage::getEquTar (struct ln_equ_posn *tar)
{
  tar->ra = getValueDouble ("RASC");
  tar->dec = getValueDouble ("DECL");
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
  obs->lng = getValueDouble ("LONG");
  obs->lat = getValueDouble ("LAT");
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
      image->writeClient (this);
      break;
    }
  Rts2DevClientFocus::postEvent (event);
}

Rts2DevClientWriteImage::Rts2DevClientWriteImage (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
}

void
Rts2DevClientWriteImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_WRITE_TO_IMAGE:
      Rts2Image * image;
      image = (Rts2Image *) event->getArg ();
      image->writeClient (this);
      break;
    }
  Rts2DevClient::postEvent (event);
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
