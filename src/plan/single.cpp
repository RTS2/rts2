/*!
 * @file Plan test client
 * $Id$
 *
 * Planner client.
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /*! _GNU_SOURCE */

#include <errno.h>
#include <ctype.h>
#include <libnova/libnova.h>
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
#include <mcheck.h>
#include <math.h>
#include <getopt.h>

#include "../utils/config.h"
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../writers/fits.h"
#include "status.h"
#include "target.h"
#include "selector.h"
#include "camera_info.h"
#include "phot_info.h"
#include "../db/db.h"
#include "../writers/process_image.h"
#include "centering.h"

char *phot_filters[] = {
  "A", "B", "C", "D", "E", "F"
};

struct device *telescope;

static int phot_obs_id = 0;

struct ln_lnlat_posn observer;

pthread_t image_que_thread;

time_t last_succes = 0;

int watch_status = 1;		// watch central server status

int target_id = -1;

int
phot_handler (struct param_status *params, struct phot_info *info)
{
  time_t t;
  time (&t);
  int ret;
  if (!strcmp (params->param_argv, "filter"))
    return param_next_integer (params, &info->filter);
  if (!strcmp (params->param_argv, "integration"))
    return param_next_integer (params, &info->integration);
  if (!strcmp (params->param_argv, "count"))
    {
      char tc[30];
      ctime_r (&t, tc);
      tc[strlen (tc) - 1] = 0;
      ret = param_next_integer (params, &info->count);
      printf ("%05i %s %i %i \n", phot_obs_id, tc, info->filter, info->count);
      fflush (stdout);
      if (telescope)
	{
	  db_add_count (phot_obs_id, &t, info->count, info->integration,
			phot_filters[info->filter],
			telescope->info.telescope.ra,
			telescope->info.telescope.dec, "PHOT0");
	}
    }
  return ret;
}

int
generate_next (Target * plan)
{
  while (plan->next)
    plan = plan->next;

  return createTarget (&plan->next, target_id, telescope, &observer);
}

// returns 1, when we are in state allowing observation
int
ready_to_observe (int status)
{
  char *autoflats;
  autoflats = get_string_default ("autoflats", "Y");
  if (autoflats[0] == 'Y')
    return status == SERVERD_NIGHT
      || status == SERVERD_DUSK || status == SERVERD_DAWN;
  return status == SERVERD_NIGHT;
}

int
observe (int hi_precission)
{
  int i = 0;
  int tar_id = 0;
  int obs_id = -1;
  Target *last, *plan, *p, *next;
  int exposure;
  int light;
  int start_move_count;

  int ret;

  struct tm last_s;

  plan = new Target (telescope, &observer);
  plan->next = NULL;
  plan->id = -1;
  ret = generate_next (plan);
  if (!ret)
    return ret;
  last = plan->next;
  last->setHiPrecission (hi_precission);
  free (plan);
  plan = last;

  devcli_command (telescope, NULL, "info");

  for (; last; last = last->next, p = plan, plan = plan->next)
    {
      time_t t = time (NULL);
      struct device *camera;

      // call next observation
      last->observe (p);
    }

  if (obs_id >= 0)
    {
      time_t t;
      time (&t);
      db_end_observation (obs_id, &t);
    }

  free (plan);
  return 0;
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  int c = 0;
  int priority = 20;
  struct device *devcli_ser = devcli_server ();
  struct device *phot;

  char *horizont_file;

  int hi_precission = (int) get_double_default ("hi_precission", 0);

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
	 "guidance", 1, 0, 'g'},
	{
	 "help", 0, 0, 'h'}
	,
	{
	 0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "aip:r:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'i':
	  watch_status = 0;
	  break;
	case 'p':
	  port = atoi (optarg);
	  if (port < 1)
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
	case 'g':
	  hi_precission = atoi (optarg);
	  break;
	case 't':
	  target_id = atoi (optarg);
	  break;
	case 'h':
	  printf
	    ("Options:\n"
	     "\tport|p <port_num>       port of the server\n"
	     "\tpriority|r <priority>   priority to run at\n"
	     "\tignore_status|i         run even when you don't have priority\n"
	     "\tguidance|g              guidance type - overwrite hi_precission from config\n"
	     "\ttarget_id|t		target_id of observation we would like to observe\n");
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

  if (target_id <= 0)
    {
      printf ("**** you have to specify target id\n");
      return EXIT_FAILURE;
    }

  printf ("Readign config file" CONFIG_FILE "...");
  if (read_config (CONFIG_FILE) == -1)
    printf ("failed, defaults will be used when needed.\n");
  else
    printf ("ok\n");
  observer.lng = get_double_default ("longtitude", 0);
  observer.lat = get_double_default ("latitude", 0);
  printf ("lng: %f lat: %f\n", observer.lng, observer.lat);

  horizont_file = get_string_default ("horizont", "/etc/rts2/horizont");
  printf ("Horizont will be read from: %s\n", horizont_file);

  ObjectCheck *checker = new ObjectCheck (horizont_file);

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      return EXIT_FAILURE;
    }

  devcli_device_data_handler (DEVICE_TYPE_CCD, data_handler);

  telescope = devcli_find (get_string_default ("telescope_name", "T0"));
  if (!telescope)
    {
      printf
	("**** cannot find telescope!\n**** please check that it's connected and teld runs.\n");
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

  if (devcli_command_all (DEVICE_TYPE_CCD, "ready") < 0
      || devcli_command_all (DEVICE_TYPE_CCD, "info") < 0)
    {
      perror ("devcli_write_read_camd camera");
      //return EXIT_FAILURE;
    }
  if (devcli_command (telescope, NULL, "ready;info") < 0)
    {
      perror ("devcli_write_read telescope");
      //return EXIT_FAILURE;
    }

//  devcli_set_general_notifier (camera, status_handler, NULL);

  srandom (time (NULL));

  umask (0x002);

  devcli_server_command (NULL, "priority %i", priority);

  printf ("waiting for priority on telescope");
  fflush (stdout);

  devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);

  printf ("...waiting end\n");

  pthread_create (&image_que_thread, NULL, process_images, NULL);

loop:
  devcli_server_command (NULL, "status_txt P:waiting");
  if (watch_status)
    {
      while (!ready_to_observe (devcli_ser->statutes[0].status))
	{
	  time_t t;
	  time (&t);
	  printf ("waiting for night..%s", ctime (&t));
	  fflush (stdout);
	  if ((t = devcli_command (telescope, NULL, "ready;park")))
	    fprintf (stderr, "Error while moving telescope: %i\n", (int) t);
	  else
	    printf ("PARK ok\n");
	  fflush (stdout);
	  sleep (100);
	}
    }

  for (phot = devcli_devices (); phot; phot = phot->next)
    if (phot->type == DEVICE_TYPE_PHOT)
      {
	printf ("setting handler: %s\n", phot->name);
	devcli_set_command_handler (phot,
				    (devcli_handle_response_t) phot_handler);
      }

  observe (hi_precission);
  printf ("done\n");

  devcli_command (telescope, NULL, "home");
  devcli_command (telescope, NULL, "park");

  sleep (300);

  if (watch_status)
    goto loop;

  devcli_server_disconnect ();
  db_disconnect ();

  return 0;
}
