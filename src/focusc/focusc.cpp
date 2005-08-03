#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*!
 * Command-line fits writer.
 * $Id$
 *
 * Client to test camera deamon.
 *
 * @author petr
 */

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2dataconn.h"

#include "../writers/rts2image.h"
#include "../writers/rts2devclifoc.h"

#include "status.h"
#include "imghdr.h"

#include <vector>
#include <iostream>

#define EXPOSURE_TIME	5.0

#define EVENT_START_EXPOSURE	1000
#define EVENT_STOP_EXPOSURE	1001

class Rts2focusc:public Rts2Client
{
private:
  std::vector < char *>cameraNames;
  float defExposure;
  exposureType exposureT;
  char *focExe;
protected:
    virtual void help ();
public:
    Rts2focusc (int argc, char **argv);

  virtual int processOption (int in_opt);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual int run ();

  float getDefaultExposure ()
  {
    return defExposure;
  }

  exposureType getExposureType ();
};

class Rts2focuscCamera:public Rts2DevClientCameraFoc
{
private:
  Rts2focusc * master;
protected:
  virtual void queExposure ()
  {
    exposureT = master->getExposureType ();
    Rts2DevClientCameraFoc::queExposure ();
  }
public:
    Rts2focuscCamera (Rts2Conn * in_connection, Rts2focusc * in_master,
		      const char *in_exe);

  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
      case EVENT_START_EXPOSURE:
	exposureCount = (exe != NULL) ? 1 : -1;
	queExposure ();
	break;
      case EVENT_STOP_EXPOSURE:
	exposureCount = 0;
	break;
      }
    Rts2DevClientCameraFoc::postEvent (event);
  }

  virtual void stateChanged (Rts2ServerState * state);
};

void
Rts2focusc::help ()
{
  Rts2Client::help ();
  std::cout << "Examples:" << std::endl
    <<
    "\trts2-focusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1"
    << std::endl;
}

Rts2focusc::Rts2focusc (int argc, char **argv):
Rts2Client (argc, argv)
{
  defExposure = 10.0;
  exposureT = EXP_LIGHT;

  focExe = NULL;

  addOption ('d', "device", 1,
	     "camera device name(s) (multiple for multiple cameras)");
  addOption ('e', "exposure", 1, "exposure (defults to 10 sec)");
  addOption ('s', "secmod", 1, "exposure every UT sec");
  addOption ('D', "dark", 0, "create dark images");
  addOption ('F', "imageprocess", 1,
	     "image processing script (default to NULL - no image processing will be done");
}

int
Rts2focusc::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'd':
      cameraNames.push_back (optarg);
      break;
    case 'e':
      defExposure = atof (optarg);
      break;
    case 'D':
      exposureT = EXP_DARK;
      break;
    case 'F':
      focExe = optarg;
      break;
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
}

Rts2DevClient *
Rts2focusc::createOtherType (Rts2Conn * conn, int other_device_type)
{
  std::vector < char *>::iterator cam_iter;
  switch (other_device_type)
    {
    case DEVICE_TYPE_CCD:
      Rts2focuscCamera * cam;
      cam = new Rts2focuscCamera (conn, this, focExe);
      // post exposure event..if name agree
      for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end ();
	   cam_iter++)
	{
	  if (!strcmp (*cam_iter, conn->getName ()))
	    {
	      printf ("Get conn: %p\n", conn);
	      cam->postEvent (new Rts2Event (EVENT_START_EXPOSURE));
	    }
	}
      return cam;
    case DEVICE_TYPE_MOUNT:
      return new Rts2DevClientTelescopeImage (conn);
    case DEVICE_TYPE_FOCUS:
      return new Rts2DevClientFocusImage (conn);
    case DEVICE_TYPE_DOME:
      return new Rts2DevClientDomeImage (conn);
    default:
      return Rts2Client::createOtherType (conn, other_device_type);
    }
}

int
Rts2focusc::run ()
{
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 136"));
  return Rts2Client::run ();
}

exposureType Rts2focusc::getExposureType ()
{
  return exposureT;
}

Rts2focuscCamera::Rts2focuscCamera (Rts2Conn * in_connection, Rts2focusc * in_master, const char *in_exe):Rts2DevClientCameraFoc
  (in_connection,
   in_exe)
{
  exposureTime = in_master->getDefaultExposure ();
  master = in_master;
}

void
Rts2focuscCamera::stateChanged (Rts2ServerState * state)
{
  Rts2DevClientCameraFoc::stateChanged (state);
  if (state->isName ("img_chip"))
    {
      std::cout << connection->getName () << " state: " << state->
	getValue () << std::endl;
    }
}

Rts2focusc *client;

int
main (int argc, char **argv)
{
  int ret;

  client = new Rts2focusc (argc, argv);
  ret = client->init ();
  if (ret)
    {
      std::cerr << "Cannot init focuser client" << std::endl;
      delete client;
      return ret;
    }
  client->run ();
  delete client;
  return 0;
}
