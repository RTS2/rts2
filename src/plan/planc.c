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
#include <termios.h>
#include <signal.h>
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

#define EXPOSURE_TIME		120
#define EPOCH			"002"

struct device *camera, *telescope;
#ifdef USE_WF2
struct device *wf2;
#endif
struct target *plan;

char *dark_name = NULL;

struct image_que
{
  char *image;
  char *directory;
  int correction_mark;
  struct image_que *previous;
}
 *image_que;

pthread_mutex_t image_que_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t image_que_cond = PTHREAD_COND_INITIALIZER;
pthread_t image_que_thread;

time_t last_succes = 0;


/*!
 * Handle image processing
 */
void *
process_images (struct device *tel)
{
  struct image_que *actual_image;

  char *cmd, *filename;

  long int id;
  double ra, dec, ra_err, dec_err;

  int ret = 0;

  FILE *past_out;

  while (1)
    {
      // wait for image to process
      pthread_mutex_lock (&image_que_mutex);
      while (!image_que)
	pthread_cond_wait (&image_que_cond, &image_que_mutex);
      actual_image = image_que;
      image_que = image_que->previous;
      pthread_mutex_unlock (&image_que_mutex);

      printf ("chdir %s\n", actual_image->directory);

      if ((ret = chdir (actual_image->directory)))
	{
	  perror ("chdir");
	  goto free_image;
	}

      // call image processing script
      asprintf (&cmd, "/home/petr/rts2/src/plan/img_process %s",
		actual_image->image);
      printf ("\ncalling %s.", cmd);

      if (!(past_out = popen (cmd, "r")))
	{
	  perror ("popen");
	  free (cmd);
	  ret = -1;
	  goto free_image;
	};

      free (cmd);

      printf ("OK\n");

      do
	{
	  char strs[200];
	  if (!fgets (strs, 200, past_out))
	    {
	      ret = EOF;
	      break;
	    }

	  strs[199] = 0;

	  printf ("get: %s\n", strs);

	  ret = sscanf
	    (strs, "%li %lf %lf (%lf,%lf)", &id, &ra, &dec, &ra_err,
	     &dec_err);
	}
      while (!(ret == EOF || ret == 5));

      pclose (past_out);

      filename =
	(char *) malloc (strlen (actual_image->directory) +
			 strlen (actual_image->image) + 2);

      strcpy (filename, actual_image->directory);
      strcat (filename, "/");
      strcat (filename, actual_image->image);

      if (ret == EOF)
	{
	  // bad image, move to trash
	  char *trash_name;
	  asprintf (&trash_name, "/trash/%s", actual_image->image);
	  printf ("%s scanf error, invalid line\n", filename);
	  printf ("mv %s -> %s", filename, trash_name);
	  if ((ret = (mv (filename, trash_name))))
	    perror ("rename bad image");
	  printf ("..OK\n");
	  free (trash_name);
	}
      else
	{

	  printf ("ra: %f dec: %f ra_err: %f min dec_err: %f min\n", ra, dec,
		  ra_err, dec_err);

	  last_succes = time (NULL);

	  if (actual_image->correction_mark >= 0)
	    {
	      if (devcli_command (tel, NULL, "correct %i %f %f",
				  actual_image->correction_mark,
				  ra_err / 60.0, dec_err / 60.0))
		perror ("telescope correct");
	    }


	  // add image to db
	  asprintf (&cmd, "/home/petr/fitsdb/wcs/wcs2db %s|psql stars",
		    filename);

	  if (system (cmd))
	    {
	      perror ("calling wcs2db");
	    }

	  free (cmd);
	}

      free (filename);

    free_image:
      free (actual_image->directory);
      free (actual_image->image);
      free (actual_image);
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
data_handler (int sock, size_t size, struct image_info *image)
{
  struct fits_receiver_data receiver;
  struct tm gmt;
  char data[DATA_BLOCK_SIZE];
  int ret = 0;
  ssize_t s;
  char *filen;
  char *filename;
  char *dirname;

  gmtime_r (&image->exposure_time, &gmt);

  switch (image->target_type)
    {
    case TARGET_DARK:
      if (!asprintf (&dirname, "/darks/%s/%s/", EPOCH, image->camera_name))
	{
	  perror ("planc data_handler asprintf");
	  return -1;
	}
      break;
    default:
      if (!asprintf
	  (&dirname, "/images/%s/%s/%05i/", EPOCH, image->camera_name,
	   image->target_id))
	{
	  perror ("planc data_handler asprintf");
	  return -1;
	}
    }

  mkpath (dirname, 0777);

  filen = (char *) malloc (25);

  strftime (filen, 24, "%Y%m%d%H%M%S.fits", &gmt);
  filen[24] = 0;
  asprintf (&filename, "%s%s", dirname, filen);

  if (fits_create (&receiver, filename) || fits_init (&receiver, size))
    {
      printf ("camc data_handler fits_init\n");
      ret = -1;
      goto free_filen;
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
      if (sock < 0 || size != 3145784)
	{
	  printf ("socket error: %i", sock);
	  exit (EXIT_FAILURE);
	}

      if ((ret = fits_handler (data, s, &receiver)) < 0)
	goto free_filen;
      if (ret == 1)
	break;
    }

  if (s < 0)
    {
      perror ("camc data_handler");
      ret = -1;
      goto free_filen;
    }

  printf ("reading finished\n");

  if (fits_write_image_info (&receiver, image, dark_name)
      || fits_close (&receiver))
    {
      perror ("camc data_handler fits_write");
      ret = -1;
      goto free_filen;
    }

  switch (image->target_type)
    {
      struct image_que *new_que;
    case TARGET_LIGHT:
      printf ("putting %s to que\n", filename);

      new_que = (struct image_que *) malloc (sizeof (struct image_que));
      new_que->directory = dirname;
      new_que->image = filen;

      if (strcmp (image->camera_name, "C0") == 0)
	new_que->correction_mark = image->telescope.correction_mark;
      else
	new_que->correction_mark = -1;


      pthread_mutex_lock (&image_que_mutex);
      new_que->previous = image_que;
      image_que = new_que;
      pthread_cond_broadcast (&image_que_cond);
      pthread_mutex_unlock (&image_que_mutex);

      ret = 0;

      goto free_filename;

      break;

    case TARGET_DARK:
      printf ("darkname: %s\n", dark_name);

      ret = db_add_darkfield (dark_name, &image->exposure_time,
			      image->exposure_length,
			      image->camera.ccd_temperature * 10);
      break;
    }

free_filen:
  free (filen);
  free (dirname);

free_filename:
  free (filename);
  return ret;
#undef receiver
}

int
status_handler (struct device *dev, char *status_name, int new_val)
{
  if (strcmp (status_name, "img_chip") == 0)
    {
      if ((dev->statutes[0].status & CAM_EXPOSING) && (new_val & CAM_DATA))
	devcli_command (dev, NULL, "readout 0");
    }
  return 0;
}

int
move (struct target *last)
{
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
	get_hrz_from_equ (&object, &observer, get_julian_from_sys (), &hrz);
	printf ("Ra: %f Dec: %f\n", object.ra, object.dec);
	printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);
      }

      if (devcli_command (telescope, NULL, "move %f %f", last->ra, last->dec))
	{
	  printf ("telescope error\n\n--------------\n");
	  return -1;
	}
    }
  return 0;
}

