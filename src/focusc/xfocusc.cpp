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

#include <pthread.h>

#include <math.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include "../utils/rts2block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2dataconn.h"

#include "../writers/rts2image.h"
#include "../writers/rts2devcliimg.h"

#include "status.h"
#include "imghdr.h"

#define PP_NEG

#define GAMMA 0.995

//#define PP_MED 0.60
#define PP_HIG 0.995
#define PP_LOW (1 - PP_HIG)

#define HISTOGRAM_LIMIT		65536

// events types
#define EVENT_START_EXPOSURE	1000
#define EVENT_STOP_EXPOSURE	1001

#define EVENT_INTEGRATE_START   1002
#define EVENT_INTEGRATE_STOP    1003

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

public:
    Rts2xfocus (int argc, char **argv);
    virtual ~ Rts2xfocus (void);

  virtual int processOption (int in_opt);

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual int init ();
  virtual int run ();

  float defaultExpousure ()
  {
    return defExposure;
  }

  Display *getDisplay ()
  {
    return display;
  }
  int getDepth ()
  {
    return depth;
  }
  Visual *getVisual ()
  {
    return visual;
  }

  XColor *getRGB (int i)
  {
    return &rgb[i];
  }
  Colormap *getColormap ()
  {
    return &colormap;
  }
};

class Rts2xfocusCamera:public Rts2DevClientCameraImage
{
private:
  Rts2xfocus * master;

  pthread_t XeventThread;

  // X11 stuff
  Window window;
  GC gc;
  XGCValues gvc;
  Pixmap pixmap;
  XImage *image;
  XSetWindowAttributes xswa;

  void buildWindow ();
  void redraw ();
  // thread entry function..
  void XeventLoop ();
  static void *staticXEventLoop (void *arg)
  {
    ((Rts2xfocusCamera *) arg)->XeventLoop ();
    return NULL;
  }

  int histogram[HISTOGRAM_LIMIT];

public:
  Rts2xfocusCamera (Rts2Conn * in_connection, Rts2xfocus * in_master);
  virtual ~ Rts2xfocusCamera (void)
  {
    pthread_cancel (XeventThread);
    pthread_join (XeventThread, NULL);
  }

  virtual void postEvent (Rts2Event * event)
  {
    switch (event->getType ())
      {
      case EVENT_START_EXPOSURE:
	exposureEnabled = 1;
	queExposure ();
	break;
      case EVENT_STOP_EXPOSURE:
	exposureEnabled = 0;
	break;
      }
    Rts2DevClientCameraImage::postEvent (event);
  }

  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);
  virtual void stateChanged (Rts2ServerState * state);
  virtual Rts2Image *createImage (const struct timeval *expStart)
  {
    return new Rts2Image ("test.fits", expStart);
  }
};

Rts2xfocusCamera::Rts2xfocusCamera (Rts2Conn * in_connection, Rts2xfocus * in_master):Rts2DevClientCameraImage
  (in_connection)
{
  master = in_master;

  exposureTime = master->defaultExpousure ();
  // build window etc..
  buildWindow ();
}

void
Rts2xfocusCamera::buildWindow ()
{
  XTextProperty window_title;
  char *cameraName;

  window =
    XCreateWindow (master->getDisplay (),
		   DefaultRootWindow (master->getDisplay ()), 0, 0, 100, 100,
		   0, master->getDepth (), InputOutput, master->getVisual (),
		   0, &xswa);
  pixmap =
    XCreatePixmap (master->getDisplay (), window, 1200, 1200,
		   master->getDepth ());

  gc = XCreateGC (master->getDisplay (), pixmap, 0, &gvc);
  XSelectInput (master->getDisplay (), window,
		KeyPressMask | ButtonPressMask | ExposureMask);
  XMapRaised (master->getDisplay (), window);

  cameraName = new char[strlen (connection->getName ()) + 1];
  strcpy (cameraName, connection->getName ());

  XStringListToTextProperty (&cameraName, 1, &window_title);
  XSetWMName (master->getDisplay (), window, &window_title);

  pthread_create (&XeventThread, NULL, staticXEventLoop, this);
}

void
Rts2xfocusCamera::redraw ()
{
  // do some line-drawing etc..
}

