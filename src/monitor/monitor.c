#include <unistd.h>
#include <stdio.h>
#include <mcheck.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>

#include "telescope_info.h"
#include "camera_info.h"

#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../status.h"

void
status_telescope (const struct telescope_info *info)
{
  if (info)
    {
      printf (" Ra: %+03.3f\n", info->ra);
      printf ("Dec: %+03.3f\n", info->dec);
      printf ("Lon: %+03.3f\n", info->longtitude);
      printf ("Lat: %+03.3f\n", info->latitude);
      printf ("Stm: %02.20f\n", info->siderealtime);
    }
  else
    {
      printf ("NOT READY\n");
    }
}

void
status_camera (const struct camera_info *info)
{
  if (info)
    {
      printf ("name: '%s'\n", info->name);
      printf ("Set: %+03.3f\n", info->temperature_setpoint);
      printf ("Air: %+03.3f\n", info->air_temperature);
      printf ("CCD: %+03.3f\n", info->ccd_temperature);
    }
  else
    {
      printf ("NOT READY\n");
    }
}

int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  char c;
  int camd_id, teld_id;
  int ret_code;
  union devhnd_info *info;

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

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      exit (EXIT_FAILURE);
    }

  if (devcli_connectdev (&camd_id, "camd", NULL) < 0)
    {
      perror ("devcli_connectdev");
      exit (EXIT_FAILURE);
    }

  printf ("camd_id: %i\n", camd_id);

  if (devcli_connectdev (&teld_id, "teld", NULL) < 0)
    {
      perror ("devcli_connectdev");
      exit (EXIT_FAILURE);
    }

  printf ("teld_id: %i\n---------------\n", teld_id);

  while (1)
    {
      if (devcli_command (teld_id, &ret_code, "ready"))
	{
	  status_telescope (NULL);
	}
      else
	{
	  devcli_command (teld_id, &ret_code, "info");
	  devcli_getinfo (teld_id, &info);
	  status_telescope ((struct telescope_info *) info);
	}

      if (devcli_command (camd_id, &ret_code, "ready"))
	{
	  status_camera (NULL);
	}
      else
	{
	  devcli_command (camd_id, &ret_code, "info");
	  devcli_getinfo (camd_id, &info);
	  status_camera (info);
	}

      for (c = 10; c; c--)
	{
	  printf ("next update in %02i seconds\n", c);
	  sleep (1);
	}
    }
}
