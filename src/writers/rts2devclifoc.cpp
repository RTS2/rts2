#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2devclifoc.h"

#include <iostream>

Rts2DevClientCameraFoc::Rts2DevClientCameraFoc (Rts2Conn * in_connection):Rts2DevClientCameraImage
  (in_connection)
{
}

void
Rts2DevClientCameraFoc::processImage (Rts2Image * image)
{
  // create focus connection
  int ret;

  Rts2DevClientCameraImage::processImage (image);

  Rts2ConnFocus *focCon = new Rts2ConnFocus (this, image);
  ret = focCon->init ();
  if (ret)
    {
      delete focCon;
    }
  // after we finish, we will call focus routines..
  connection->getMaster ()->addConnection (focCon);
}

Rts2ConnFocus::Rts2ConnFocus (Rts2DevClientCameraFoc * in_camera, Rts2Image * in_image):Rts2ConnFork (in_camera->getMaster (),
	      "/home/petr/foc-test")
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
    }
  return -1;
}