void
Rts2xfocusCamera::XeventLoop ()
{
  XEvent event;
  KeySym ks;

  while (1)
    {
      XWindowEvent (master->getDisplay (), window,
		    KeyPressMask | ButtonPressMask | ExposureMask, &event);
      switch (event.type)
	{
	case Expose:
	  redraw ();
	  break;
	case KeyPress:
	  ks = XLookupKeysym ((XKeyEvent *) & event, 0);
	  switch (ks)
	    {
	    case XK_1:
	      connection->queCommand (new Rts2CommandBinning (master, 1, 1));
	      break;
	    case XK_2:
	      connection->queCommand (new Rts2CommandBinning (master, 2, 2));
	      break;
	    case XK_3:
	      connection->queCommand (new Rts2CommandBinning (master, 3, 3));
	      break;
	    case XK_e:
	      exposureTime += 1;
	      break;
	    case XK_d:
	      if (exposureTime > 1)
		exposureTime -= 1;
	      break;
	    case XK_w:
	      exposureTime += 0.1;
	      break;
	    case XK_s:
	      if (exposureTime > 0.1)
		exposureTime -= 0.1;
	      break;
	    case XK_q:
	      exposureTime += 0.01;
	      break;
	    case XK_a:
	      if (exposureTime > 0.01)
		exposureTime -= 0.01;
	      break;
	    case XK_f:
	      connection->
		queCommand (new Rts2Command (master, "box 0 -1 -1 -1 -1"));
	      break;
	    case XK_c:
	      connection->queCommand (new Rts2Command (master, "center"));
	      break;
	    case XK_p:
	      master->postEvent (new Rts2Event (EVENT_INTEGRATE_START));
	      break;
	    case XK_o:
	      master->postEvent (new Rts2Event (EVENT_INTEGRATE_STOP));
	      break;
	    case XK_y:
	      saveImage = 1;
	      break;
	    case XK_u:
	      saveImage = 0;
	      break;
	    default:
	      break;
	    }
	  break;
	}
    }
}

void
Rts2xfocusCamera::dataReceived (Rts2ClientTCPDataConn * dataConn)
{
  struct imghdr *header;
  int width, height;
  int dataSize;
  int i, j, k;
  unsigned short *im_ptr;
  unsigned short low, med, hig;

  // get to upper classes as well
  Rts2DevClientCameraImage::dataReceived (dataConn);

  header = dataConn->getImageHeader ();
  width = header->sizes[0];
  height = header->sizes[1];
  // draw window with image..
  if (!image)
    {
      image =
	XCreateImage (master->getDisplay (), master->getVisual (),
		      master->getDepth (), ZPixmap, 0, 0, width, height, 8,
		      0);
      image->data = new char[image->bytes_per_line * height * 16];
    }

  // build histogram
  memset (histogram, 0, sizeof (int) * HISTOGRAM_LIMIT);
  dataSize = height * width;
  k = 0;
  im_ptr = dataConn->getData ();
  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      {
	histogram[*im_ptr]++;
	im_ptr++;
      }

  low = med = hig = 0;
  j = 0;
  for (i = 0; i < HISTOGRAM_LIMIT; i++)
    {
      j += histogram[i];
      if ((!low) && (((float) j / (float) dataSize) > PP_LOW))
	low = i;
#ifdef PP_MED
      if ((!med) && (((float) j / (float) dataSize) > PP_MED))
	med = i;
#endif
      if ((!hig) && (((float) j / (float) dataSize) > PP_HIG))
	hig = i;
    }
  if (!hig)
    hig = 65536;
  if (low == hig)
    low = hig / 2 - 1;

  im_ptr = dataConn->getData ();

  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      {
	unsigned short val;
	val = *im_ptr;
	im_ptr++;
	if (val < low)
	  XPutPixel (image, i, j, master->getRGB (0)->pixel);
	else if (val > hig)
	  XPutPixel (image, i, j, master->getRGB (255)->pixel);
	else
	  {
	    XPutPixel (image, i, j,
		       master->
		       getRGB ((int) (256 * (val - low) / (hig - low)))->
		       pixel);
	  }
      }

  XResizeWindow (master->getDisplay (), window, width, height);

  XPutImage (master->getDisplay (), pixmap, gc, image, 0, 0, 0, 0, width,
	     height);

  xswa.colormap = *(master->getColormap ());
  xswa.background_pixmap = pixmap;

  XChangeWindowAttributes (master->getDisplay (), window,
			   CWColormap | CWBackPixmap, &xswa);

  XClearWindow (master->getDisplay (), window);
  redraw ();
  XFlush (master->getDisplay ());
}

void
Rts2xfocusCamera::stateChanged (Rts2ServerState * state)
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

Rts2DevClient *
Rts2xfocus::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_CCD:
      return new Rts2xfocusCamera (conn, this);
    default:
      return Rts2Client::createOtherType (conn, other_device_type);
    }
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
      conn->postEvent (new Rts2Event (EVENT_START_EXPOSURE));
    }
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 137"));
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
