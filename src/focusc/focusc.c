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
#include "../writers/fits.h"
#include "status.h"

#define EXPOSURE_TIME		20

struct device *camera;

char *dark_name = NULL;

pid_t parent_pid;

#define fits_call(call) if (call) fits_report_error(stderr, status);

int
get_regions ()
{
  FILE *ds9_regs;

  float x, y, w, h;
  char buf[80];

  // get regions
  ds9_regs = popen ("xpaget ds9 regions", "r");
  if (!ds9_regs)
    {
      perror ("popen xpaget ds9 regions");
      return -1;
    }

  while (fgets (buf, 80, ds9_regs))	//, "image;box(%f,%f,%f,%f,", &x, &y, &w, &h)) != EOF)
    {
      int ret;
      printf ("get: %s\n", buf);

      ret = sscanf (buf, "image;box(%f,%f,%f,%f,", &x, &y, &w, &h);

      if (ret == 4)
	{
	  int cx, cy, cw, ch;
	  cw = (int) w;
	  ch = (int) h;
	  cx = (int) x - cw / 2;
	  cy = (int) y + ch / 2;

	  cy = camera->info.camera.chip_info[0].height - (int) cy;

	  printf ("get: %i,%i,%i,%i\n", cx, cy, cw, ch);
	  devcli_command (camera, NULL, "box 0 %i %i %i %i", cx, cy, cw, ch);
	}
      fflush (stdout);
    }

  pclose (ds9_regs);

  return 0;
}

/*!
 * Handle camera data connection.
 *
 * @params sock 	socket fd.
 *
 * @return	0 on success, -1 and set errno on error.
 */
int
data_handler (int sock, size_t size, struct image_info *image)
{
  struct fits_receiver_data receiver;
  struct tm gmt;
  char data[DATA_BLOCK_SIZE];
  int ret = 0;
  ssize_t s;
  char filen[250];
  char *filename;

  char *cmd;

  gmtime_r (&image->exposure_time, &gmt);

/*  switch (image->target_type)
    {
    case TARGET_DARK:
      if (!asprintf (&dirname, "/darks/%s/", image->camera.name))
	{
	  perror ("planc data_handler asprintf");
	  return -1;
	}
      break;
    default:
      if (!asprintf
	  (&dirname, "/images/%s/%i/", image->camera.name, image->target_id))
	{
	  perror ("planc data_handler asprintf");
	  return -1;
	}
    }

  mkpath (dirname, 0777); */

  asprintf (&filename, "tmp%i.fits", parent_pid);
  strcpy (filen, filename);

  printf ("filename: %s\n", filename);

  if (fits_create (&receiver, filename) || fits_init (&receiver, size))
    {
      perror ("camc data_handler fits_init");
      ret = -1;
      goto free_filename;
    }

/*  if (image->target_type == TARGET_DARK)
    {
      if (dark_name)
	free (dark_name);
      dark_name = (char *) malloc (strlen (filename) + 1);
      strcpy (dark_name, filename);
    } */
  printf ("reading data socket: %i size: %i\n", sock, size);

  while ((s = devcli_read_data (sock, data, DATA_BLOCK_SIZE)) > 0)
    {
      if ((ret = fits_handler (data, s, &receiver)) < 0)
	goto free_filename;
      if (ret == 1)
	break;
    }

  if (s < 0)
    {
      perror ("camc data_handler");
      ret = -1;
      goto free_filename;
    }

  printf ("reading finished\n");

  if (fits_write_image_info (&receiver, image, dark_name)
      || fits_close (&receiver))
    {
      perror ("camc data_handler fits_write");
      ret = -1;
      goto free_filename;
    }

  get_regions ();

  asprintf (&cmd, "cat '%s' | xpaset ds9 fits", filename);
  printf ("calling '%s'\n", cmd);
  if (system (cmd))
    {
      perror ("call");
    }
  free (cmd);

  free (filename);

  return 0;
free_filename:
  free (filename);
  return ret;
#undef receiver
}

int
readout ()
{
  struct image_info *info;
  union devhnd_info *devinfo;

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

  get_regions ();

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

  char *server;
  time_t start_time;

  char *camera_name;

  int c, i = 0;

#ifdef DEBUG
  mtrace ();
#endif
  /* get attrs */

  parent_pid = getpid ();

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
	  if (port < 1 || port == UINT_MAX)
	    {
	      printf ("invalcamd_id port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
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

  while (1)
    {
      time_t t = time (NULL);
      int ret;

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

  return 0;
}
