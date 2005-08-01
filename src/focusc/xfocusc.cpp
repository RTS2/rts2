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
#include "../writers/rts2devclifoc.h"

#include "status.h"
#include "imghdr.h"

#include <limits.h>

#define PP_NEG

#define GAMMA 0.995

#define PP_MED 0.60
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
  XColor rgb[260];		// <= 255 - images, 256 - red line
  Colormap colormap;
    std::vector < char *>cameraNames;
  float defExposure;
  char *displayName;
  int defCenter;
  int defBin;

  // X11 stuff
  Display *display;
  Visual *visual;
  int depth;

  int autoSave;
  int centerHeight;
  int centerWidth;

  int crossType;

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

class Rts2xfocusCamera:public Rts2DevClientCameraFoc
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

  int windowHeight;
  int windowWidth;

  // when 1, we are in focusing mode - not start next exposure before focus is calculated & changed
  int focusing;

  int pixmapHeight;
  int pixmapWidth;

  void buildWindow ();
  void rebuildWindow ();
  void redraw ();
  void drawCenterCross (int xc, int yc);
  void drawCross1 ();
  void drawCross2 ();
  void drawCross3 ();
  void printInfo ();
  // thread entry function..
  void XeventLoop ();
  static void *staticXEventLoop (void *arg)
  {
    ((Rts2xfocusCamera *) arg)->XeventLoop ();
    return NULL;
  }

  int histogram[HISTOGRAM_LIMIT];

  unsigned short low, med, hig;
  double average;
  struct imghdr *lastHeader;

  virtual void getPriority ();
  virtual void lostPriority ();

  int crossType;

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
	exposureCount = focusing ? 1 : -1;
	// build window etc..
	buildWindow ();
	queExposure ();
	break;
      case EVENT_STOP_EXPOSURE:
	exposureCount = 0;
	break;
      }
    Rts2DevClientCameraFoc::postEvent (event);
  }

  virtual void dataReceived (Rts2ClientTCPDataConn * dataConn);
  virtual void stateChanged (Rts2ServerState * state);
  virtual Rts2Image *createImage (const struct timeval *expStart);
  void center (int centerWidth, int centerHeight);
  void setCrossType (int in_crossType);
};

Rts2xfocusCamera::Rts2xfocusCamera (Rts2Conn * in_connection, Rts2xfocus * in_master):Rts2DevClientCameraFoc
  (in_connection)
{
  master = in_master;

  window = 0L;
  pixmap = 0L;
  gc = 0L;
  windowHeight = 800;
  windowWidth = 800;

  pixmapHeight = windowHeight;
  pixmapWidth = windowWidth;

  exposureTime = master->defaultExpousure ();

  lastHeader = NULL;

  average = 0;

  low = med = hig = 0;
}