int
get_info (struct target *entry, struct device *tel, struct device *cam)
{
  struct image_info *info =
    (struct image_info *) malloc (sizeof (struct image_info));

  info->camera_name = cam->name;
  info->telescope_name = tel->name;
  info->exposure_time = time (NULL);
  info->exposure_length = EXPOSURE_TIME;
  info->target_id = entry->id;
  info->observation_id = entry->obs_id;
  info->target_type = entry->type;
  if (devcli_command (cam, NULL, "info") ||
      !memcpy (&info->camera, &cam->info, sizeof (struct camera_info)) ||
      devcli_command (tel, NULL, "info") ||
      !memcpy (&info->telescope, &tel->info,
	       sizeof (struct telescope_info))
      || devcli_image_info (cam, info))
    {
      free (info);
      return -1;
    }
  free (info);
  return 0;
}

int
observe (int watch_status)
{
  int i = 0;
  struct target *last;
  time_t start_time;

  struct tm last_s;

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

  for (last = plan; last; plan = last, last = last->next, free (plan))
    {
      time_t t = time (NULL);
      struct timeval tv;
      fd_set rdfs;

      FD_ZERO (&rdfs);
      FD_SET (0, &rdfs);

      tv.tv_sec = 0;
      tv.tv_usec = 0;

      if (watch_status
	  && devcli_server ()->statutes[0].status != SERVERD_NIGHT)
	break;

      if (last->ctime < t)
	{
	  printf ("ctime %li (%s)", last->ctime, ctime (&last->ctime));
	  printf (" < t %li (%s)", t, ctime (&t));
	  goto generate_next;
	}

      move (last);

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

      gmtime_r (&last_succes, &last_s);

      devcli_server_command (NULL,
			     "status_txt P:_%i_obs:_%i_ra:%i_dec:%i_suc:%i:%i",
			     last->id, last->obs_id, (int) last->ra,
			     (int) last->dec, last_s.tm_hour, last_s.tm_min);

      devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

#if USE_WF2
      devcli_wait_for_status (wf2, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);
#endif
      devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			      TEL_OBSERVING, 0);

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
	  perror ("expose camera");
	}

