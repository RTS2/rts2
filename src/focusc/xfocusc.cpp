/*!
 * @file Plan test client
 * $Id$
 *
 * Client to test camera deamon.
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <libnova/libnova.h>
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
#include <math.h>

#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/mkpath.h"
#include "../utils/mv.h"
#include "../writers/fits.h"
#include "imghdr.h"
#include "status.h"
#include "phot_info.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define PP_NEG

#define GAMMA 0.995

//#define PP_MED 0.60
#define PP_HIG 0.995
#define PP_LOW (1 - PP_HIG)

#define HISTOGRAM_LIMIT		65536

#define MAX_DEVICES             5

Display *display;
Visual *visual;
int depth;

int light_image = 1;

XColor rgb[256];

Colormap colormap;

int
phot_handler (struct param_status *params, struct phot_info *info)
{
  time_t t;
  time (&t);
  int ret;
  if (!strcmp (params->param_argv, "filter"))
    return param_next_integer (params, &info->filter);
  if (!strcmp (params->param_argv, "count"))
    {
      FILE *phot_log;
      char tc[30];
      phot_log = fopen ("phot_log", "a");
      ctime_r (&t, tc);
      tc[strlen (tc) - 1] = 0;
      ret = param_next_integer (params, &info->count);
      fprintf (phot_log, "%s %i %i\n", tc, info->filter, info->count);
      fclose (phot_log);
    }
  return ret;
}

class DeviceWindow
{
private:
  int save_fits;
  int autodark;
  int img_num; // image number with same parameters (exposure time, binning etc..
  unsigned short *dark_image;
public:
  struct device *camera;

  Window window;

  GC gc;
  XGCValues gcv;
  Pixmap pixmap;
  XImage *image;

  int hist[HISTOGRAM_LIMIT];

  float exposure_time;

  pthread_t thr;
  pthread_attr_t attrs;

    DeviceWindow (char *camera_name, Window root_window, int center,
		  float exposure, int i_save_fits, int i_autodark);

   ~DeviceWindow ()
  {
    XUnmapWindow (display, window);
  };

  void redraw ()
  {
    XDrawImageString (display, window, gc, 50, 50, "hellow", 6);
  };

  static void *static_event_loop (void *arg)
  {
    return ((DeviceWindow *) arg)->event_loop (NULL);
  }

  int center_exposure (void)
  {
    struct camera_info *caminfo = (struct camera_info *) &camera->info;
    int x, y, w, h;
    if (devcli_command (camera, NULL, "chipinfo 0"))
      return -1;
    x = caminfo->chip_info[0].width / 2 - 128;
    y = caminfo->chip_info[0].height / 2 - 128;
    w =
      x + 256 <
      caminfo->chip_info[0].width ? 256 : caminfo->chip_info[0].width - x;
    h =
      y + 256 <
      caminfo->chip_info[0].height ? 256 : caminfo->chip_info[0].height - y;

    devcli_command (camera, NULL, "box 0 %i %i %i %i", x, y, w, h);
    printf
      ("reading frame center [%i,%i:%i,%i]!!\n==============\n",
       x, y, x + w, y + h);
  }


  void *event_loop (void *arg)
  {
    struct camera_info *caminfo = (struct camera_info *) &camera->info;
    while (1)
      {
	XEvent event;

	printf ("waiting for event\n", event.type);
	fflush (stdout);

	XNextEvent (display, &event);
	KeySym ks;

	printf ("XEvent %i\n", event.type);
	fflush (stdout);

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
		clearDark ();
		devcli_command (this->camera, NULL, "binning 0 1 1");
		break;
	      case XK_2:
		clearDark ();
		devcli_command (this->camera, NULL, "binning 0 2 2");
		break;
	      case XK_3:
		clearDark ();
		devcli_command (this->camera, NULL, "binning 0 3 3");
		break;
	      case XK_e:
		clearDark ();
		exposure_time += 1;
		break;
	      case XK_d:
		clearDark ();
		if (exposure_time > 1)
		  exposure_time -= 1;
		break;
	      case XK_w:
		clearDark ();
		exposure_time += 0.1;
		break;
	      case XK_s:
		clearDark ();
		if (exposure_time > 0.1)
		  exposure_time -= 0.1;
		break;
	      case XK_q:
		clearDark ();
		exposure_time += 0.01;
		break;
	      case XK_a:
		clearDark ();
		if (exposure_time > 0.01)
		  exposure_time -= 0.01;
		break;
	      case XK_f:
		clearDark ();
		devcli_command (camera, NULL, "box 0 -1 -1 -1 -1");
		printf ("reading FULL FRAME!!\n================");
		break;
	      case XK_c:
		clearDark ();
		center_exposure ();
		break;
	      case XK_p:
		devcli_command_all (DEVICE_TYPE_PHOT, "integrate 5 10");
		break;
	      case XK_o:
		devcli_command_all (DEVICE_TYPE_PHOT, "stop");
		break;
	      case XK_y:
		save_fits = 1;
		printf ("Will write fits from %s\n", camera->name);
		break;
	      case XK_u:
		save_fits = 0;
		printf ("Don't write fits from %s\n", camera->name);
		break;
	      default:
		// fprintf (stderr, "Unknow key pressed:%i\n", (int) ks);
		break;
	      }
	  }
	printf ("next_exposure_time: %f\n", exposure_time);
      }
  };

  static int static_data_handler (int sock, size_t size,
				  struct image_info *image_info, void *arg)
  {
    printf ("data handler");
    fflush (stdout);
    return ((DeviceWindow *) arg)->data_handler (sock, size, image_info);
  }

  /*!
   * Handle camera data connection.
   *
   * @params sock         socket fd.
   *
   * @return      0 on success, -1 and set errno on error.
   */
  int data_handler (int sock, size_t size, struct image_info *image_info)
  {
    int i, j, k, width, height;
    struct fits_receiver_data receiver;
    struct tm gmt;
    char *data = NULL, *d, *pix_data;
    unsigned short *im;
    unsigned short m;
    int ret = 0;
    ssize_t s;
    int l_save_fits;
    XSetWindowAttributes xswa;
    struct imghdr *imghdr;
    char *filename = NULL;
    char filen[250];

    int ds;

    l_save_fits = save_fits;

    unsigned short low, med, hig;

    unsigned short *im_ptr;
    unsigned short *dark_ptr;

    printf ("reading data socket: %i size: %i\n", sock, size);

    if (l_save_fits)
      {
	// create path with camera name
	if (mkpath (image_info->camera_name, 0755) != 0)
	  {
	    perror ("creating dir for camera");
	    l_save_fits = 0;
	  }
	else
	  {
	    asprintf (&filename, "%s/%s%04i%02i%02i%02i%02i%02i.fits",
		      image_info->camera_name, (image_info->target_type == TARGET_DARK ? "d" : ""),
		      gmt.tm_year, gmt.tm_mon,
		      gmt.tm_mday, gmt.tm_hour, gmt.tm_min, gmt.tm_sec);
	    strcpy (filen, filename);
	    printf ("filename: %s\n", filename);
	    if (fits_create (&receiver, filename)
		|| fits_init (&receiver, size))
	      {
		perror ("camc data_handler fits_init");
		ret = -1;
		goto out;
	      }
	  }
      }

    receiver.info = image_info;

    data = (char *) malloc (size * 2 + sizeof (struct imghdr));
    d = data;

    while ((s = devcli_read_data (sock, d, DATA_BLOCK_SIZE)) > 0)
      {
	if (l_save_fits)
	  {
	    if ((ret = fits_handler (d, s, &receiver)) < 0)
	      goto out;
	  }
	d += s;
	printf (".%i\n", d - data);
      }

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

    if (image_info->target_type == TARGET_DARK)
    {
      delete dark_image;
      // unfortunatelly we have to copy data..as we have in pix_data
      // data with image header, and later we will have no way how to
      // free them (if we loose their pointer)
      dark_image = new unsigned short[width * height];
      memcpy ((char *) dark_image, pix_data, width * height * sizeof (unsigned short));
    }

    if (l_save_fits)
      {
	if (fits_write_image_info (&receiver, image_info, NULL)
	    || fits_close (&receiver))
	  {
	    perror ("camc data_handler fits_write");
	    ret = -1;
	    goto out;
	  }
      }

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

    ds = height * width;
    k = 0;
    printf ("image: %ix%i\n", height, width);

    im_ptr = im;
    dark_ptr = dark_image;
    
    for (j = 0; j < height; j++)
      for (i = 0; i < width; i++)
	{
	  if (dark_ptr)
	  {
	    // substract dark image
	    if (*dark_ptr < *im_ptr)
	      *im_ptr -= *dark_ptr;
	    else
	      *im_ptr = 0;
            dark_ptr++;
	  }
	  hist[*im_ptr]++;
	  im_ptr++;
	}

    low = med = hig = 0;
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
    if (!hig)
      hig = 65535;
    if (low == hig)
      low = hig / 2 - 1;
    fprintf (stderr, "levels: low:%d, med:%d, hi:%d ds: %d\n", low, med, hig,
	     ds);

    im_ptr = im;
    
    for (j = 0; j < height; j++)
      for (i = 0; i < width; i++)
	{
	  m = *im_ptr;
	  im_ptr++;
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

    XChangeWindowAttributes (display, window, CWColormap | CWBackPixmap,
			     &xswa);

    XClearWindow (display, window);
    redraw ();
    XFlush (display);

  out:
    free (data);
    free (filename);
    img_num++;
    return ret;
  };

  int readout ()
  {
    struct image_info *info;

    info = (struct image_info *) malloc (sizeof (struct image_info));
    info->exposure_time = time (NULL);
    info->exposure_length = exposure_time;
    info->target_id = -1;
    info->observation_id = -1;
    info->target_type = (isLight () ? TARGET_LIGHT : TARGET_DARK);
    info->camera_name = camera->name;
    printf ("waiting for camera\n");
    fflush (stdout);
    if (devcli_command (camera, NULL, "info") ||
	!memcpy (&info->camera, &camera->info, sizeof (struct camera_info)) ||
	!memset (&info->telescope, 0, sizeof (struct telescope_info)) ||
	devcli_image_info (camera, info)
	|| devcli_wait_for_status (camera, "img_chip", CAM_MASK_EXPOSE,
				   CAM_NOEXPOSURE, 0))
      {
	printf ("wait with error\n");
	fflush (stdout);
	free (info);
	return -1;
      }
    printf ("wait for readout\n");
    fflush (stdout);

    if (devcli_command (camera, NULL, "readout 0"))
      {
	printf ("reading commond err\n");
	fflush (stdout);
	free (info);
	return -1;
      }
    free (info);
    printf ("reading out\n");
    fflush (stdout);
    return 0;
  };

  static void *run (void *arg)
  {

    ((DeviceWindow *) arg)->expose_loop ();
  }

  void expose_loop ()
  {
    time_t start_time;

    time (&start_time);
    start_time += 20;

    pthread_attr_init (&attrs);
    pthread_create (&thr, &attrs, static_event_loop, this);

    while (1)
      {
	time_t t = time (NULL);

	devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
				DEVICE_PRIORITY, 0);

	printf ("OK\n");

	time (&t);
	printf ("exposure countdown %s..", ctime (&t));
	t += (long int) exposure_time;
	printf ("readout at: %s", ctime (&t));
	printf ("exposure_time: %f\n", exposure_time);
	if (devcli_wait_for_status (camera, "img_chip", CAM_MASK_READING,
				    CAM_NOTREADING, 0) ||
	    devcli_command (camera, NULL, "expose 0 %i %f", (isLight () ? 1 : 0), exposure_time))
	  {
	    perror ("expose:");
	  }
	printf ("startign readout\n");
	fflush (stdout);
	readout ();
	printf ("after readout\n");
	fflush (stdout);
      }
  }
  
  int isLight (void)
  {
    return !(autodark && img_num == 0);

  }

  void clearDark (void)
  {
    img_num = 0;
    delete dark_image;
    dark_image = NULL;
  }
};

