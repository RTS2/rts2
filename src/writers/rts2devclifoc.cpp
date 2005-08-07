#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2devclifoc.h"

#include <iostream>

Rts2DevClientCameraFoc::Rts2DevClientCameraFoc (Rts2Conn * in_connection,
						const char *in_exe):
Rts2DevClientCameraImage (in_connection)
{
  if (in_exe)
    {
      exe = new char[strlen (in_exe) + 1];
      strcpy (exe, in_exe);
    }
  else
    {
      exe - NULL;
    }
  isFocusing = 0;
  darkImage = NULL;
  focConn = NULL;
  autoDark = 0;
}

Rts2DevClientCameraFoc::~Rts2DevClientCameraFoc (void)
{
  if (exe)
    delete exe;
  if (darkImage)
    delete darkImage;
  if (focConn)
    focConn->nullCamera ();
}

void
Rts2DevClientCameraFoc::queExposure ()
{
  if (isFocusing)
    return;
  if (autoDark)
    {
      if (darkImage)
	exposureT = EXP_LIGHT;
      else
	exposureT = EXP_DARK;
    }
  Rts2DevClientCameraImage::queExposure ();
}

void
Rts2DevClientCameraFoc::postEvent (Rts2Event * event)
{
  Rts2Conn *focus;
  Rts2DevClientFocusFoc *focuser;
  const char *focName;
  char *cameraFoc;
  switch (event->getType ())
    {
    case EVENT_CHANGE_FOCUS:
      focus =
	connection->getMaster ()->
	getOpenConnection (getValueChar ("focuser"));
      if (focus && focConn == (Rts2ConnFocus *) event->getArg ())
	{
	  focusChange (focus, focConn);
	}
      break;
    case EVENT_FOCUSING_END:
      if (!exe)			// don't care about messages from focuser when we don't have focusing script
	break;
      focuser = (Rts2DevClientFocusFoc *) event->getArg ();
      focName = focuser->getName ();
      cameraFoc = getValueChar ("focuser");
      if (focName && cameraFoc
	  && !strcmp (focuser->getName (), getValueChar ("focuser")))
	{
	  isFocusing = 0;
	  exposureCount = 1;
	  queExposure ();
	}
      break;
    }
  Rts2DevClientCameraImage::postEvent (event);
}

void
Rts2DevClientCameraFoc::processImage (Rts2Image * image)
{
  // create focus connection
  int ret;

  Rts2DevClientCameraImage::processImage (image);

  // got requested dark..
  if (image->getType () == IMGTYPE_DARK)
    {
      if (darkImage)
	delete darkImage;
      darkImage = image;
      darkImage->keepImage ();
      exposureCount = 1;
    }
  else if (exe)
    {
      if (darkImage)
	image->substractDark (darkImage);
      image->saveImage ();
      if (focConn)
	focConn->endConnection ();	// master will delete that connection..
      focConn = new Rts2ConnFocus (this, image, exe);
      ret = focConn->init ();
      if (ret)
	{
	  delete focConn;
	  return;
	}
      // after we finish, we will call focus routines..
      connection->getMaster ()->addConnection (focConn);
    }
}

void
Rts2DevClientCameraFoc::focusChange (Rts2Conn * focus,
				     Rts2ConnFocus * focConn)
{
  int change = focConn->getChange ();
  if (change == INT_MAX)
    {
      exposureCount = 1;
      queExposure ();
      return;
    }
  focus->postEvent (new Rts2Event (EVENT_START_FOCUSING, (void *) &change));
  isFocusing = 1;
}

int
Rts2DevClientCameraFoc::addStarData (struct stardata *sr)
{
  if (images)
    return images->addStarData (sr);
  return -1;
}

Rts2DevClientFocusFoc::Rts2DevClientFocusFoc (Rts2Conn * in_connection):Rts2DevClientFocusImage
  (in_connection)
{
}

void
Rts2DevClientFocusFoc::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_START_FOCUSING:
      connection->
	queCommand (new
		    Rts2CommandChangeFocus (this,
					    *((int *) event->getArg ())));
      break;
    }
  Rts2DevClientFocusImage::postEvent (event);
}

void
Rts2DevClientFocusFoc::focusingEnd ()
{
  Rts2DevClientFocus::focusingEnd ();
  connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_FOCUSING_END, (void *) this));
}

Rts2ConnFocus::Rts2ConnFocus (Rts2DevClientCameraFoc * in_client,
			      Rts2Image * in_image, const char *in_exe):
Rts2ConnFork (in_client->getMaster (), in_exe)
{
  change = INT_MAX;
  img_path = new char[strlen (in_image->getImageName ()) + 1];
  strcpy (img_path, in_image->getImageName ());
  cameraName = new char[strlen (in_client->getName ()) + 1];
  strcpy (cameraName, in_client->getName ());
  camera = in_client;
}

Rts2ConnFocus::~Rts2ConnFocus (void)
{
  if (change == INT_MAX)	// we don't get focus change, let's try next image..
    getMaster ()->
      postEvent (new Rts2Event (EVENT_CHANGE_FOCUS, (void *) this));
  delete[]img_path;
  delete[]cameraName;
}

int
Rts2ConnFocus::newProcess ()
{
  syslog (LOG_DEBUG, "Rts2ConnFocus::newProcess exe: %s img: %s", exePath,
	  img_path);
  if (exePath)
    {
      execl (exePath, exePath, img_path, (char *) NULL);
      // when execl fails..
      syslog (LOG_ERR, "Rts2ConnFocus::newProcess: %m");
    }
  return -2;
}

int
Rts2ConnFocus::processLine ()
{
  int ret;
  int id;
  ret = sscanf (getCommand (), "change %i %i", &id, &change);
  if (ret == 2)
    {
      std::cout << "Get change: " << id << " " << change << std::endl;
      if (change == INT_MAX)
	return -1;		// that's not expected .. ignore it
      getMaster ()->
	postEvent (new Rts2Event (EVENT_CHANGE_FOCUS, (void *) this));
      // post it to focuser
    }
  else
    {
      struct stardata sr;
      ret =
	sscanf (getCommand (), " %f %f %f %f", &sr.X, &sr.Y, &sr.F, &sr.fwhm);
      if (ret != 4)
	{
	  std::cout << "Get line: " << getCommand () << std::endl;
	}
      else
	{
	  if (camera)
	    {
	      camera->addStarData (&sr);
	    }
	  std::cout << "Sex added" << std::endl;
	}
    }
  return -1;
}
