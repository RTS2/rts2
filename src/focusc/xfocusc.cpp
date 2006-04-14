#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*!
 *
 * Display images from RTS2 camera devices in nifty Xwindow.
 * Allow some basic user interaction.
 *
 */

#include "genfoc.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#include <pthread.h>

#include <math.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include <limits.h>

class Rts2xfocus:public Rts2GenFocClient
{
private:
  XColor rgb[260];		// <= 255 - images, 256 - red line
  Colormap colormap;
  char *displayName;

  // X11 stuff
  Display *display;
  Visual *visual;
  int depth;

  int crossType;
  int starsType;

  // initially in arcsec, but converted (and used) in degrees
  double changeVal;

  virtual Rts2GenFocCamera *createFocCamera (Rts2Conn * conn);
public:
    Rts2xfocus (int argc, char **argv);
    virtual ~ Rts2xfocus (void);

  virtual int processOption (int in_opt);

  virtual int init ();
  virtual void help ();

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
  int getStarsType ()
  {
    return starsType;
  }
};

class Rts2xfocusCamera:public Rts2GenFocCamera
{
private:
  Rts2xfocus * master;

  pthread_t XeventThread;

  // X11 stuff
  Window window;
  GC gc;
  XGCValues gvc;
  Pixmap pixmap;
  XImage *ximage;
  XSetWindowAttributes xswa;

  int windowHeight;
  int windowWidth;

  int pixmapHeight;
  int pixmapWidth;

  void buildWindow ();
  void rebuildWindow ();
  void redraw ();
  void drawCenterCross (int xc, int yc);
  void drawCross1 ();
  void drawCross2 ();
  void drawCross3 ();
  void drawStars (Rts2Image * image);
  void printInfo ();
  void printMouse ();
  void redrawMouse ();
  // thread entry function..
  void XeventLoop ();
  static void *staticXEventLoop (void *arg)
  {
    ((Rts2xfocusCamera *) arg)->XeventLoop ();
    return NULL;
  }

  int crossType;
  int lastImage;

  long lastX;
  long lastY;
  long lastSizeX;
  long lastSizeY;
  int binningsX;
  int binningsY;

  int mouseX;
  int mouseY;

  int buttonX;
  int buttonY;
  struct timeval buttonImageTime;

  struct timeval exposureStart;

  double change_val;		// change value in degrees
protected:
  virtual void printFWHMTable ();

public:
  Rts2xfocusCamera (Rts2Conn * in_connection, double in_change_val,
		    Rts2xfocus * in_master);
  virtual ~ Rts2xfocusCamera (void);

  virtual void postEvent (Rts2Event * event);

  virtual void processImage (Rts2Image * image);
  void setCrossType (int in_crossType);
};

Rts2xfocusCamera::Rts2xfocusCamera (Rts2Conn * in_connection,
				    double in_change_val,
				    Rts2xfocus * in_master):
Rts2GenFocCamera (in_connection, in_master)
{
  master = in_master;

  window = 0L;
  pixmap = 0L;
  gc = 0L;
  windowHeight = 800;
  windowWidth = 800;

  pixmapHeight = windowHeight;
  pixmapWidth = windowWidth;

  lastImage = 0;

  mouseX = mouseY = -1;
  buttonX = buttonY = -1;

  timerclear (&buttonImageTime);
  timerclear (&exposureStart);

  change_val = in_change_val;
}

Rts2xfocusCamera::~Rts2xfocusCamera (void)
{
  if (XeventThread)
    {
      pthread_cancel (XeventThread);
      pthread_join (XeventThread, NULL);
    }
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
		KeyPressMask | ButtonPressMask | ExposureMask |
		PointerMotionMask);
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
Rts2xfocusCamera::drawStars (Rts2Image * image)
{
  struct stardata *sr;
  if (!image)
    return;
  sr = image->sexResults;
  for (int i = 0; i < image->sexResultNum; i++, sr++)
    {
      XDrawArc (master->getDisplay (), pixmap, gc, (int) sr->X - 10,
		(int) sr->Y - 10, 20, 20, 0, 23040);
    }
}

void
Rts2xfocusCamera::printInfo ()
{
  char *stringBuf;
  int len;
  XSetBackground (master->getDisplay (), gc, master->getRGB (0)->pixel);
  len =
    asprintf (&stringBuf, "L: %d M: %d H: %d Min: %d Avg: %.2f Max: %d",
	      low, med, hig, min, average, max);
  XDrawImageString (master->getDisplay (), pixmap, gc, pixmapWidth / 2 - 100,
		    20, stringBuf, len);
  free (stringBuf);
  if (lastImage)
    {
      len =
	asprintf (&stringBuf,
		  "[%li,%li:%li,%li] binn: %i:%i exposureTime: %.3f s",
		  lastX, lastY, lastSizeX, lastSizeY,
		  binningsX, binningsY, exposureTime);
      XDrawImageString (master->getDisplay (), pixmap, gc,
			pixmapWidth / 2 - 150, pixmapHeight - 20, stringBuf,
			len);
      free (stringBuf);
    }
}

