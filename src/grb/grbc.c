#define _GNU_SOURCE

#include <getopt.h>
#include <mcheck.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <libnova.h>

#include "../db/db.h"
#include "ibas/ibas.h"
#include "bacodine/bacodine.h"
#include "status.h"
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/mkpath.h"
#include "../utils/mv.h"
#include "../writers/fits.h"
#include "../writers/process_image.h"
#include "../plan/selector.h"
#include "../utils/config.h"

#include "../../config.h"

#include "image_info.h"

#define EXPOSURE_TIME		20

struct grb
{
  int tar_id;
  int grb_id;
  int obs_id;
  int seqn;
  time_t created;
  time_t last_update;
  struct ln_equ_posn object;
}
observing;

pthread_t image_que_thread;

time_t last_succes = 0;

pthread_t iban_thread;
pthread_mutex_t observing_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t observing_cond = PTHREAD_COND_INITIALIZER;

struct device *telescope;

int
get_info (struct grb *entry, struct device *tel, struct device *cam)
{
  struct image_info *info =
    (struct image_info *) malloc (sizeof (struct image_info));
  int ret;

  info->camera_name = cam->name;
  printf ("info camera_name = %s\n", cam->name);
  info->telescope_name = tel->name;
  info->exposure_time = time (NULL);
  info->exposure_length = EXPOSURE_TIME;
  info->target_id = entry->tar_id;
  info->observation_id = entry->obs_id;
  info->target_type = TARGET_LIGHT;
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
readout ()
{
  struct device *camera;
  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			  TEL_OBSERVING, 0);
  devcli_command_all (DEVICE_TYPE_CCD, "expose 0 1 %i", EXPOSURE_TIME);

  devcli_command (telescope, NULL, "info");
  for (camera = devcli_devices (); camera; camera = camera->next)
    get_info (&observing, telescope, camera);
  devcli_wait_for_status_all (DEVICE_TYPE_CCD, "img_chip",
			      CAM_MASK_EXPOSE, CAM_NOEXPOSURE, 0);
  devcli_command_all (DEVICE_TYPE_CCD, "readout 0");
  return 0;
}

