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
#include "selector.h"
#include "../db/db.h"

#define EXPOSURE_TIME		30

struct device *camera, *telescope;

char *dark_name = NULL;


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
  char filen[250];
  char *filename;
  char *dirname;

  char *cmd;
  FILE *past_out;

  long int id;
  float ra;
  float dec;
  float ra_err;
  float dec_err;

  gmtime_r (&image->exposure_time, &gmt);

  switch (image->target_type)
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

  mkpath (dirname, 0777);

  strftime (filen, 250, "%Y%m%d%H%M%S.fits", &gmt);
  asprintf (&filename, "%s%s", dirname, filen);

  printf ("filename: %s\n", filename);

  if (fits_create (&receiver, filename) || fits_init (&receiver, size))
    {
      perror ("camc data_handler fits_init");
      ret = -1;
      goto free_filename;
    }

  if (image->target_type == TARGET_DARK)
    {
      if (dark_name)
	free (dark_name);
      dark_name = (char *) malloc (strlen (filename) + 1);
      strcpy (dark_name, filename);
    }
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

  switch (image->target_type)
    {
    case TARGET_LIGHT:
      printf ("chdir %s\n", dirname);

      if ((ret = chdir (dirname)))
	{
	  perror ("chdir");
	  goto free_filename;
	}

      asprintf (&cmd, "/home/rtopera/bin/detect %s 2>&1", filen);
      printf ("calling %s.", cmd);

      if ((ret = system (cmd)))
	{
	  perror ("system");
	  free (cmd);
	  goto free_filename;
	}

      free (cmd);

      // now call past..
      asprintf (&cmd, "/home/rtopera/bin/past %s 2>%s.past", filen, filen);
      printf ("\ncalling %s.", cmd);

      if (!(past_out = popen (cmd, "r")))
	{
	  perror ("popen");
	  free (cmd);
	  ret = -1;
	  goto free_filename;
	};

      free (cmd);

      printf ("OK\n");

      if (fscanf
	  (past_out, "%li %f %f (%f,%f)", &id, &ra, &dec, &ra_err,
	   &dec_err) != 5)
	{
	  char *trash_name;
	  asprintf (&trash_name, "/trash/%s", filen);
	  printf ("%s scanf error, invalid line\n", filename);
	  printf ("mv %s -> %s", filename, trash_name);
	  if ((ret = (mv (filename, trash_name))))
	    perror ("rename bad image");
	  printf ("..OK\n");
	  free (trash_name);
	  goto free_filename;
	}

      printf ("ra: %f dec: %f ra_err: %f min dec_err: %f min\n", ra, dec,
	      ra_err, dec_err);

      if (!devcli_command (telescope, NULL, "correct %i %f %f",
			   image->telescope.correction_mark, ra_err / 60.0,
			   dec_err / 60.0))
	perror ("telescope correct");
      break;

    case TARGET_DARK:
      printf ("darkname: %s\n", dark_name);

      ret = db_add_darkfield (dark_name, &image->exposure_time,
			      image->exposure_length,
			      image->camera.ccd_temperature * 100);
      break;
    }

  free (dirname);
  return 0;
free_filename:
  free (dirname);
  free (filename);
  return ret;
#undef receiver
}

int
readout (struct target *plan)
{
  struct image_info *info =
    (struct image_info *) malloc (sizeof (struct image_info));

  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING, TEL_STILL,
			  0);

  info->exposure_time = time (NULL);
  info->exposure_length = EXPOSURE_TIME;
  info->target_id = plan->id;
  info->observation_id = plan->obs_id;
  info->target_type = plan->type;
  if (devcli_command (camera, NULL, "info") ||
      !memcpy (&info->camera, &camera->info, sizeof (struct camera_info)) ||
      devcli_command (telescope, NULL, "info") ||
      !memcpy (&info->telescope, &telescope->info,
	       sizeof (struct telescope_info))
      || devcli_image_info (camera, info)
      || devcli_wait_for_status (camera, "img_chip", CAM_MASK_EXPOSE,
				 CAM_NOEXPOSURE, 0)
      || devcli_command (camera, NULL, "readout 0"))
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
  struct target *plan, *last;
  time_t start_time;

  int c, i = 0;

  int priority;

#ifdef DEBUG
  mtrace ();