void
Rts2xfocusCamera::printMouse ()
{
  char stringBuf[20];
  int len;
  len = snprintf (stringBuf, 20, "%i %i", mouseX, mouseY);
  XSetBackground (master->getDisplay (), gc, master->getRGB (0)->pixel);
  XDrawImageString (master->getDisplay (), pixmap, gc, 30, 30, stringBuf,
		    len);
  if (buttonX >= 0 && buttonY >= 0)
    drawCenterCross (buttonX, buttonY);
}

void
Rts2xfocusCamera::redrawMouse ()
{
  XClearArea (master->getDisplay (), window, 0, 0, 100, 40, False);
  XClearArea (master->getDisplay (), window, buttonX - 10, buttonY - 10, 20,
	      20, False);
}

void
Rts2xfocusCamera::redraw ()
{
  // do some line-drawing etc..
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
  // draw plots over stars..
  drawStars (images);

  printMouse ();

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
  struct ln_equ_posn change;

  while (1)
    {
      XWindowEvent (master->getDisplay (), window,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    PointerMotionMask, &event);
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
	      connection->queCommand (new Rts2CommandBinning (this, 1, 1));
	      break;
	    case XK_2:
	      connection->queCommand (new Rts2CommandBinning (this, 2, 2));
	      break;
	    case XK_3:
	      connection->queCommand (new Rts2CommandBinning (this, 3, 3));
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
	      setSaveImage (0 || exe);
	      break;
	      // change stuff
	    case XK_h:
	    case XK_Left:
	      change.ra = -1 * change_val;
	      change.dec = 0;
	      master->
		postEvent (new
			   Rts2Event (EVENT_MOUNT_CHANGE, (void *) &change));
	      break;
	    case XK_j:
	    case XK_Down:
	      change.ra = 0;
	      change.dec = -1 * change_val;
	      master->
		postEvent (new
			   Rts2Event (EVENT_MOUNT_CHANGE, (void *) &change));
	      break;
	    case XK_k:
	    case XK_Up:
	      change.ra = 0;
	      change.dec = change_val;
	      master->
		postEvent (new
			   Rts2Event (EVENT_MOUNT_CHANGE, (void *) &change));
	      break;
	    case XK_l:
	    case XK_Right:
	      change.ra = change_val;
	      change.dec = 0;
	      master->
		postEvent (new
			   Rts2Event (EVENT_MOUNT_CHANGE, (void *) &change));
	      break;
	    default:
	      break;
	    }
	  break;
	case MotionNotify:
	  mouseX = ((XMotionEvent *) & event)->x;
	  mouseY = ((XMotionEvent *) & event)->y;

	  printMouse ();
	  redrawMouse ();
	  break;
	case ButtonPress:
	  mouseX = ((XButtonPressedEvent *) & event)->x;
	  mouseY = ((XButtonPressedEvent *) & event)->y;
	  if (buttonX < 0 && buttonY < 0)
	    {
	      buttonX = mouseX;
	      buttonY = mouseY;
	      buttonImageTime = exposureStart;
	    }
	  else
	    {
	      // calculate distance travelled, print it, pixels / sec travelled, discard button
	      timersub (&exposureStart, &buttonImageTime, &exposureStart);
	      double del =
		exposureStart.tv_sec +
		(double) exposureStart.tv_usec / USEC_SEC;
	      double offsetX = buttonX - mouseX;
	      double offsetY = buttonY - mouseY;
	      if (del > 0)
		{
		  printf
		    ("Delay %.4f sec\nX offset: %.1f drift: %.1f pixels/sec\nY offset: %.1f drift: %1.f pixels/sec\n",
		     del, offsetX, offsetX / del, offsetY, offsetY / del);
		}
	      else
		{
		  printf
		    ("Delay %.4f sec\nX offset: %.1f\nY offset: %.1f\n",
		     del, offsetX, offsetY);
		}
	      // clear results
	      buttonX = -1;
	      buttonY = -1;
	      timerclear (&buttonImageTime);
	    }

	  printf ("%i %i\n", mouseX, mouseY);

	  printMouse ();
	  redrawMouse ();
	  break;
	}
    }
}

void
Rts2xfocusCamera::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_START_EXPOSURE:
      Rts2GenFocCamera::postEvent (event);
      // build window etc..
      buildWindow ();
      if (connection->havePriority ())
	queExposure ();
      return;
    }
  Rts2GenFocCamera::postEvent (event);
}