DeviceWindow::DeviceWindow (char *camera_name, Window root_window, int center,
			    float exposure, int i_save_fits, int i_autodark)
{
  XSetWindowAttributes xswa;
  XTextProperty window_title;

  gc = NULL;
  image = NULL;
  rgb[256];
  exposure_time = exposure;
  
  save_fits = i_save_fits;
  autodark = i_autodark;
  img_num = 0;
  dark_image = NULL;

  window =
    XCreateWindow (display, DefaultRootWindow (display), 0, 0, 100,
		   100, 0, depth, InputOutput, visual, 0, &xswa);

  pixmap = XCreatePixmap (display, window, 1200, 1200, depth);

  gc = XCreateGC (display, pixmap, 0, &gcv);

  XSelectInput (display, window,
		KeyPressMask | ButtonPressMask | ExposureMask);

  XMapRaised (display, window);

  camera = devcli_find (camera_name);

  if (!camera)
    {
      printf ("**** Cannot find camera\n");
      exit (EXIT_FAILURE);
    }
  camera->data_handler_args = this;
  camera->data_handler = static_data_handler;

#define CAMD_WRITE_READ(command) if (devcli_command (camera, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_camd"); \
				  return; \
				}

  CAMD_WRITE_READ ("ready");
  CAMD_WRITE_READ ("base_info");
  CAMD_WRITE_READ ("info");
  CAMD_WRITE_READ ("chipinfo 0");

  if (center)
    {
      center_exposure ();
    }

  devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);
  XStringListToTextProperty (&camera_name, 1, &window_title);
  XSetWMName (display, window, &window_title);
}


