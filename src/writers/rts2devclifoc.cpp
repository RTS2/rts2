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
}

Rts2DevClientCameraFoc::~Rts2DevClientCameraFoc (void)
{
  if (exe)
    delete exe;
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

Rts2ConnFocus::Rts2ConnFocus (Rts2DevClientCameraFoc * in_camera,
			      Rts2Image * in_image, const char *in_exe):
Rts2ConnFork (in_camera->getMaster (), in_exe)
{
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
      // post it to focuser
    }
  return -1;
}