void
Rts2xfocusCamera::printFWHMTable ()
{
  Rts2GenFocCamera::printFWHMTable ();
  if (master->getStarsType ())
    redraw ();
}

void
Rts2xfocusCamera::processImage (Rts2Image * image)
{
  int dataSize;
  int i, j, k;
  unsigned short *im_ptr;

  // get to upper classes as well
  Rts2DevClientCameraFoc::processImage (image);

  pixmapWidth = image->getWidth ();
  pixmapHeight = image->getHeight ();
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
  if (!ximage)
    {
      ximage =
	XCreateImage (master->getDisplay (), master->getVisual (),
		      master->getDepth (), ZPixmap, 0, 0, pixmapWidth,
		      pixmapHeight, 8, 0);
      ximage->data = new char[ximage->bytes_per_line * pixmapHeight];
    }

  // build histogram
  memset (histogram, 0, sizeof (int) * HISTOGRAM_LIMIT);
  dataSize = pixmapHeight * pixmapWidth;
  k = 0;
  im_ptr = image->getDataUShortInt ();
  average = 0;
  max = 0;
  min = 65536;
  for (i = 0; i < pixmapHeight; i++)
    for (j = 0; j < pixmapWidth; j++)
      {
	histogram[*im_ptr]++;
	average += *im_ptr;
	if (max < *im_ptr)
	  max = *im_ptr;
	if (min > *im_ptr)
	  min = *im_ptr;
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

  im_ptr = image->getDataUShortInt ();

  for (j = 0; j < pixmapHeight; j++)
    for (i = 0; i < pixmapWidth; i++)
      {
	unsigned short val;
	val = *im_ptr;
	im_ptr++;
	if (val < low)
	  XPutPixel (ximage, i, j, master->getRGB (0)->pixel);
	else if (val > hig)
	  XPutPixel (ximage, i, j, master->getRGB (255)->pixel);
	else
	  {
	    XPutPixel (ximage, i, j,
		       master->
		       getRGB ((int) (255 * (val - low) / (hig - low)))->
		       pixel);
	  }
      }

  std::
    cout << "Data low:" << low << " med:" << med << " hig:" << hig <<
    " min:" << min << " average:" << average << " max:" << max << std::endl;

  XResizeWindow (master->getDisplay (), window, pixmapWidth, pixmapHeight);

  XPutImage (master->getDisplay (), pixmap, gc, ximage, 0, 0, 0, 0,
	     pixmapWidth, pixmapHeight);

  if (!lastImage)
    lastImage = 1;
  // some info values
  image->getValue ("X", lastX);
  image->getValue ("Y", lastY);
  lastSizeX = image->getWidth ();
  lastSizeY = image->getHeight ();
  image->getValue ("BIN_V", binningsX);
  image->getValue ("BIN_H", binningsY);

  exposureStart.tv_sec = image->getExposureSec ();
  exposureStart.tv_usec = image->getExposureUsec ();

  redraw ();
  XFlush (master->getDisplay ());
}

void
Rts2xfocusCamera::setCrossType (int in_crossType)
{
  crossType = in_crossType;
}

Rts2xfocus::Rts2xfocus (int in_argc, char **in_argv):
Rts2GenFocClient (in_argc, in_argv)
{
  displayName = NULL;

  crossType = 1;
  starsType = 0;

  changeVal = 15;

  addOption ('x', "display", 1, "name of X display");
  addOption ('t', "stars", 0, "draw stars over image (default to don't)");
  addOption ('X', "cross", 1,
	     "cross type (default to 1; possible values 0 - no cross, 1 - rectangles\n"
	     "    2 - circles, 3 - BOOTES special");
  addOption ('S', "save", 0, "save filenames (default don't save");
  addOption ('m', "change_val", 1,
	     "change value (in arcseconds; default to 15 arcsec");
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
    << "\thjkl, arrrows .. move (change mount position)\n" << std::endl
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
    case 'S':
      autoSave = 1;
      break;
    case 'x':
      displayName = optarg;
      break;
    case 't':
      starsType = 1;
      break;
    case 'X':
      crossType = atoi (optarg);
      break;
    case 'm':
      changeVal = atof (optarg);
      break;
    default:
      return Rts2GenFocClient::processOption (in_opt);
    }
  return 0;
}

int
Rts2xfocus::init ()
{
  int ret;
  ret = Rts2GenFocClient::init ();
  if (ret)
    return ret;

  // convert to degrees
  changeVal /= 3600.0;

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

Rts2GenFocCamera *
Rts2xfocus::createFocCamera (Rts2Conn * conn)
{
  Rts2xfocusCamera *cam;
  cam = new Rts2xfocusCamera (conn, changeVal, this);
  cam->setCrossType (crossType);
  return cam;
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
