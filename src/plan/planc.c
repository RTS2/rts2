/*!
 * @file Plan test client
 * $Id$
 *
 * Planner client.
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
#include "../writers/fits.h"
#include "status.h"
#include "selector.h"
#include "../db/db.h"
#include "../writers/process_image.h"

#define EXPOSURE_TIME		60
#define EPOCH			"002"

struct device *telescope;

pthread_t image_que_thread;

time_t last_succes = 0;

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
  if ((last->type == TARGET_LIGHT || last->type == TARGET_FLAT)
      && last->moved == 0)
    {
      struct ln_equ_posn object;
      struct ln_lnlat_posn observer;
      struct ln_hrz_posn hrz;
      object.ra = last->ra;
      object.dec = last->dec;
      observer.lat = 37.1;
      observer.lng = 6.377;
      get_hrz_from_equ (&object, &observer, get_julian_from_sys (), &hrz);
      printf ("Ra: %f Dec: %f\n", object.ra, object.dec);
      printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);

      last->moved = 1;

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
  int ret;

  info->camera_name = cam->name;
  printf ("info camera_name = %s\n", cam->name);
  info->telescope_name = tel->name;
  info->exposure_time = time (NULL);
  info->exposure_length = EXPOSURE_TIME;
  info->target_id = entry->id;
  info->observation_id = entry->obs_id;
  info->target_type = entry->type;
  if ((ret = devcli_command (cam, NULL, "info")))
    {
      printf ("camera info error\n");
    }
  memcpy (&info->camera, &cam->info, sizeof (struct camera_info));
  memcpy (&info->telescope, &tel->info, sizeof (struct telescope_info));
  devcli_image_info (cam, info);
  free (info);
  return ret;
}

int
generate_next (int i, struct target *plan)
{
  time_t start_time;
  time (&start_time);
  start_time += EXPOSURE_TIME;

  printf ("Making plan %s", ctime (&start_time));
  if (get_next_plan
      (plan, SELECTOR_AIRMASS, &start_time, i, EXPOSURE_TIME,
       devcli_server ()->statutes[0].status, 6.733, 37.1))
    {
      printf ("Error making plan\n");
      exit (EXIT_FAILURE);
    }
  printf ("...plan made\n");
  i++;
  return i;
}

int
observe (int watch_status)
{
  int i = 0;
  int tar_id = 0;
  int obs_id = -1;
  struct target *last, *plan, *p;
  int exposure;
  int light;

  struct tm last_s;

  plan = (struct target *) malloc (sizeof (struct target));
  plan->next = NULL;
  plan->id = -1;
  i = generate_next (i, plan);
  last = plan->next;
  free (plan);
  plan = last;

  for (; last; last = last->next, p = plan, plan = plan->next, free (p))
    {
      time_t t = time (NULL);
      struct timeval tv;
      struct device *camera = devcli_find ("C0");
      fd_set rdfs;

      FD_ZERO (&rdfs);
      FD_SET (0, &rdfs);

      tv.tv_sec = 0;
      tv.tv_usec = 0;

      if (watch_status
	  && (devcli_server ()->statutes[0].status != SERVERD_NIGHT &&
	      devcli_server ()->statutes[0].status != SERVERD_DUSK &&
	      devcli_server ()->statutes[0].status != SERVERD_DAWN))
	break;

      if (abs (last->ctime - t) > last->tolerance)
	{
	  printf ("ctime %li (%s)", last->ctime, ctime (&last->ctime));
	  printf (" < t %li (%s)", t, ctime (&t));
	  generate_next (i, plan);
	  continue;
	}

      move (last);

      while ((t = time (NULL)) < last->ctime - last->tolerance)
	{
	  time_t sleep_time = last->ctime - last->tolerance;
	  printf ("sleeping for %li sec, till %s\n", sleep_time - t,
		  ctime (&sleep_time));
	  sleep (sleep_time - t);
	}


      gmtime_r (&last_succes, &last_s);

      devcli_server_command (NULL,
			     "status_txt P:_%i_obs:_%i_ra:%i_dec:%i_suc:%i:%i",
			     last->id, last->obs_id, (int) last->ra,
			     (int) last->dec, last_s.tm_hour, last_s.tm_min);

      devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      devcli_wait_for_status_all (DEVICE_TYPE_CCD, "priority",
				  DEVICE_MASK_PRIORITY, DEVICE_PRIORITY, 0);

      devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			      TEL_OBSERVING, 0);

      printf ("OK\n");
      time (&t);
      exposure = (last->type == TARGET_FLAT
		  || last->type == TARGET_FLAT_DARK) ? 1 : EXPOSURE_TIME;
      light = !(last->type == TARGET_DARK || last->type == TARGET_FLAT_DARK);

      devcli_wait_for_status_all (DEVICE_TYPE_CCD, "img_chip",
				  CAM_MASK_READING, CAM_NOTREADING, 0);

      if (obs_id >= 0 && last->id != tar_id)
	{
	  if (camera)
	    t = camera->statutes[0].last_update;
	  else
	    time (&t);
	  db_end_observation (obs_id, &camera->statutes[0].last_update);
	  obs_id = -1;
	}

      if (last->type == TARGET_LIGHT)
	{
	  if (last->id != tar_id)
	    {
	      time (&t);
	      tar_id = last->id;
	      db_start_observation (last->id, &t, &obs_id);
	    }
	  last->obs_id = obs_id;
	}
      devcli_command_all (DEVICE_TYPE_CCD, "expose 0 %i %i", light, exposure);

      printf ("exposure countdown ..%s", ctime (&t));
      t += EXPOSURE_TIME;
      printf ("readout at: %s", ctime (&t));
      i = generate_next (i, plan);
      printf ("next plan #%i: id %i type %i\n", i, last->next->id,
	      last->next->type);
      devcli_command (telescope, NULL, "info");
      for (camera = devcli_devices (); camera; camera = camera->next)
	get_info (last, telescope, camera);
      devcli_wait_for_status_all (DEVICE_TYPE_CCD, "img_chip",
				  CAM_MASK_EXPOSE, CAM_NOEXPOSURE, 0);
      move (last->next);
      devcli_command_all (DEVICE_TYPE_CCD, "readout 0");
    }
  if (obs_id >= 0)
    {
      time_t t;
      time (&t);
      db_end_observation (obs_id, &t);
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
	{
	 "ignore_status", 0, 0, 'i'}
	,
	{
	 "port", 1, 0, 'p'}
	,
	{
	 "priority", 1, 0, 'r'}
	,
	{
	 "help", 0, 0, 'h'}
	,
	{
	 0, 0, 0, 0}
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

  devcli_device_data_handler (DEVICE_TYPE_CCD, data_handler);

  telescope = devcli_find ("T0");
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

  if (devcli_command_all (DEVICE_TYPE_CCD, "ready;info") < 0)
    {
      perror ("devcli_write_read_camd camera");
      return EXIT_FAILURE;
    }
  if (devcli_command (telescope, NULL, "ready;base_info;info") < 0)
    {
      perror ("devcli_write_read_camd camera");
      return EXIT_FAILURE;
    }

//  devcli_set_general_notifier (camera, status_handler, NULL);

  srandom (time (NULL));

  umask (0x002);

  devcli_server_command (NULL, "priority %i", priority);

  printf ("waiting for priority on telescope");
  fflush (stdout);

  devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);

  printf ("waiting end\n");

  pthread_create (&image_que_thread, NULL, process_images, NULL);

loop:
  devcli_server_command (NULL, "status_txt P:waiting");
  if (watch_status)
    {
      while (!(devcli_server ()->statutes[0].status == SERVERD_NIGHT
	       || devcli_server ()->statutes[0].status == SERVERD_DUSK
	       || devcli_server ()->statutes[0].status == SERVERD_DAWN))
	{
	  printf ("waiting for night..\n");
	  fflush (stdout);
	  devcli_command (telescope, NULL, "park");
	  sleep (600);
	}
    }
  observe (watch_status);
  printf ("done\n");

  devcli_command (telescope, NULL, "park");

  sleep (300);

  if (watch_status)
    goto loop;

  devcli_server_disconnect ();
  db_disconnect ();

  return 0;
}
