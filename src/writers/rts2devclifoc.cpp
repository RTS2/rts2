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
}

Rts2DevClientCameraFoc::~Rts2DevClientCameraFoc (void)
{
  if (exe)
    delete exe;
}

void
Rts2DevClientCameraFoc::queExposure ()
{
  if (isFocusing)
    return;
  Rts2DevClientCameraImage::queExposure ();
}

void
Rts2DevClientCameraFoc::postEvent (Rts2Event * event)
{
  Rts2DevClientFocusFoc *focuser;
  const char *focName;
  char *cameraFoc;
  switch (event->getType ())
    {
    case EVENT_FOCUSING_END:
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

  if (exe)
    {
      Rts2ConnFocus *focCon = new Rts2ConnFocus (this, image, exe);
      ret = focCon->init ();
      if (ret)
	{
	  delete focCon;
	}
      // after we finish, we will call focus routines..
      connection->getMaster ()->addConnection (focCon);
    }
}

void
Rts2DevClientCameraFoc::changeFocus (int steps)
{
  Rts2Conn *focuser;
  focuser = getMaster ()->getOpenConnection (getValueChar ("focuser"));
  if (focuser)
    {
      focuser->
	postEvent (new Rts2Event (EVENT_START_FOCUSING, (void *) &steps));
      isFocusing = 1;
    }
}

Rts2DevClientFocusFoc::Rts2DevClientFocusFoc (Rts2Conn * in_connection):Rts2DevClientFocus
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
  Rts2DevClientFocus::postEvent (event);
}

void
Rts2DevClientFocusFoc::focusingEnd ()
{
  Rts2DevClientFocus::focusingEnd ();
  connection->getMaster ()->
    postEvent (new Rts2Event (EVENT_FOCUSING_END, (void *) this));
}

Rts2ConnFocus::Rts2ConnFocus (Rts2DevClientCameraFoc * in_camera,
			      Rts2Image * in_image, const char *in_exe):
Rts2ConnFork (in_camera->getMaster (), in_exe)
{
  camera = in_camera;
  img_path = new char[strlen (in_image->getImageName ()) + 1];
  strcpy (img_path, in_image->getImageName ());
}

Rts2ConnFocus::~Rts2ConnFocus (void)
{
  delete[]img_path;
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
  int change;
  ret = sscanf (getCommand (), "change %i %i", &id, &change);
  if (ret == 2)
    {
      std::cout << "Get change: " << id << " " << change << std::endl;
      camera->changeFocus (change);
      // post it to focuser
    }
  return -1;
}