#endif
  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"port", 1, 0, 'p'},
	{"priority", 1, 0, 'r'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "p:r:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  port = atoi (optarg);
	  if (port < 1 || port == UINT_MAX)
	    {
	      printf ("invalid server port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'r':
	  priority = atoi (optarg);
	  if (priority < 1 || priority > 10000)
	    {
	      printf ("invalid priority value: %s\n", optarg);
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

  if (c == db_connect ())
    {
      fprintf (stderr, "cannot connect to db, SQL error code: %i\n", c);
      exit (EXIT_FAILURE);
    }

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  camera = devcli_find ("camd");
  if (!camera)
    {
      printf
	("**** cannot find camera!\n**** please check that it's connected and camd runs.");
      exit (EXIT_FAILURE);
    }

  camera->data_handler = data_handler;

  telescope = devcli_find ("teld");
  if (!telescope)
    {
      printf
	("**** cannot find telescope!\n**** please check that it's connected and teld runs.");
      exit (EXIT_FAILURE);
    }

#define CAMD_WRITE_READ(command) if (devcli_command (camera, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_camd"); \
				  return EXIT_FAILURE; \
				}

#define TELD_WRITE_READ(command) if (devcli_command (telescope, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_teld"); \
				  return EXIT_FAILURE; \
				}
  CAMD_WRITE_READ ("ready");
  CAMD_WRITE_READ ("info");

  TELD_WRITE_READ ("ready");
  TELD_WRITE_READ ("info");

  srandom (time (NULL));

  umask (0x002);

  devcli_server_command (NULL, "priority %i", priority);

  printf ("waiting for priority\n");

  devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);

  printf ("waiting end\n");


  plan = (struct target *) malloc (sizeof (struct target));

  time (&start_time);
  start_time += 20;


  printf ("Making plan %li... \n", start_time);
  if (get_next_plan
      (plan, SELECTOR_AIRMASS, NULL, start_time, 0, EXPOSURE_TIME))
    {
      printf ("Error making plan\n");
      exit (EXIT_FAILURE);
    }
  printf ("...plan made\n");
  fflush (stdout);

  for (last = plan; last; plan = last, last = last->next, free (plan))
    {
      time_t t = time (NULL);

      if (last->ctime < t)
	{
	  printf ("ctime %li (%s)", last->ctime, ctime (&last->ctime));
	  printf (" < t %li (%s)", t, ctime (&t));
	  continue;
	}


      if (last->type == TARGET_LIGHT || last->type == TARGET_FLAT)
	{
	  {
	    struct ln_equ_posn object;
	    struct ln_lnlat_posn observer;
	    struct ln_hrz_posn hrz;
	    object.ra = last->ra;
	    object.dec = last->dec;
	    observer.lat = 50;
	    observer.lng = -15;
	    get_hrz_from_equ (&object, &observer, get_julian_from_sys (),
			      &hrz);
	    printf ("Ra: %f Dec: %f\n", object.ra, object.dec);
	    printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);
	  }

	  if (devcli_command
	      (telescope, NULL, "move %f %f", last->ra, last->dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	    }
	}

      while ((t = time (NULL)) < last->ctime)
	{
	  printf ("sleeping for %li sec, till %s\n", last->ctime - t,
		  ctime (&last->ctime));
	  sleep (last->ctime - t);
	}

      if (last->type == TARGET_LIGHT)
	{
	  db_start_observation (last->id, &t, &last->obs_id);
	}

      devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("OK\n");

      time (&t);
      printf ("exposure countdown %s..\n", ctime (&t));
      t += EXPOSURE_TIME;
      printf ("readout at: %s\n", ctime (&t));
      if (devcli_wait_for_status (camera, "img_chip", CAM_MASK_READING,
				  CAM_NOTREADING, 0) ||
	  devcli_command (camera, NULL, "expose 0 %i %i",
			  last->type != TARGET_DARK, EXPOSURE_TIME))
	{
	  perror ("expose:");
	}

      // generate next plan entry
      i++;
      plan = (struct target *) malloc (sizeof (struct target));
      get_next_plan (plan, SELECTOR_AIRMASS, last, start_time, i,
		     EXPOSURE_TIME);
      last->next = plan;
      printf ("next plan #%i: id %i type %i\n", i, last->next->id,
	      last->next->type);

      readout (last);

      if (last->type == TARGET_LIGHT)
	{
	  db_end_observation (last->obs_id, time (NULL) - last->ctime);
	}
    }

  devcli_server_disconnect ();
  db_disconnect ();

  return 0;
}
