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
#include <argz.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <mcheck.h>
#include <getopt.h>

#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/mkpath.h"
#include "../writers/fits.h"
#include "status.h"
#include "selector.h"
#include "../db/db.h"

int camd_id, teld_id;

#define fits_call(call) if (call) fits_report_error(stderr, status);

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
  char filename[250];
  char *dirname;

  gmtime_r (&image->exposure_time, &gmt);

  if (asprintf (&dirname, "~/%i", image->target_id))
    {
      perror ("planc data_handler asprintf");
      return -1;
    }

  mkpath (dirname, 0777);
  free (dirname);

  strftime (filename, 250, "!120/%Y%m%d%H%M%S.fits", &gmt);

  printf ("filename: %s", filename);

  if (fits_create (&receiver, filename) || fits_init (&receiver, size))
    {
      perror ("camc data_handler fits_init");
      return -1;
    }

  printf ("reading data socket: %i size: %i\n", sock, size);

  while ((s = devcli_read_data (sock, data, DATA_BLOCK_SIZE)) > 0)
    {
      if ((ret = fits_handler (data, s, &receiver)) < 0)
	return -1;
      if (ret == 1)
	break;
    }
  if (s < 0)
    {
      perror ("camc data_handler");
      return -1;
    }

  printf ("reading finished\n");

  if (fits_write_image_info (&receiver, image) || fits_close (&receiver))
    {
      perror ("camc data_handler fits_write");
      return -1;
    }

  return 0;
#undef receiver
}

int
expose (int npic)
{
  devcli_wait_for_status ("teld", "telescope", TEL_MASK_MOVING, TEL_STILL, 0);
  for (; npic > 0; npic--)
    {
      struct image_info *info;
      time_t t;

      printf ("exposure countdown %i..\n", npic);
      if (devcli_wait_for_status ("camd", "img_chip", CAM_MASK_READING,
				  CAM_NOTREADING, 0) ||
	  devcli_command (camd_id, NULL, "expose 0 120"))
	return -1;
      t = time (NULL);
      info = (struct image_info *) malloc (sizeof (struct image_info));
      if (devcli_command (camd_id, NULL, "info") ||
	  devcli_getinfo (camd_id, &info->camera) ||
	  devcli_command (teld_id, NULL, "info") ||
	  devcli_getinfo (teld_id, &info->telescope) ||
	  devcli_wait_for_status ("camd", "img_chip", CAM_MASK_EXPOSE,
				  CAM_NOEXPOSURE, 0)
	  || devcli_image_info (camd_id, info) ||
	  devcli_command (camd_id, NULL, "readout 0"))
	{
	  free (info);
	  return -1;
	}
    }
  return 0;
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  struct target *plan, *last;

  int c;

#ifdef DEBUG
  mtrace ();
#endif
  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"port", 1, 0, 'p'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "p:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
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

  /* connect to db */

  db_connect ();

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  if (devcli_connectdev (&camd_id, "camd", &data_handler) < 0)
    {
      perror ("devcli_connectdev camd");
      exit (EXIT_FAILURE);
    }

  printf ("camd_id: %i\n", camd_id);

  if (devcli_connectdev (&teld_id, "teld", NULL) < 0)
    {
      perror ("devcli_connectdev");
      exit (EXIT_FAILURE);
    }

  printf ("teld_id: %i\n", teld_id);

#define CAMD_WRITE_READ(command) if (devcli_command (camd_id, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read"); \
				  exit (EXIT_FAILURE); \
				}

#define TELD_WRITE_READ(command) if (devcli_command (teld_id, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read"); \
				  exit (EXIT_FAILURE); \
				}
  CAMD_WRITE_READ ("ready");
  CAMD_WRITE_READ ("info");

  TELD_WRITE_READ ("ready");
  TELD_WRITE_READ ("info");

  // make plan
  make_plan (&plan);

  for (last = plan; last; plan = last, last = last->next, free (plan))
    {
      time_t t = time (NULL);
      int obs_id;

      if (last->ctime < t)
	{
	  printf ("ctime %li (%s)", last->ctime, ctime (&last->ctime));
	  printf (" < t %li (%s)", t, ctime (&t));
	  continue;
	}

      devcli_server_command (NULL, "priority 120");

      printf ("waiting for priority\n");

      devcli_wait_for_status ("teld", "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("waiting end\n");

      {
	struct ln_equ_posn object;
	struct ln_lnlat_posn observer;
	struct ln_hrz_posn hrz;
	object.ra = last->ra;
	object.dec = last->dec;
	observer.lat = 50;
	observer.lng = -15;
	get_hrz_from_equ (&object, &observer, get_julian_from_sys (), &hrz);
	printf ("Ra: %f Dec: %f\n", object.ra, object.dec);
	printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);
      }
      if (devcli_command (teld_id, NULL, "move %f %f", last->ra, last->dec))
	{
	  printf ("telescope error\n\n--------------\n");
	}

      while ((t = time (NULL)) < last->ctime)
	{
	  printf ("sleeping for %li sec, till %s\n", last->ctime - t,
		  ctime (&last->ctime));
	  sleep (last->ctime - t);
	}

      db_start_observation (last->id, &t, &obs_id);

      devcli_wait_for_status ("camd", "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("OK\n");

      expose (1);

      db_end_observation (obs_id, time (NULL) - t);
    }

  devcli_server_disconnect ();
  db_disconnect ();

  return 0;
}
