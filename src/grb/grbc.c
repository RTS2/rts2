#include <getopt.h>
#include <mcheck.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include <libnova.h>

#include "../db/db.h"
#include "ibas/ibas.h"
#include "bacodine/bacodine.h"
#include "status.h"
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/mkpath.h"
#include "../writers/fits.h"

#include "image_info.h"

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

pthread_t iban_thread;
pthread_mutex_t observing_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t observing_cond = PTHREAD_COND_INITIALIZER;

int camd_id;
int teld_id;

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

  gmtime_r (&image->exposure_time, &gmt);

  if (asprintf (&dirname, "~/%i", image->target_id))
    {
      perror ("planc data_handler asprintf");
      return -1;
    }

  mkpath (dirname, 0777);

  strftime (filen, 250, "%Y%m%d%H%M%S.fits", &gmt);
  asprintf (&filename, "!%s/%s", dirname, filen);

  free (dirname);

  printf ("filename: %s", filename);

  if (fits_create (&receiver, filename) || fits_init (&receiver, size))
    {
      perror ("camc data_handler fits_init");
      free (filename);
      return -1;
    }

  free (filename);

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

  if (fits_write_image_info (&receiver, image, NULL)
      || fits_close (&receiver))
    {
      perror ("camc data_handler fits_write");
      return -1;
    }

  return 0;
#undef receiver
}

int
expose (int target_id, int observation_id, int npic)
{
  devcli_wait_for_status ("teld", "telescope", TEL_MASK_MOVING, TEL_STILL, 0);
  for (; npic > 0; npic--)
    {
      struct image_info *info;
      time_t t;

      union devhnd_info *devinfo;

      printf ("exposure countdown %i..\n", npic);
      if (devcli_wait_for_status ("camd", "img_chip", CAM_MASK_READING,
				  CAM_NOTREADING, 0) ||
	  devcli_command (camd_id, NULL, "expose 0 1 120"))
	return -1;
      t = time (NULL);
      info = (struct image_info *) malloc (sizeof (struct image_info));
      info->exposure_time = t;
      info->exposure_length = 120;
      info->target_id = target_id;
      info->observation_id = observation_id;
      if (devcli_command (camd_id, NULL, "info") ||
	  devcli_getinfo (camd_id, &devinfo) ||
	  !memcpy (&info->camera, devinfo, sizeof (struct camera_info)) ||
	  devcli_command (teld_id, NULL, "info") ||
	  devcli_getinfo (teld_id, &devinfo) ||
	  !memcpy (&info->telescope, devinfo, sizeof (struct telescope_info)) ||
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
process_grb_event (int id, int seqn, double ra, double dec)
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
	  observing.created = observing.last_update = time (NULL);
	  observing.object = object;
	  db_update_grb (id, seqn, ra, dec, &observing.tar_id);
	  pthread_cond_broadcast (&observing_cond);
	  pthread_mutex_unlock (&observing_lock);
	  return 0;
	}
    }

  // just add to planer
  return db_update_grb (id, seqn, ra, dec, NULL);
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
				  continue;\
				}

#define TELD_WRITE_READ(command) if (devcli_command (teld_id, NULL, command) < 0) \
  				{ \
      		                  perror ("devcli_write_read"); \
			          continue;\
				}
  while (1)
    {
      pthread_mutex_lock (&observing_lock);
      while (observing.id == -1)
	{
	  pthread_cond_wait (&observing_cond, &observing_lock);
	}
      printf ("Starting observing %i tar_id: %i\n", observing.id,
	      observing.tar_id);

      CAMD_WRITE_READ ("ready");
      CAMD_WRITE_READ ("info");

      TELD_WRITE_READ ("ready");
      TELD_WRITE_READ ("info");


      devcli_server_command (NULL, "priority 200");

      printf ("waiting for priority\n");

      devcli_wait_for_status ("teld", "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);

      printf ("waiting end\n");

      time (&t);

      db_start_observation (observing.tar_id, &t, &obs_id);

      get_hrz_from_equ (&observing.object, &observer, get_julian_from_sys (),
			&hrz);

      while (hrz.alt > 10)
	{
	  if (devcli_command
	      (teld_id, NULL, "move %f %f", observing.object.ra,
	       observing.object.dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	    }
	  pthread_mutex_unlock (&observing_lock);

	  devcli_wait_for_status ("camd", "priority", DEVICE_MASK_PRIORITY,
				  DEVICE_PRIORITY, 0);

	  printf ("OK\n");
	  expose (observing.tar_id, obs_id, 1);

	  pthread_mutex_lock (&observing_lock);
	  get_hrz_from_equ (&observing.object, &observer,
			    get_julian_from_sys (), &hrz);
	}

      pthread_mutex_unlock (&observing_lock);
      db_end_observation (obs_id, time (NULL) - t);
    }

  db_disconnect ();
  return 0;
}
