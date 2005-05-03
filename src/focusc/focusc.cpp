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
#include "../writers/rts2devcliimg.h"

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
protected:
    virtual void help ();
  virtual Rts2ConnClient *createClientConnection (char *in_deviceName);

public:
    Rts2focusc (int argc, char **argv);

  virtual int processOption (int in_opt);

  virtual int run ();

  float getDefaultExposure ()
  {
    return defExposure;
  }
};

class Rts2focuscConn:public Rts2ConnClient
{
private:
  Rts2focusc * master;
protected:
  virtual void setOtherType (int other_device_type);
public:
    Rts2focuscConn (Rts2focusc * in_master,
		    char *in_name):Rts2ConnClient (in_master, in_name)
  {
    master = in_master;
  }
};

class Rts2focuscCamera:public Rts2DevClientCameraImage
{
public:
  Rts2focuscCamera (Rts2focuscConn * in_connection, Rts2focusc * in_master);

  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
      case EVENT_START_EXPOSURE:
	exposureEnabled = 1;
	queExposure ();
	break;
	case EVENT_STOP_EXPOSURE:exposureEnabled = 0;
	break;
      }
    Rts2DevClientCameraImage::postEvent (event);
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

Rts2ConnClient *
Rts2focusc::createClientConnection (char *in_deviceName)
{
  Rts2focuscConn *conn;
  conn = new Rts2focuscConn (this, in_deviceName);
  return conn;
}

Rts2focusc::Rts2focusc (int argc, char **argv):Rts2Client (argc, argv)
{

  addOption ('d', "device", 1,
	     "camera device name(s) (multiple for multiple cameras)");
  addOption ('e', "exposure", 1, "exposure (defults to 10 sec)");
//  addOption ('D', "date", 1, "stamp with date");
//  addOption ('s', "secmod", 1, "exposure every UT sec");
  addOption ('I', "inc", 0, "add number of exposures");

  defExposure = 10.0;
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
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
}

int
Rts2focusc::run ()
{
  // find camera names..
  std::vector < char *>::iterator cam_iter;
  for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end ();
       cam_iter++)
    {
      Rts2Conn *conn;
      conn = getConnection ((*cam_iter));
      conn->postEvent (new Rts2Event (EVENT_START_EXPOSURE));
    }
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 136"));
  return Rts2Client::run ();
}

void
Rts2focuscConn::setOtherType (int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_CCD:
      otherDevice = new Rts2focuscCamera (this, master);
      break;
    default:
      Rts2ConnClient::setOtherType (other_device_type);
    }
}

Rts2focuscCamera::Rts2focuscCamera (Rts2focuscConn * in_connection, Rts2focusc * in_master):Rts2DevClientCameraImage
  (in_connection)
{
  exposureTime = in_master->getDefaultExposure ();
}

void
Rts2focuscCamera::stateChanged (Rts2ServerState * state)
{
  Rts2DevClientCameraImage::stateChanged (state);
  if (state->isName ("priority"))
    {
      if (state->value == 1)
	{
	  if (!exposureEnabled)
	    {
	      exposureEnabled = 1;
	      queExposure ();
	    }
	}
      else
	exposureEnabled = 0;
    }
  if (state->isName ("img_chip"))
    {
      std::cout << connection->getName () << " state: " << state->
	value << std::endl;
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