int
process_grb_event (int id, int seqn, double ra, double dec, time_t * date)
{
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn hrz;

  observer.lat = 50;
  observer.lng = -14.96;

  get_hrz_from_equ (&object, &observer, get_julian_from_sys (), &hrz);

  if (observing.grb_id == id)
    {
      // cause I'm not interested in old updates
      if (observing.seqn < seqn)
	{
	  pthread_mutex_lock (&observing_lock);
	  observing.seqn = seqn;
	  observing.last_update = time (NULL);
	  object.ra = ra;
	  object.dec = dec;
	  observing.object = object;
	  pthread_cond_broadcast (&observing_cond);
	  pthread_mutex_unlock (&observing_lock);
	}
    }
  else if (observing.grb_id < id)	// -1 count as well..get the latest
    {
      if (hrz.alt > 10)		// start observation - if not above horizont, don't care, we already observe something else
	{
	  pthread_mutex_lock (&observing_lock);
	  db_update_grb (id, &seqn, &ra, &dec, date, &observing.tar_id);
	  observing.grb_id = id;
	  observing.seqn = seqn;
	  observing.created = *date;
	  observing.last_update = time (NULL);
	  object.ra = ra;
	  object.dec = dec;
	  observing.object = object;
	  pthread_cond_broadcast (&observing_cond);
	  pthread_mutex_unlock (&observing_lock);
	  return 0;
	}
    }

  // just add to planer
  return db_update_grb (id, &seqn, &ra, &dec, date, NULL);
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn hrz;
  int c;
  time_t t;
  int observing_count;
  int thread_count = 0;

#ifdef DEBUG
  mtrace ();
#endif
  observing.tar_id = -1;
  observing.grb_id = -1;

  printf ("Readign config file" CONFIG_FILE "...");
  if (read_config (CONFIG_FILE) == -1)
    printf ("failed, defaults will be used when apopriate.\n");
  else
    printf ("OK\n");

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
	      printf ("invalid server port option: %s\n", optarg);
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

  if (optind == argc)
    {
      server = "localhost";
    }
  else if (optind == argc - 1)
    {
      server = argv[optind++];
    }
  else
    {
      printf ("Only one extra parameter - server name - is allowed");
      return EXIT_FAILURE;
    }

  if (db_connect ())
    {
      perror ("grbc db_connect");
      return -1;
    }

  observer.lng = get_double_default ("longtitude", -14.95);
  observer.lat = get_double_default ("latitude", 50);	// Ondrejov as default

  printf ("GRB will be observerd at longtitude %f, latitude %f\n",
	  observer.lng, observer.lat);

  if (*(get_device_string_default ("grbc", "iban", "Y")) == 'Y')
    {
      printf ("Starting iban thread..");
      pthread_create (&iban_thread, NULL, receive_iban,
		      (void *) process_grb_event);
      thread_count++;
      printf ("OK\n");
    }
  if (*(get_device_string_default ("grbc", "bacodine", "Y")) == 'Y')
    {
      printf ("Starting bacodine (as server) thread...");
      pthread_create (&iban_thread, NULL, receive_bacodine,
		      (void *) process_grb_event);
      thread_count++;
      printf ("OK\n");
    }
  if (*(get_device_string_default ("grbc", "bacoclient", "Y")) == 'Y')
    {
      printf ("Starting bacodine (as server) thread...");
      pthread_create (&iban_thread, NULL, receive_bacodine,
		      (void *) process_grb_event);
      thread_count++;
      printf ("OK\n");
    }

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  devcli_device_data_handler (DEVICE_TYPE_CCD, data_handler);

  telescope = devcli_find (get_string_default ("telescope_name", "T0"));

  if (!telescope)
    {
      printf
	("**** telescope cannot be found.\n**** please check that's connected and teled is running.\n");
      exit (EXIT_FAILURE);
    }

  devcli_server_command (NULL, "status_txt grbc_waiting");

  pthread_create (&image_que_thread, NULL, process_images, NULL);

  while (1)
    {
      pthread_mutex_lock (&observing_lock);
      printf ("Waiting for GRB event.\n");
      fflush (stdout);
      while (observing.tar_id == -1)
	pthread_cond_wait (&observing_cond, &observing_lock);
      if (devcli_server ()->statutes[0].status != SERVERD_NIGHT
	  && devcli_server ()->statutes[0].status != SERVERD_DUSK
	  && devcli_server ()->statutes[0].status != SERVERD_DAWN)
	{
	  observing.tar_id = -1;
	  printf ("serverd not in correct state, continuiing\n");
	  pthread_mutex_unlock (&observing_lock);
	  continue;
	}
      printf ("Starting observing %i tar_id: %i\n", observing.tar_id,
	      observing.tar_id);

      devcli_command_all (DEVICE_TYPE_CCD, "ready;info");
      devcli_command (telescope, NULL, "ready;base_info;info");

      devcli_server_command (NULL, "priority 200");

      printf ("waiting for priority\n");

      devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("waiting end\n");

      time (&t);

      db_start_observation (observing.tar_id, &t, &observing.obs_id);

      get_hrz_from_equ (&observing.object, &observer, get_julian_from_sys (),
			&hrz);

      observing_count = 0;
      while (hrz.alt > 10
	     && (devcli_server ()->statutes[0].status == SERVERD_NIGHT
		 || devcli_server ()->statutes[0].status == SERVERD_DUSK
		 || devcli_server ()->statutes[0].status == SERVERD_DAWN))
	{
	  if (devcli_command
	      (telescope, NULL, "move %f %f", observing.object.ra,
	       observing.object.dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	    }
	  if (observing_count)
	    devcli_wait_for_status_all (DEVICE_TYPE_CCD, "img_chip",
					CAM_MASK_READING, CAM_NOTREADING, 0);
	  observing_count++;
	  pthread_mutex_unlock (&observing_lock);

	  devcli_wait_for_status_all (DEVICE_TYPE_CCD, "priority",
				      DEVICE_MASK_PRIORITY, DEVICE_PRIORITY,
				      0);

	  printf ("OK\n");
	  readout ();

	  pthread_mutex_lock (&observing_lock);
	  get_hrz_from_equ (&observing.object, &observer,
			    get_julian_from_sys (), &hrz);
	}

      devcli_server_command (NULL, "priority -1");

      t = time (NULL);

      db_end_observation (observing.obs_id, &t);
      pthread_mutex_unlock (&observing_lock);
    }

  db_disconnect ();
  return 0;
}