int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;

  int i;

  char *server;

  char *camera_names[MAX_DEVICES];
  int camera_num = 0;
  DeviceWindow *camera_window[MAX_DEVICES];

  pthread_t camera_thr[MAX_DEVICES];
  pthread_attr_t attrs;

  Window root_window;
  XSetWindowAttributes xswa;

  struct device *phot;

  int center = 0;
  float exposure = 10.0;
  int save_fits = 0;
  int autodark = 0;

  int c = 0;

#ifdef DEBUG
  mtrace ();
#endif
  /* get attrs */

  camera_names[0] = "C0";

  while (1)
    {
      static struct option long_option[] = {
	{"device", 1, 0, 'd'},
	{"center", 0, 0, 'c'},
	{"exposure", 1, 0, 'e'},
	{"port", 1, 0, 'p'},
	{"help", 0, 0, 'h'},
	{"save", 0, 0, 's'},
	{"autodark", 0, 0, 'a'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "d:ce:p:hsD", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'd':
	  camera_names[camera_num] = optarg;
	  camera_num++;
	  if (camera_num >= MAX_DEVICES)
	    {
	      printf ("More than %i devices specified, exiting.\n",
		      MAX_DEVICES);
	      exit (1);
	    }
	  break;
	case 'c':
	  center = 1;
	  break;
	case 'e':
	  exposure = atof (optarg);
	  break;
	case 'p':
	  port = atoi (optarg);
	  break;
	case 'h':
	  printf ("Options:\n");
	  printf ("\t--device|-d <device_name> device(s) name(s) (xfocusc accept multiple entries)\n");
	  printf ("\t--port|-p <port_num>   port of the server\n");
	  printf ("\t--center|-c            start center exposure\n");
	  printf ("\t--exposure|-e          exposure time in seconds\n");
	  printf ("\t--save|-s              autosave images\n");
	  printf ("\t--autodark|-a          takes and uses dark images\n");
	  printf ("Keys:\n"
		  "\t1,2,3 .. binning 1x1, 2x2, 3x3\n"
		  "\tq,a   .. increase/decrease exposure 0.01 sec\n"
		  "\tw,s   .. increase/decrease exposure 0.1 sec\n"
		  "\te,d   .. increase/decrease exposure 1 sec\n"
		  "\tf     .. full frame exposure\n"
		  "\tc     .. center (256x256) exposure\n"
		  "\ty     .. save fits file\n"
		  "\tu     .. don't save fits file\n");
	  printf ("Examples:\n"
	          "\trts2-xfocusc -d C0 -d C1 -e 20 .. takes 20 sec exposures on devices C0 and C1\n"
	          "\trts2-xfocusc -d C2 -a -e 10    .. takes 10 sec exposures on device C2. Takes darks and use them\n");
	  exit (EXIT_SUCCESS);
	case 's':
	  save_fits = 1;
	  break;
	case 'a':
	  autodark = 1;
	  break;
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

  if (!display)
    {
      printf ("null display\n");
      exit (1);
    }

  depth = DefaultDepth (display, DefaultScreen (display));
  visual = DefaultVisual (display, DefaultScreen (display));

  colormap = DefaultColormap (display, DefaultScreen (display));

  printf ("Window created.\n");

  // allocate colormap..
  for (int i = 0; i < 256; i++)
    {
      rgb[i].red = (unsigned short) (65536 * (1.0 * i / 256));
      rgb[i].green = (unsigned short) (65536 * (1.0 * i / 256));
      rgb[i].blue = (unsigned short) (65536 * (1.0 * i / 256));
      rgb[i].flags = DoRed | DoGreen | DoBlue;


      XAllocColor (display, colormap, rgb + i);
    }

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  devcli_server_command (NULL, "priority 137");

  printf ("waiting for priority\n");

  for (i = 0; i < camera_num; i++)
    {
      camera_window[i] =
	new DeviceWindow (camera_names[i], root_window, center, exposure,
			  save_fits, autodark);
    }

  printf ("waiting end\n");

  for (phot = devcli_devices (); phot; phot = phot->next)
    if (phot->type == DEVICE_TYPE_PHOT)
      {
	printf ("setting handler: %s\n", phot->name);
	devcli_set_command_handler (phot,
				    (devcli_handle_response_t) phot_handler);
      }

  for (i = 0; i < camera_num; i++)
    {
      pthread_attr_init (&attrs);
      pthread_create (&(camera_thr[i]), &attrs, DeviceWindow::run,
		      (void *) camera_window[i]);
    }

  for (i = 0; i < camera_num; i++)
    {
      pthread_join (camera_thr[i], NULL);
    }

  printf ("joind sucessfull\n");
  fflush (stdout);

  devcli_server_disconnect ();

  XCloseDisplay (display);

  return 0;
}