void
Rts2xfocusCamera::buildWindow ()
{
  XTextProperty window_title;
  char *cameraName;

  if (!exposureCount)
    return;

  window =
    XCreateWindow (master->getDisplay (),
		   DefaultRootWindow (master->getDisplay ()), 0, 0, 100, 100,
		   0, master->getDepth (), InputOutput, master->getVisual (),
		   0, &xswa);
  pixmap =
    XCreatePixmap (master->getDisplay (), window, windowHeight, windowWidth,
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

// called to 
void
Rts2xfocusCamera::rebuildWindow ()
{
  if (gc)
    XFreeGC (master->getDisplay (), gc);
  if (pixmap)
    XFreePixmap (master->getDisplay (), pixmap);
  pixmap =
    XCreatePixmap (master->getDisplay (), window, windowHeight, windowWidth,
		   master->getDepth ());

  gc = XCreateGC (master->getDisplay (), pixmap, 0, &gvc);
}

void
Rts2xfocusCamera::drawCenterCross (int xc, int yc)
{
  // draw cross at center
  XDrawLine (master->getDisplay (), pixmap, gc, xc - 10, yc, xc - 2, yc);
  XDrawLine (master->getDisplay (), pixmap, gc, xc + 10, yc, xc + 2, yc);
  XDrawLine (master->getDisplay (), pixmap, gc, xc, yc - 10, xc, yc - 2);
  XDrawLine (master->getDisplay (), pixmap, gc, xc, yc + 10, xc, yc + 2);
}

void
Rts2xfocusCamera::drawCross1 ()
{
  XSetForeground (master->getDisplay (), gc, master->getRGB (256)->pixel);
  int rectNum;
  int i;
  int xc, yc;
  XRectangle *rectangles;

  rectNum =
    (pixmapWidth / 40 >
     pixmapHeight / 40) ? pixmapHeight / 40 : pixmapWidth / 40;
  rectangles = new XRectangle[rectNum];

  XRectangle *rect = rectangles;

  xc = pixmapWidth / 2;
  yc = pixmapHeight / 2;

  drawCenterCross (xc, yc);

  for (i = 0; i < rectNum;)
    {
      i++;
      xc -= 20;
      yc -= 20;
      rect->x = xc;
      rect->y = yc;
      rect->width = i * 40;
      rect->height = i * 40;
      rect++;
    }
  XDrawRectangles (master->getDisplay (), pixmap, gc, rectangles, rectNum);

  delete[]rectangles;
}

void
Rts2xfocusCamera::drawCross2 ()
{
  XSetForeground (master->getDisplay (), gc, master->getRGB (256)->pixel);
  int arcNum;
  int i;
  int xc, yc;
  XArc *arcs;

  arcNum =
    (pixmapWidth / 40 >
     pixmapHeight / 40) ? pixmapHeight / 40 : pixmapWidth / 40;
  arcs = new XArc[arcNum];

  XArc *arc = arcs;

  xc = pixmapWidth / 2;
  yc = pixmapHeight / 2;

  drawCenterCross (xc, yc);

  for (i = 0; i < arcNum;)
    {
      i++;
      xc -= 20;
      yc -= 20;
      arc->x = xc;
      arc->y = yc;
      arc->width = i * 40;
      arc->height = i * 40;
      arc->angle1 = 0;
      arc->angle2 = 23040;
      arc++;
    }
  XDrawArcs (master->getDisplay (), pixmap, gc, arcs, arcNum);

  delete[]arcs;
}

void
Rts2xfocusCamera::drawCross3 ()
{
  static XPoint points[5];
  int xc, yc;

  XSetForeground (master->getDisplay (), gc, master->getRGB (256)->pixel);
  // draw lines on surrounding
  int w = pixmapWidth / 7;
  int h = pixmapHeight / 7;
  XDrawLine (master->getDisplay (), pixmap, gc, 0, 0, w, h);
  XDrawLine (master->getDisplay (), pixmap, gc, pixmapWidth, 0,
	     pixmapWidth - w, h);
  XDrawLine (master->getDisplay (), pixmap, gc, 0, pixmapHeight, w,
	     pixmapHeight - h);
  XDrawLine (master->getDisplay (), pixmap, gc, pixmapWidth, pixmapHeight,
	     pixmapWidth - w, pixmapHeight - h);
  XDrawRectangle (master->getDisplay (), pixmap, gc, pixmapWidth / 4,
		  pixmapHeight / 4, pixmapWidth / 2, pixmapHeight / 2);
  // draw center..
  xc = pixmapWidth / 2;
  yc = pixmapHeight / 2;
  w = pixmapWidth / 10;
  h = pixmapHeight / 10;
  points[0].x = xc - w;
  points[0].y = yc;

  points[1].x = xc;
  points[1].y = yc + h;

  points[2].x = xc + w;
  points[2].y = yc;

  points[3].x = xc;
  points[3].y = yc - h;

  points[4].x = xc - w;
  points[4].y = yc;

  XDrawLines (master->getDisplay (), pixmap, gc, points, 5, CoordModeOrigin);
  XDrawLine (master->getDisplay (), pixmap, gc, xc, yc - pixmapHeight / 15,
	     xc, yc + pixmapHeight / 15);
}

void
Rts2xfocusCamera::printInfo ()
{
  char *stringBuf;
  int len;
  len =
    asprintf (&stringBuf, "L: %d M: %d H: %d Avg: %.2f", low, med, hig,
	      average);
  XDrawString (master->getDisplay (), pixmap, gc, pixmapWidth / 2 - 100, 20,
	       stringBuf, len);
  free (stringBuf);
  if (lastHeader)
    {
      len =
	asprintf (&stringBuf,
		  "[%i,%i:%i,%i] binn: %i:%i exposureTime: %.3f s",
		  lastHeader->x, lastHeader->y, lastHeader->sizes[0],
		  lastHeader->sizes[1], lastHeader->binnings[0],
		  lastHeader->binnings[1], exposureTime);
      XDrawString (master->getDisplay (), pixmap, gc, pixmapWidth / 2 - 150,
		   pixmapHeight - 20, stringBuf, len);
      free (stringBuf);
    }
}

void
Rts2xfocusCamera::redraw ()
{
  // do some line-drawing etc..
  int ret;

  switch (crossType)
    {
    case 1:
      drawCross1 ();
      break;
    case 2:
      drawCross2 ();
      break;
    case 3:
      drawCross3 ();
      break;
    }
  if (crossType > 0)
    printInfo ();
  xswa.colormap = *(master->getColormap ());
  xswa.background_pixmap = pixmap;

  XChangeWindowAttributes (master->getDisplay (), window,
			   CWColormap | CWBackPixmap, &xswa);

  XClearWindow (master->getDisplay (), window);
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
	  if (pixmap && gc)
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
	      connection->queCommand (new Rts2Command (master, "center 0"));
	      break;
	    case XK_p:
	      master->postEvent (new Rts2Event (EVENT_INTEGRATE_START));
	      break;
	    case XK_o:
	      master->postEvent (new Rts2Event (EVENT_INTEGRATE_STOP));
	      break;
	    case XK_y:
	      setSaveImage (1);
	      break;
	    case XK_u:
	      setSaveImage (0);
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
  int dataSize;
  int i, j, k;
  unsigned short *im_ptr;

  // get to upper classes as well
  Rts2DevClientCameraFoc::dataReceived (dataConn);

  header = dataConn->getImageHeader ();
  pixmapWidth = header->sizes[0];
  pixmapHeight = header->sizes[1];
  if (pixmapWidth > windowWidth || pixmapHeight > windowHeight)
    {
      windowWidth = pixmapWidth;
      windowHeight = pixmapHeight;
      rebuildWindow ();
    }
  std::
    cout << "Get data : [" << pixmapWidth << "x" << pixmapHeight << "]" <<
    std::endl;
  // draw window with image..
  if (!image)
    {
      image =
	XCreateImage (master->getDisplay (), master->getVisual (),
		      master->getDepth (), ZPixmap, 0, 0, pixmapWidth,
		      pixmapHeight, 8, 0);
      image->data = new char[image->bytes_per_line * pixmapHeight];
    }

  // build histogram
  memset (histogram, 0, sizeof (int) * HISTOGRAM_LIMIT);
  dataSize = pixmapHeight * pixmapWidth;
  k = 0;
  im_ptr = dataConn->getData ();
  average = 0;
  for (i = 0; i < pixmapHeight; i++)
    for (j = 0; j < pixmapWidth; j++)
      {
	histogram[*im_ptr]++;
	average += *im_ptr;
	im_ptr++;
      }

  average /= dataSize;

  low = med = hig = 0;
  j = 0;
  for (i = 0; i < HISTOGRAM_LIMIT; i++)
    {
      j += histogram[i];
      if ((!low) && (((float) j / (float) dataSize) > PP_LOW))
	low = i;
      if ((!med) && (((float) j / (float) dataSize) > PP_MED))
	med = i;
      if ((!hig) && (((float) j / (float) dataSize) > PP_HIG))
	hig = i;
    }
  if (!hig)
    hig = 65536;
  if (low == hig)
    low = hig / 2 - 1;

  im_ptr = dataConn->getData ();

  for (j = 0; j < pixmapHeight; j++)
    for (i = 0; i < pixmapWidth; i++)
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
		       getRGB ((int) (255 * (val - low) / (hig - low)))->
		       pixel);
	  }
      }

  std::
    cout << "Data low:" << low << " med:" << med << " hig:" << hig <<
    " average:" << average << std::endl;

  XResizeWindow (master->getDisplay (), window, pixmapWidth, pixmapHeight);

  XPutImage (master->getDisplay (), pixmap, gc, image, 0, 0, 0, 0,
	     pixmapWidth, pixmapHeight);

  if (!lastHeader)
    lastHeader = new imghdr;
  memcpy (lastHeader, header, sizeof (imghdr));

  redraw ();
  XFlush (master->getDisplay ());
}