#if USE_WF2
      if (devcli_wait_for_status (wf2, "img_chip", CAM_MASK_READING,
				  CAM_NOTREADING, 0) ||
	  devcli_command (wf2, NULL, "expose 0 %i %i",
			  last->type != TARGET_DARK, EXPOSURE_TIME))
	{
	  perror ("expose wf2");
	}
#endif

    generate_next:
      // generate next plan entry
      i++;


      plan = (struct target *) malloc (sizeof (struct target));

      printf ("Making next plan..\n");

      get_next_plan (plan, SELECTOR_AIRMASS, last, start_time, i,
		     EXPOSURE_TIME);
      last->next = plan;
      printf ("next plan #%i: id %i type %i\n", i, last->next->id,
	      last->next->type);

      get_info (last, telescope, camera);
#if USE_WF2
      get_info (last, telescope, wf2);
#endif

      devcli_wait_for_status (camera, "img_chip", CAM_MASK_EXPOSE,
			      CAM_NOEXPOSURE, 0);
#if USE_WF2
      devcli_wait_for_status (wf2, "img_chip", CAM_MASK_EXPOSE,
			      CAM_NOEXPOSURE, 0);
#endif

      move (last->next);

      devcli_command (camera, NULL, "readout 0");
#if USE_WF2
      devcli_command (wf2, NULL, "readout 0");
#endif

      printf ("after readout\n");

      if (last->type == TARGET_LIGHT)
	{
	  db_end_observation (last->id, last->obs_id, &last->next->ctime);
	}
    }
  return 0;
}


int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;

  char *server;

  int c = 0;

  int priority = 20;

  int watch_status = 1;		// watch central server status

#ifdef DEBUG
  mtrace ();
#endif

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"ignore_status", 0, 0, 'i'},
	{"port", 1, 0, 'p'},
	{"priority", 1, 0, 'r'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "ip:r:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'i':
	  watch_status = 0;
	  break;
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
  if (optind == argc - 1)
    {
      server = argv[optind++];
    }
  else if (optind == argc)
    {
      server = "localhost";
    }
  else
    {
      printf ("**** only legal parameter is server name\n");
      return EXIT_FAILURE;

    }


  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      return EXIT_FAILURE;
    }

  if (watch_status && devcli_server ()->statutes[0].status != SERVERD_NIGHT)
    {
      printf ("**** serverd in invalid state: %i, should be in %i\n",
	      devcli_server ()->statutes[0].status, SERVERD_NIGHT);
      devcli_server_disconnect ();
      return EXIT_FAILURE;
    }

  camera = devcli_find ("C0");
  if (!camera)
    {
      printf
	("**** cannot find camera!\n**** please check that it's connected and camd runs.");
      devcli_server_disconnect ();
      return 0;
    }

  camera->data_handler = data_handler;

#if USE_WF2
  wf2 = devcli_find ("C1");
  if (!wf2)
    {
      printf
	("**** cannot find camera!\n**** please check that it's connected and camd runs.");
      devcli_server_disconnect ();
      return 0;
    }

  wf2->data_handler = data_handler;
#endif

  telescope = devcli_find ("teld");
  if (!telescope)
    {
      printf
	("**** cannot find telescope!\n**** please check that it's connected and teld runs.");
      devcli_server_disconnect ();
      return 0;
    }

  /* connect to db */

  if (c == db_connect ())
    {
      fprintf (stderr, "cannot connect to db, SQL error code: %i\n", c);
      devcli_server_disconnect ();
      return 0;
    }

#define CAMD_WRITE_READ(command) if (devcli_command (camera, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_camd"); \
				  return EXIT_FAILURE; \
				}

#if USE_WF2
#define WF2_WRITE_READ(command) if (devcli_command (wf2, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_camd"); \
				  return EXIT_FAILURE; \
				}
#endif

#define TELD_WRITE_READ(command) if (devcli_command (telescope, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read_teld"); \
				  return EXIT_FAILURE; \
				}
  CAMD_WRITE_READ ("ready");
  CAMD_WRITE_READ ("info");

#if USE_WF2
  WF2_WRITE_READ ("ready");
  WF2_WRITE_READ ("info");
#endif

  TELD_WRITE_READ ("ready");
  TELD_WRITE_READ ("info");

  devcli_set_general_notifier (camera, status_handler, NULL);

  srandom (time (NULL));

  umask (0x002);

  devcli_server_command (NULL, "priority %i", priority);

  printf ("waiting for priority\n");

  devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);

  printf ("waiting end\n");

  if (watch_status)
    {
      while (devcli_server ()->statutes[0].status != SERVERD_NIGHT)
	{
	  printf ("wating for night..\n");
	  sleep (60);
	}

      image_que = NULL;
      pthread_create (&image_que_thread, NULL, process_images, telescope);

      observe (watch_status);
    }

  printf ("done\n");

  devcli_command (telescope, NULL, "park");

  devcli_server_disconnect ();
  db_disconnect ();

  return 0;
}
