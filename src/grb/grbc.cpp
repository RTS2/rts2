#include <getopt.h>
#include <mcheck.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <libnova/libnova.h>

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

#include "image_info.h"

double exposure_time;
#define EXPOSURE_TIME_DEFAULT   120
#define EXPOSURE_TIMEOUT	50

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
  struct timezone tz;

  info->camera_name = cam->name;
  printf ("info camera_name = %s\n", cam->name);
  info->telescope_name = tel->name;
  gettimeofday (&info->exposure_tv, &tz);
  info->exposure_length = exposure_time;
  info->target_id = entry->tar_id;
  info->observation_id = entry->obs_id;
  info->target_type = TARGET_LIGHT;
  if ((ret = devcli_command (cam, NULL, "info")))
    {
      printf ("camera info error\n");
    }
  memcpy (&info->camera, &cam->info, sizeof (struct camera_info));
  memcpy (&info->telescope, &tel->info, sizeof (struct telescope_info));
  info->hi_precision = NULL;
  devcli_image_info (cam, info);
  free (info);
  return ret;
}


int
readout ()
{
  struct device *camera;
  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			  TEL_OBSERVING, 120);
  devcli_command_all (DEVICE_TYPE_CCD, "expose 0 1 %f", exposure_time);

  devcli_command (telescope, NULL, "info");
  for (camera = devcli_devices (); camera; camera = camera->next)
    get_info (&observing, telescope, camera);
  devcli_wait_for_status_all (DEVICE_TYPE_CCD, "img_chip",
			      CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
			      1.1 * exposure_time + EXPOSURE_TIMEOUT);
  devcli_command_all (DEVICE_TYPE_CCD, "readout 0");
  return 0;
}

int
process_grb_event (int id, int seqn, double ra, double dec, time_t * date)
{
  struct ln_equ_posn object;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn hrz;

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
      object.ra = ra;
      object.dec = dec;

      observer.lng = get_double_default ("longtitude", 6.733);
      observer.lat = get_double_default ("latitude", 37.1);

      ln_get_hrz_from_equ (&object, &observer, ln_get_julian_from_sys (),
			   &hrz);

      if (hrz.alt >= -1)	// start observation - if not above horizont, don't care, we already observe something else
	{
	  pthread_mutex_lock (&observing_lock);
	  db_update_grb (id, &seqn, &ra, &dec, date, &observing.tar_id, true);
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
  return db_update_grb (id, &seqn, &ra, &dec, date, NULL, true);
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

  if ((c = db_connect ()))
    {
      fprintf (stderr, "cannot connect to db, SQL error code: %i\n", c);
      devcli_server_disconnect ();
      return 0;
    }

  observer.lng = get_double_default ("longtitude", 6.377);
  observer.lat = get_double_default ("latitude", 37.1);	// BOOTES as default
  exposure_time =
    get_device_double_default ("grbc", "exposure_time",
			       EXPOSURE_TIME_DEFAULT);

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
  while (devcli_server_login (server, port, "petr", "petr") == -1)
    perror ("devcli_server_login");


  while (!
	 (telescope =
	  devcli_find (get_string_default ("telescope_name", "T0"))))
    printf
      ("**** cannot find telescope!\n**** please check that it's connected and teld runs.\n");

  devcli_server_command (NULL, "status_txt grbc_waiting");

  pthread_create (&image_que_thread, NULL, process_images, NULL);

  while (1)
    {
      int tar_id;
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

      devcli_server_command (NULL, "info");
      devcli_command_all (DEVICE_TYPE_CCD, "ready");
      devcli_command_all (DEVICE_TYPE_CCD, "info");
      devcli_device_data_handler (DEVICE_TYPE_CCD, data_handler);
      devcli_command (telescope, NULL, "ready");
      devcli_command (telescope, NULL, "base_info");
      devcli_command (telescope, NULL, "info");

      devcli_server_command (NULL, "priority 200");

      printf ("waiting for priority\n");

      devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);
      printf ("waiting end\n");

      time (&t);

      db_start_observation (observing.tar_id, &t, &observing.obs_id);

      ln_get_hrz_from_equ (&observing.object, &observer,
			   ln_get_julian_from_sys (), &hrz);

      observing_count = 0;
      tar_id = observing.tar_id;
      while (hrz.alt > get_device_double_default ("grbc", "horizont", 0)
	     && (devcli_server ()->statutes[0].status == SERVERD_NIGHT
		 || devcli_server ()->statutes[0].status == SERVERD_DUSK
		 || devcli_server ()->statutes[0].status == SERVERD_DAWN))
	{
	  time (&t);
	  if (devcli_command
	      (telescope, NULL, "move %f %f", observing.object.ra,
	       observing.object.dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	    }
	  if (observing_count)
	    devcli_wait_for_status_all (DEVICE_TYPE_CCD, "img_chip",
					CAM_MASK_READING, CAM_NOTREADING,
					120);
	  observing_count++;
	  if (tar_id != observing.tar_id)	// tar_id was changed
	    {
	      db_end_observation (observing.obs_id, &t);
	      tar_id = observing.tar_id;
	      db_start_observation (tar_id, &t, &observing.obs_id);
	    }
	  pthread_mutex_unlock (&observing_lock);

	  devcli_wait_for_status_all (DEVICE_TYPE_CCD, "priority",
				      DEVICE_MASK_PRIORITY, DEVICE_PRIORITY,
				      0);

	  printf ("OK\n");
	  readout ();

	  pthread_mutex_lock (&observing_lock);
	  ln_get_hrz_from_equ (&observing.object, &observer,
			       ln_get_julian_from_sys (), &hrz);
	  if (observing.grb_id < 100 && observing_count > 10)
	    break;
	}

      devcli_server_command (NULL, "priority -1");

      t = time (NULL);

      db_end_observation (observing.obs_id, &t);
      if (observing.tar_id == tar_id)
	observing.tar_id = -1;
      pthread_mutex_unlock (&observing_lock);
    }

  db_disconnect ();
  return 0;
}
