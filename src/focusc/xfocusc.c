/*!
 * @file Plan test client
 * $Id$
 *
 * Client to test camera deamon.
 *
 * @author petr
 */

#define _GNU_SOURCE

#include <errno.h>
#include <libnova.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <mcheck.h>
#include <getopt.h>

#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/mkpath.h"
#include "../utils/mv.h"
#include "imghdr.h"
#include "status.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define PP_NEG

#define GAMMA 0.995

//#define PP_MED 0.60
#define PP_HIG 0.995
#define PP_LOW (1 - PP_HIG)

#define HISTOGRAM_LIMIT		65536

#define EXPOSURE_TIME		10

struct device *camera;

Display *display;
Visual *visual;
Window window;

GC gc = NULL;
XGCValues gcv;
Pixmap pixmap;
XImage *image = NULL;

int hist[HISTOGRAM_LIMIT];
int depth = 0;

Colormap colormap;
XColor rgb[256];

void
redraw ()
{
  XDrawImageString (display, window, gc, 50, 50, "hellow", 6);
}

void *
event_loop (void *arg)
{
  while (1)
    {
      XEvent event;
      XNextEvent (display, &event);
      KeySym ks;

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
	      devcli_command (camera, NULL, "binning 0 1 1");
	      break;
	    case XK_2:
	      devcli_command (camera, NULL, "binning 0 2 2");
	      break;
	    case XK_3:
	      devcli_command (camera, NULL, "binning 0 3 3");
	      break;
	    default:
	      fprintf (stderr, "Unknow key pressed:%i\n", ks);
	      break;
	    }
	}
    }
}

/*!
 * Handle camera data connection.
 *
 * @params sock 	socket fd.
 *
 * @return	0 on success, -1 and set errno on error.
 */
int
data_handler (int sock, size_t size, struct image_info *image_info)
{
  int i, j, k, width, height;
  char *data, *d, *pix_data;
  unsigned short *im;
  unsigned short m;
  int ret = 0;
  ssize_t s;
  XSetWindowAttributes xswa;
  struct imghdr *imghdr;

  int ds;

  short low, med, hig;

  printf ("reading data socket: %i size: %i\n", sock, size);

  data = (char *) malloc (size * 2 + sizeof (struct imghdr));
  d = data;

  while ((s = devcli_read_data (sock, d, DATA_BLOCK_SIZE)) > 0)
    d += s;

  if (s < 0)
    {
      perror ("camc data_handler");
      ret = -1;
      goto out;
    }

  printf ("reading finished\n");

  pix_data = data + sizeof (struct imghdr);

  im = (unsigned short *) pix_data;


  imghdr = (struct imghdr *) data;

  width = imghdr->sizes[0];
  height = imghdr->sizes[1];

  if (!image)
    {
      image =
	XCreateImage (display, visual, depth, ZPixmap, 0, 0, width, height,
		      8, 0);
      image->data = (char *) malloc (image->bytes_per_line * height + 16);

    }

  // histogram build
  for (i = 0; i < HISTOGRAM_LIMIT; i++)
    hist[i] = 0;

  ds = 0;
  k = 0;
  printf ("image: %ix%i\n", height, width);
  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      {
	m = im[i + width * j];
	hist[m]++;
	ds++;
      }

  low = hig = med = 0;
  j = 0;
  for (i = 0; i < HISTOGRAM_LIMIT; i++)
    {
      j += hist[i];
      if ((!low) && (((float) j / (float) ds) > PP_LOW))
	low = i;
#ifdef PP_MED
      if ((!med) && (((float) j / (float) ds) > PP_MED))
	med = i;
#endif
      if ((!hig) && (((float) j / (float) ds) > PP_HIG))
	hig = i;
    }
  fprintf (stderr, "levels: low:%d, med:%d, hi:%d ds: %d\n", low, med, hig,
	   ds);

  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      {
	m = im[i + j * width];
	if (m < low)
	  XPutPixel (image, i, j, rgb[0].pixel);
	else if (m > hig)
	  XPutPixel (image, i, j, rgb[255].pixel);
	else
	  {
//printf ("middle: %i %i %li\n", m, 256 * (m - low) / (hig - low), rgb[(int)256 * (m - low) / (hig - low)].pixel);
	    XPutPixel (image, i, j, rgb[(int) (256 * (m - low) / (hig - low))].pixel);	// );
	  }
      }

  XResizeWindow (display, window, width, height);

  XPutImage (display, pixmap, gc, image, 0, 0, 0, 0, width, height);

  xswa.colormap = colormap;
  xswa.background_pixmap = pixmap;

  XChangeWindowAttributes (display, window, CWColormap | CWBackPixmap, &xswa);

  XClearWindow (display, window);
  redraw ();
  XFlush (display);

out:
  free (data);
  return ret;
}