void
Rts2xfocusCamera::getPriority ()
{
  if (exposureCount)
    queExposure ();
}

void
Rts2xfocusCamera::lostPriority ()
{
  std::cout << "Priority lost" << std::endl;
  exposureCount = 0;
}

void
Rts2xfocusCamera::stateChanged (Rts2ServerState * state)
{
  std::cout << "State changed:" << state->getName () << " value:" << state->
    getValue () << std::endl;
  Rts2DevClientCameraFoc::stateChanged (state);
}

Rts2Image *
Rts2xfocusCamera::createImage (const struct timeval *expStart)
{
  Rts2Image *image;
  char *filename;
  asprintf (&filename, "!/tmp/%s_%i.fits", connection->getName (), getpid ());
  image = new Rts2Image (filename, expStart);
  free (filename);
  return image;
}

void
Rts2xfocusCamera::center (int centerWidth, int centerHeight)
{
  connection->
    queCommand (new Rts2CommandCenter (master, 0, centerWidth, centerHeight));
}

void
Rts2xfocusCamera::setCrossType (int in_crossType)
{
  crossType = in_crossType;
}

Rts2xfocus::Rts2xfocus (int argc, char **argv):
Rts2Client (argc, argv)
{
  defExposure = 10;
  displayName = NULL;
  defCenter = 0;
  defBin = -1;

  autoSave = 0;

  centerWidth = -1;
  centerHeight = -1;

  crossType = 1;

  addOption ('d', "device", 1,
	     "camera device name(s) (multiple for multiple cameras)");
  addOption ('e', "exposure", 1, "exposure (defaults to 10 sec)");
  addOption ('s', "save", 1, "save filenames (default don't save");
  addOption ('a', "autodark", 1, "take and use dark frame");
  addOption ('x', "display", 1, "name of X display");
  addOption ('c', "center", 0, "takes only center images");
  addOption ('b', "binning", 1,
	     "default binning (ussually 1, depends on camera setting)");
  addOption ('W', "width", 1, "center width");
  addOption ('H', "height", 1, "center height");
  addOption ('X', "cross", 1,
	     "cross type (default to 1; possible values 0 - no cross, 1 - rectangles\n"
	     "    2 - circles, 3 - BOOTES special");
}

