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

#include "image_info.h"

#define EXPOSURE_TIME		120
#define EPOCH			"002"

struct grb
{
  int tar_id;
  int id;
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

struct device *camera, *telescope;

int
readout (struct grb *object)
{
  struct image_info *info =
    (struct image_info *) malloc (sizeof (struct image_info));

  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			  TEL_OBSERVING, 0);
  devcli_command (camera, NULL, "expose 0 1 %i", EXPOSURE_TIME);

  info->exposure_time = time (NULL);
  info->exposure_length = EXPOSURE_TIME;
  info->target_id = object->tar_id;
  info->observation_id = object->id;
  info->target_type = -1;
  info->camera_name = camera->name;
  info->telescope_name = telescope->name;
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
process_grb_event (int id, int seqn, double ra, double dec, time_t * date)
{
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn hrz;

  object.ra = ra;
  object.dec = dec;
  observer.lat = 50;
  observer.lng = -14.96;

  get_hrz_from_equ (&object, &observer, get_julian_from_sys (), &hrz);

  if (observing.id == id)
    {
      // cause I'm not interested in old updates
      if (observing.seqn < seqn)
	{
	  pthread_mutex_lock (&observing_lock);
	  observing.seqn = seqn;
	  observing.last_update = time (NULL);
	  observing.object = object;
	  pthread_cond_broadcast (&observing_cond);
	  pthread_mutex_unlock (&observing_lock);
	}
    }

  if (observing.id < id)	// -1 count as well..
    {
      if (hrz.alt > 10)		// start observation
	{
	  pthread_mutex_lock (&observing_lock);
	  observing.id = id;
	  observing.seqn = -1;	// filled down
	  observing.seqn = seqn;
	  observing.created = *date;
	  observing.last_update = time (NULL);
	  observing.object = object;
	  db_update_grb (id, seqn, ra, dec, date, &observing.tar_id);
	  pthread_cond_broadcast (&observing_cond);
	  pthread_mutex_unlock (&observing_lock);
	  return 0;
	}
    }

  // just add to planer
  return db_update_grb (id, seqn, ra, dec, date, NULL);
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
  int obs_id;

#ifdef DEBUG
  mtrace ();
#endif
  observing.id = -1;

  observer.lat = 50;
  observer.lng = -14.96;
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

  pthread_create (&iban_thread, NULL, receive_iban,
		  (void *) process_grb_event);
  pthread_create (&iban_thread, NULL, receive_bacodine,
		  (void *) process_grb_event);

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  camera = devcli_find ("C0");

  if (!camera)
    {
      printf
	("**** camera cannot be found.\n**** please check that's connected and camd is running.\n");
      exit (EXIT_FAILURE);
    }

  camera->data_handler = data_handler;

  telescope = devcli_find ("T1");

  if (!telescope)
    {
      printf
	("**** telescope cannot be found.\n**** please check that's connected and teled is running.\n");
      exit (EXIT_FAILURE);
    }

  devcli_server_command (NULL, "status_txt grbc_waiting");

#define CAMD_WRITE_READ(command) if (devcli_command (camera, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read"); \
				  continue;\
				}

#define TELD_WRITE_READ(command) if (devcli_command (telescope, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read"); \
			          continue;\
				}
  while (1)
    {
      pthread_mutex_lock (&observing_lock);
      printf ("Waiting for GRB event.\n");
      fflush (stdout);
      while (observing.id == -1)
	{
	  pthread_cond_wait (&observing_cond, &observing_lock);
	}
      if (devcli_server ()->statutes[0].status != SERVERD_NIGHT)
	{
	  printf ("serverd not in correct state, continuiing\n");
	  continue;
	}
      printf ("Starting observing %i tar_id: %i\n", observing.id,
	      observing.tar_id);

      CAMD_WRITE_READ ("ready");
      CAMD_WRITE_READ ("info");

      TELD_WRITE_READ ("ready");
      TELD_WRITE_READ ("info");


      devcli_server_command (NULL, "priority 200");

      printf ("waiting for priority\n");

      devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("waiting end\n");

      time (&t);

      db_start_observation (observing.tar_id, &t, &obs_id);

      get_hrz_from_equ (&observing.object, &observer, get_julian_from_sys (),
			&hrz);

      while (hrz.alt > 10)
	{
	  if (devcli_command
	      (telescope, NULL, "move %f %f", observing.object.ra,
	       observing.object.dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	    }
	  pthread_mutex_unlock (&observing_lock);

	  devcli_wait_for_status (camera, "priority", DEVICE_MASK_PRIORITY,
				  DEVICE_PRIORITY, 0);

	  printf ("OK\n");
	  readout (&observing);

	  pthread_create (&image_que_thread, NULL, process_images, NULL);

	  pthread_mutex_lock (&observing_lock);
	  get_hrz_from_equ (&observing.object, &observer,
			    get_julian_from_sys (), &hrz);
	}

      t = time (NULL);

      pthread_mutex_unlock (&observing_lock);
      db_end_observation (observing.tar_id, obs_id, &t);
    }

  db_disconnect ();
  return 0;
}
