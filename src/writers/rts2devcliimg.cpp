#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2devcliimg.h"
#include "../utils/config.h"

Rts2DevClientCameraImage::Rts2DevClientCameraImage (Rts2Conn * in_connection):Rts2DevClientCamera
  (in_connection)
{
  images = NULL;
  chipNumbers = 0;
}

void
Rts2DevClientCameraImage::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SET_TARGET_ID:
      activeTargetId = (int) event->getArg ()break;
    }
  Rts2DevClientCamera::postEvent (event);
}

void
Rts2DevClientCameraImage::dataReceived (Rts2ClientTCPDataConn * dataConn)
{
  Rts2DevClientCamera::dataReceived (dataConn);
  if (images)
    {
      images->writeDate (dataConn);
      delete images;
      images = NULL;
    }
}

void
Rts2DevClientCameraImage::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("img_chip"))
    {
      // starting exposure
      if (chipNumbers == 0
	  && (state->value & CAM_MASK_EXPOSE) == CAM_EXPOSING)
	{
	  struct timeval expStart;
	  struct tm expT;
	  char fn[20];
	  gettimeofday (&expStart, NULL);
	  gmtime_r (&expStart.tv_sec, &expT);
	  strftime (fn, 20, "%H%M%S.fits", &expT);
	  delete images;
	  images = new Rts2Image (fn, &expStart);
	  connection->
	    postMaster (new Rts2Event (EVENT_WRITE_TO_IMAGE, images));
	  images->setValue ("CCD_TEMP", getValueChar ("ccd_temperature"),
			    "CCD temperature");
	  images->setValue ("CCD_NAME", connection->getName (),
			    "camera name");
	}
    }
  Rts2DevClientCamera::stateChanged (state);
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
      image = (Rts2Image *) event->getArg ();
      image->setValue ("TEL_NAME", connection->getName (),
		       "name of telescope");
      image->setValue ("TEL_TYPE", getValueChar ("type"), "telescope type");
      image->setValue ("RA", getValueDouble ("ra"), "telescope RA");
      image->setValue ("DEC", getValueDouble ("dec"), "telescope DEC");
      break;
    }

  Rts2DevClientTelescope::postEvent (event);
}

Rts2DevClientDomeImage::Rts2DevClientDomeImage (Rts2Conn * in_connection):Rts2DevClientDome
  (in_connection)
{
}

void
Rts2DevClientDomeImage::postEvent (Rts2Event * event)
{
  Rts2DevClientDome::postEvent (event);
}

Rts2DevClientFocusImage::Rts2DevClientFocusImage (Rts2Conn * in_connection):Rts2DevClientFocus
  (in_connection)
{
}

void
Rts2DevClientFocusImage::postEvent (Rts2Event * event)
{
  Rts2DevClientFocus::postEvent (event);
}