Rts2xfocus::~Rts2xfocus (void)
{
  if (display)
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
    << "\tc     .. center (1/2x1/2 chip size) exposure" << std::endl
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
    case 's':
      autoSave = 1;
      break;
    case 'x':
      displayName = optarg;
      break;
    case 'c':
      defCenter = 1;
      break;
    case 'b':
      defBin = atoi (optarg);
      break;
    case 'W':
      centerWidth = atoi (optarg);
      break;
    case 'H':
      centerHeight = atoi (optarg);
      break;
    case 'X':
      crossType = atoi (optarg);
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

      ret = XAllocColor (display, colormap, rgb + i);
    }
  rgb[256].red = USHRT_MAX;
  rgb[256].green = 0;
  rgb[256].blue = 0;
  rgb[256].flags = DoRed | DoGreen | DoBlue;
  ret = XAllocColor (display, colormap, &rgb[256]);
  return 0;
}

Rts2DevClient *
Rts2xfocus::createOtherType (Rts2Conn * conn, int other_device_type)
{
  std::vector < char *>::iterator cam_iter;
  switch (other_device_type)
    {
    case DEVICE_TYPE_CCD:
      Rts2xfocusCamera * cam;
      cam = new Rts2xfocusCamera (conn, this);
      cam->setSaveImage (autoSave);
      cam->setCrossType (crossType);
      if (defCenter)
	{
	  cam->center (centerWidth, centerHeight);
	}
      if (defBin > 0)
	{
	  conn->queCommand (new Rts2CommandBinning (this, defBin, defBin));
	}
      // post exposure event..if name agree
      for (cam_iter = cameraNames.begin (); cam_iter != cameraNames.end ();
	   cam_iter++)
	{
	  if (!strcmp (*cam_iter, conn->getName ()))
	    {
	      printf ("Get conn: %s\n", conn->getName ());
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
Rts2xfocus::run ()
{
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
