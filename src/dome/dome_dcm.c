/*! 
 * @file Telescope deamon.
 * $Id$
 * @author petr
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <getopt.h>
#include <time.h>

#include "dome_info.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "status.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_NAME		"DCM"
#define DEVICE_PORT		5555	// default camera TCP/IP port

#define DOME_FILE		"/var/log/dome"

/*! 
 * Handle teld command.
 *
 * @param command	command to perform
 * 
 * @return -2 on exit, -1 and set errno on HW failure, 0 when command
 * was successfully performed
 */
int
dome_handle_command (char *command)
{
  int ret;

  if (strcmp (command, "ready") == 0)
    {
      struct stat fs;
      time_t now = time (NULL);
      if (stat ("/var/log/dome", &fs) && fs.st_ctime < now - 2000)
	{
	  devser_write_command_end (DEVDEM_E_HW, "Dome not ready");
	  return -1;
	}


      ret = errno = 0;
    }

  else if (strcmp (command, "info") == 0)
    {
      struct dome_info info;
      FILE *fs = fopen ("/var/log/dome", "r");
      char state[50], weather[50];

      if (!stat)
	{
	  devser_write_command_end (DEVDEM_E_HW, "File error %i %s", errno,
				    strerror (errno));
	  return -1;
	}
      fscanf (fs, "%s %f %f %s", state, &info.temperature, &info.humidity,
	      weather);
      fclose (fs);
      if (!strcasecmp (state, "OPEN"))
	info.state = DOME_OPENED;
      else if (!strcasecmp (state, "CLOSE"))
	info.state = DOME_CLOSED;
      else if (!strcasecmp (state, "CLOSING"))
	info.state = DOME_CLOSING;
      else if (!strcasecmp (state, "OPENINIG"))
	info.state = DOME_OPENIGN;
      else
	info.state = 0;
      devser_dprintf ("temperature %f", info.temperature);
      devser_dprintf ("humidity %f", info.humidity);
      ret = errno = 0;
    }
  else if (strcmp (command, "help") == 0)
    {
      devser_dprintf ("ready - is dome ready");
      devser_dprintf ("info - dome informations");
      devser_dprintf ("exit - exit from main loop");
      devser_dprintf ("help - print, what you are reading just now");
      ret = errno = 0;
    }
  else
    {
      devser_write_command_end (DEVDEM_E_COMMAND, "Unknow command: '%s'",
				command);
      return -1;
    }
  return ret;
}

int
main (int argc, char **argv)
{
  char *stats[] = { "dome" };

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  char *device_name = DEVICE_NAME;
  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;

  // int dcm_socket;

#ifdef DEBUG
  mtrace ();
#endif

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"tel_port", 1, 0, 'l'},
	{"port", 1, 0, 'p'},
	{"serverd_host", 1, 0, 's'},
	{"serverd_port", 1, 0, 'q'},
	{"device_name", 1, 0, 'd'},
	{"help", 0, 0, 0},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "l:p:s:q:d:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  device_port = atoi (optarg);
	  if (device_port < 1 || device_port == UINT_MAX)
	    {
	      printf ("invalid device port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 's':
	  serverd_host = optarg;
	  break;
	case 'q':
	  serverd_port = atoi (optarg);
	  if (serverd_port < 1 || serverd_port == UINT_MAX)
	    {
	      printf ("invalid serverd port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'd':
	  device_name = optarg;
	  break;
	case 0:
	  printf
	    ("Options:\n\tserverd_port|p <port_num>\t\tport of the serverd");
	  exit (EXIT_SUCCESS);
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }

  if (optind != argc - 1)
    {
      printf ("hostname wasn't specified\n");
      exit (EXIT_FAILURE);
    }

  hostname = argv[argc - 1];

  // open syslog
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  if (devdem_init (stats, 1, NULL))
    {
      syslog (LOG_ERR, "devdem_init: %m");
      return EXIT_FAILURE;
    }

  if (devdem_register
      (serverd_host, serverd_port, device_name, DEVICE_TYPE_DOME, hostname,
       device_port) < 0)
    {
      perror ("devser_register");
      devdem_done ();
      return EXIT_FAILURE;
    }

  if (devdem_run (device_port, dome_handle_command) == -2)
    {
//      telescope_done ();
    }
  return EXIT_SUCCESS;
}