int
readout ()
{
  struct image_info *info;

  info = (struct image_info *) malloc (sizeof (struct image_info));
  info->exposure_time = time (NULL);
  info->exposure_length = EXPOSURE_TIME;
  info->target_id = -1;
  info->observation_id = -1;
  info->target_type = 1;
  info->camera_name = camera->name;
  if (devcli_command (camera, NULL, "info") ||
      !memcpy (&info->camera, &camera->info, sizeof (struct camera_info)) ||
      !memset (&info->telescope, 0, sizeof (struct telescope_info)) ||
      devcli_image_info (camera, info)
      || devcli_wait_for_status (camera, "img_chip", CAM_MASK_EXPOSE,
				 CAM_NOEXPOSURE, 0))
    {
      free (info);
      return -1;
    }

  if (devcli_command (camera, NULL, "readout 0"))
    {
      free (info);
      return -1;
    }
  free (info);
  return 0;
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;

  XSetWindowAttributes xswa;

  int i;

  char *server;
  time_t start_time;

  char *camera_name;

  int c = 0;

  pthread_t thr;
  pthread_attr_t attrs;

#ifdef DEBUG
  mtrace ();
#endif
  /* get attrs */

  camera_name = "C0";

  while (1)
    {
      static struct option long_option[] = {
	{"device", 1, 0, 'd'},
	{"port", 1, 0, 'p'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "d:p:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'd':
	  camera_name = optarg;
	  break;
	case 'p':
	  port = atoi (optarg);
	  break;
	case 'h':
	  printf ("Options:\n\tport|p <port_num>\t\tport of the server");
	  exit (EXIT_SUCCESS);
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }
  if (optind != argc - 1)
    {
      printf ("You must pass server address\n");
      exit (EXIT_FAILURE);
    }

  server = argv[optind++];

  display = XOpenDisplay (NULL);

  depth = DefaultDepth (display, DefaultScreen (display));
  visual = DefaultVisual (display, DefaultScreen (display));

  window =
    XCreateWindow (display, DefaultRootWindow (display), 0, 0, 100,
		   100, 0, depth, InputOutput, visual, 0, &xswa);

  pixmap =
    XCreatePixmap (display, DefaultRootWindow (display), 1000, 1000, depth);

  gc = XCreateGC (display, pixmap, 0, &gcv);

  XSelectInput (display, window,
		KeyPressMask | ButtonPressMask | ExposureMask);

  colormap = DefaultColormap (display, DefaultScreen (display));

  // allocate colormap..
  for (i = 0; i < 256; i++)
    {
      rgb[i].red = 65536 * (1.0 * i / 256);
      rgb[i].green = 65536 * (1.0 * i / 256);
      rgb[i].blue = 65536 * (1.0 * i / 256);
      rgb[i].flags = DoRed | DoGreen | DoBlue;
      XAllocColor (display, colormap, rgb + i);
    }

  XMapRaised (display, window);

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  camera = devcli_find (camera_name);
  camera->data_handler = data_handler;

  if (!camera)
    {
      printf ("**** Cannot find camera\n");
      exit (EXIT_FAILURE);
    }

#define CAMD_WRITE_READ(command) if (devcli_command (camera, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_camd"); \
				  return EXIT_FAILURE; \
				}

  CAMD_WRITE_READ ("ready");
  CAMD_WRITE_READ ("info");
  CAMD_WRITE_READ ("chipinfo 0");

  umask (0x002);

  devcli_server_command (NULL, "priority 137");

  printf ("waiting for priority\n");

  devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);

  printf ("waiting end\n");

  time (&start_time);
  start_time += 20;

  pthread_attr_init (&attrs);
  pthread_create (&thr, &attrs, event_loop, NULL);

  while (1)
    {
      time_t t = time (NULL);

      devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("OK\n");

      time (&t);
      printf ("exposure countdown %s..\n", ctime (&t));
      t += EXPOSURE_TIME;
      printf ("readout at: %s\n", ctime (&t));
      if (devcli_wait_for_status (camera, "img_chip", CAM_MASK_READING,
				  CAM_NOTREADING, 0) ||
	  devcli_command (camera, NULL, "expose 0 %i %i", 1, EXPOSURE_TIME))
	{
	  perror ("expose:");
	}


      readout ();
    }

  devcli_server_disconnect ();

  pthread_cancel (thr);

  XUnmapWindow (display, window);
  XCloseDisplay (display);

  return 0;
}
