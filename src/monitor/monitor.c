#include <unistd.h>
#include <stdio.h>
#include <mcheck.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "telescope_info.h"
#include "camera_info.h"
#include "dome_info.h"

#include "../utils/devconn.h"
#include "../utils/devcli.h"
#include "status.h"

void
status_telescope (const struct device *dev)
{
  struct telescope_info *info = (struct telescope_info *) &dev->info;
  printf ("          Ra: %+03.3f\n", info->ra);
  printf ("         Dec: %+03.3f\n", info->dec);
  printf ("  Longtitude: %+03.3f\n", info->longtitude);
  printf ("    Latitude: %+03.3f\n", info->latitude);
  printf ("Siderealtime: %02.20f\n", info->siderealtime);
  printf ("Axis count 0: %02.0f\n", info->axis0_counts);
  printf ("Axis count 1: %02.0f\n", info->axis1_counts);
}

void
status_camera (const struct device *dev)
{
  struct camera_info *info = (struct camera_info *) &dev->info;
  printf ("Type: '%s'\n", info->type);
  printf (" Set: %+03.3f\n", info->temperature_setpoint);
  printf (" Air: %+03.3f\n", info->air_temperature);
  printf (" CCD: %+03.3f\n", info->ccd_temperature);
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  char c;
  struct device *dev;
  int ret_code;
  int ret;
  int i;
  time_t t;

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

  time (&t);

  printf ("Bootes status genarated at: %s", ctime (&t));

  printf ("Connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  for (dev = devcli_devices (); dev; dev = dev->next)
    {
      printf ("-------------------------------------------------\n");
      printf (" Device name: %s hostname: %s port: %i\n", dev->name,
	      dev->hostname, dev->port);
      if ((ret = devcli_command (dev, &ret_code, "ready")))
	printf (" Device ready returns %i\n", ret);
      printf (" Device ready return code : %i\n", ret_code);
      if ((ret = devcli_command (dev, &ret_code, "base_info")))
	printf (" Device base_info returns %i\n", ret);
      printf (" Device base_info return code : %i\n", ret_code);
      if ((ret = devcli_command (dev, &ret_code, "info")))
	printf (" Device info returns %i\n", ret);
      printf (" Device info return code : %i\n", ret_code);
      switch (dev->type)
	{
	case DEVICE_TYPE_MOUNT:
	  printf ("Type: %i (MOUNT)\n", dev->type);
	  status_telescope (dev);
	  break;
	case DEVICE_TYPE_CCD:
	  printf ("Type: %i (CCD)\n", dev->type);
	  status_camera (dev);
	  break;
	default:
	  printf ("Unknow device type: %i\n", dev->type);
	}
      printf (" Number of states: %i\n", dev->status_num);
      for (i = 0; i < dev->status_num; i++)
	{
	  struct devconn_status *st = &dev->statutes[i];
	  if (*st->name)
	    printf (" Status: %s value: %i\n", st->name, st->status);
	}
      printf (" Device %s status ends.\n", dev->name);
    }
  time (&t);
  printf (" Status report ends at %s", ctime (&t));
  return 0;
}
