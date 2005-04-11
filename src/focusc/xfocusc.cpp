#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*!
 *
 * Display images from RTS2 camera devices in nifty Xwindow.
 * Allow some basic user interaction.
 *
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <libnova/libnova.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2dataconn.h"

#include "status.h"

#define PP_NEG

#define GAMMA 0.995

//#define PP_MED 0.60
#define PP_HIG 0.995
#define PP_LOW (1 - PP_HIG)

#define HISTOGRAM_LIMIT		65536

class Rts2xfocus:public Rts2Client
{
private:
  XColor rgb[256];
  Colormap colormap;

    std::vector < char *>cameraNames;
  float defExposure;
  char *displayName;

  // X11 stuff
  Display *display;
  Visual *visual;
  int depth;

protected:
    virtual void help ();
  virtual Rts2ConnClient *createClientConnection (char *in_deviceName);

public:
    Rts2xfocus (int argc, char **argv);
    virtual ~ Rts2xfocus (void);

  virtual int processOption (int in_opt);

  virtual int init ();
  virtual int run ();

  float defaultExpousure ()
  {
    return defExposure;
  }
};

class Rts2xfocusConn:public Rts2ConnClient
{
private:
  Rts2xfocus * master;
protected:
  virtual void setOtherType (int other_device_type);
public:
    Rts2xfocusConn (Rts2xfocus * in_master,
		    char *in_name):Rts2ConnClient (in_master, in_name)
  {
    master = in_master;
  }
};

class Rts2xfocusCamera:public Rts2DevClientCamera
{
private:
  float exposureTime;
  int exposureLight;
  int exposureChip;
  int exposureEnabled;
  void queExposure ();

  Rts2xfocus *master;
public:
    Rts2xfocusCamera (Rts2xfocusConn * in_connection, Rts2xfocus * in_master);

  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);
  virtual void stateChanged (Rts2ServerState * state);
};

void
Rts2xfocusConn::setOtherType (int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_CCD:
      otherDevice = new Rts2xfocusCamera (this, master);
      break;
    default:
      Rts2ConnClient::setOtherType (other_device_type);
    }
}

Rts2xfocusCamera::Rts2xfocusCamera (Rts2xfocusConn * in_connection, Rts2xfocus * in_master):Rts2DevClientCamera
  (in_connection)
{
  master = in_master;

  exposureTime = master->defaultExpousure ();
  exposureLight = 1;
  exposureChip = 0;
  // build window etc..
}

void
Rts2xfocusCamera::queExposure ()
{
  if (!exposureEnabled)
    return;
  char *text;
  asprintf (&text, "expose 0 %i %f", exposureLight, exposureTime);
  connection->queCommand (new Rts2Command (master, text));
  free (text);
  exposureEnabled = 0;
}

void
Rts2xfocusCamera::dataReceived (Rts2ClientTCPDataConn * dataConn)
{

}

void
Rts2xfocusCamera::stateChanged (Rts2ServerState * state)
{
  char *text;
  if (state->isName ("priority"))
    {
      if (state->value == 1)
	queExposure ();
      else
	exposureEnabled = 0;
    }
  else if (state->isName ("img_chip"))
    {
      int stateVal;
      std::cout << "New state:" << state->value << std::endl;
      stateVal =
	state->value & (CAM_MASK_EXPOSE | CAM_MASK_READING | CAM_MASK_DATA);
      if (stateVal == (CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NODATA))
	{
	  queExposure ();
	}
      if (stateVal == (CAM_NOEXPOSURE | CAM_NOTREADING | CAM_DATA))
	{
	  exposureEnabled = 1;
	  connection->queCommand (new Rts2Command (master, "readout 0"));
	}
    }
}

Rts2xfocus::Rts2xfocus (int argc, char **argv):
Rts2Client (argc, argv)
{
  defExposure = 10;
  displayName = NULL;

  addOption ('d', "device", 1,
	     "camera device name(s) (multiple for multiple cameras)");
  addOption ('e', "exposure", 1, "exposure (defaults to 10 sec)");
  addOption ('s', "save", 1, "save filenames (default don't save");
  addOption ('a', "autodark", 1, "take and use dark frame");
  addOption ('x', "display", 1, "name of X display");
}

Rts2xfocus::~Rts2xfocus (void)
{
  XCloseDisplay (display);
}

void
Rts2xfocus::help ()
{
  Rts2Client::help ();
  std::cout << "Keys:" << std::endl
    << "\t1,2,3 .. binning 1x1, 2x2, 3x3" << std::endl
    << "\tq,a   .. increase/decrease exposure 0.01 sec" << std::endl
    << "\tw,s   .. increase/decrease exposure 0.1 sec" << std::endl
    << "\te,d   .. increase/decrease exposure 1 sec" << std::endl
    << "\tf     .. full frame exposure" << std::endl
    << "\tc     .. center (256x256) exposure" << std::endl
    << "\ty     .. save fits file\n" << std::endl
    << "\tu     .. don't save fits file\n" << std::endl
    << "Examples:" << std::endl
    <<
    "\trts2-xfocusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1"
    << std::
    endl <<
    "\trts2-xfocusc -d C2 -a -e 10    .. takes 10 sec exposures on device C2. Takes darks and use them"
    << std::endl;
}

Rts2ConnClient *
Rts2xfocus::createClientConnection (char *in_deviceName)
{
  Rts2xfocusConn *conn;
  conn = new Rts2xfocusConn (this, in_deviceName);
  return conn;
}

int
Rts2xfocus::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'd':
      cameraNames.push_back (optarg);
      break;
    case 'e':
      defExposure = atof (optarg);
      break;
    case 'x':
      displayName = optarg;
      break;
    default:
      return Rts2Client::processOption (in_opt);
    }
  return 0;
}

int
Rts2xfocus::init ()
{
  int ret;
  ret = Rts2Client::init ();
  if (ret)
    return ret;

  display = XOpenDisplay (displayName);
  if (!display)
    {
      std::cerr << "Cannot open display" << std::endl;
      return -1;
    }

  depth = DefaultDepth (display, DefaultScreen (display));
  visual = DefaultVisual (display, DefaultScreen (display));

  colormap = DefaultColormap (display, DefaultScreen (display));

  std::cout << "Display opened succesfully" << std::endl;

  // allocate colormap..
  for (int i = 0; i < 256; i++)
    {
      rgb[i].red = (unsigned short) (65536 * (1.0 * i / 256));
      rgb[i].green = (unsigned short) (65536 * (1.0 * i / 256));
      rgb[i].blue = (unsigned short) (65536 * (1.0 * i / 256));
      rgb[i].flags = DoRed | DoGreen | DoBlue;


      XAllocColor (display, colormap, rgb + i);
    }
  return 0;
}

int
Rts2xfocus::run ()
{
  // find camera names..
  std::vector < char *>::iterator cam_iter;
  for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end ();
       cam_iter++)
    {
      Rts2Conn *conn;
      conn = getConnection ((*cam_iter));
    }
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 200"));
  return Rts2Client::run ();
}

int
main (int argc, char **argv)
{
  Rts2xfocus *masterFocus;
  int ret;
  masterFocus = new Rts2xfocus (argc, argv);
  ret = masterFocus->init ();
  if (ret)
    {
      delete masterFocus;
      return 1;
    }
  masterFocus->run ();
  delete masterFocus;
  return 0;
}
