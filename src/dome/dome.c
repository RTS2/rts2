/*! 
 * @file Dome control deamon. 
 * $Id$
 * @author petr
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <getopt.h>
#include <time.h>

#include "dome_info.h"
#include "../utils/config.h"
#include "../utils/devdem.h"
#include "../utils/hms.h"
#include "status.h"
#include "dome.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_NAME		"DCM"
#define DEVICE_PORT		5553	// default dome TCP/IP port

char *dome_file = "/dev/ttyS0";

void *
observing (void *arg)
{
  if (dome_observing ())
    {
      devdem_status_mask (0, DOME_DOME_MASK, DOME_UNKNOW,
			  "dome not switched to observing - error");
    }
  else
    {
      devdem_status_mask (0, DOME_DOME_MASK, DOME_OBSERVING,
			  "dome switched to observing");
    }
  return NULL;
}

void *
standby (void *arg)
{
  if (dome_standby ())
    {
      devdem_status_mask (0, DOME_DOME_MASK, DOME_UNKNOW,
			  "dome not switched to standby - error");
    }
  else
    {
      devdem_status_mask (0, DOME_DOME_MASK, DOME_STANDBY,
			  "dome switched to standby");
    }
  return NULL;
}

void *
off (void *arg)
{
  if (dome_off ())
    {
      devdem_status_mask (0, DOME_DOME_MASK, DOME_UNKNOW,
			  "dome not switched to off - error");
    }
  else
    {
      devdem_status_mask (0, DOME_DOME_MASK, DOME_OFF,
			  "dome switched to off");
    }
  return NULL;
}

int
dome_handle_command (char *command)
{
  int ret;

  if (strcmp (command, "ready") == 0)
    {
      ret = errno = 0;
    }

  else if (strcmp (command, "info") == 0)
    {
      struct dome_info *info;
      dome_info (&info);

      devser_dprintf ("temperature %f", info->temperature);
      devser_dprintf ("humidity %f", info->humidity);
      devser_dprintf ("dome %i", info->dome);
      devser_dprintf ("power_telescope %i", info->power_telescope);
      devser_dprintf ("power_cameras %i", info->power_cameras);
      ret = errno = 0;
    }
  else if (strcmp (command, "observing") == 0)
    {
//      if (devdem_priority_block_start ())
//      return -1;
      devdem_status_mask (0, DOME_DOME_MASK, DOME_OBSERVING,
			  "switching dome to observing");
      if ((ret = devser_thread_create (observing, NULL, 0, NULL, NULL)) < 0)
	{
	  devdem_status_mask (0, DOME_DOME_MASK, DOME_UNKNOW,
			      "not observing - cannot create thread");
	}
//      devdem_priority_block_end ();
//      return ret;
    }
  else if (strcmp (command, "standby") == 0)
    {
//      if (devdem_priority_block_start ())
//      return -1;
      devdem_status_mask (0, DOME_DOME_MASK, DOME_STANDBY,
			  "switching dome to standby");
      if ((ret = devser_thread_create (standby, NULL, 0, NULL, NULL)) < 0)
	{
	  devdem_status_mask (0, DOME_DOME_MASK, DOME_UNKNOW,
			      "not standby - cannot create thread");
	}
//      devdem_priority_block_end ();
//      return ret;
    }
  else if (strcmp (command, "off") == 0)
    {
//      if (devdem_priority_block_start ())
//      return -1;
      devdem_status_mask (0, DOME_DOME_MASK, DOME_OFF,
			  "switching dome to off");
      if ((ret = devser_thread_create (off, NULL, 0, NULL, NULL)) < 0)
	{
	  devdem_status_mask (0, DOME_DOME_MASK, DOME_UNKNOW,
			      "not off - cannot create thread");
	}
//      devdem_priority_block_end ();
//      return ret;
    }

  else if (strcmp (command, "help") == 0)
    {
      devser_dprintf ("open - open dome");
      devser_dprintf ("close - close dome");
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
dome_handle_status (int status, int old_status)
{
  int ret = 0;
  switch (status & SERVERD_STATUS_MASK)
    {
    case SERVERD_EVENING:
    case SERVERD_NIGHT:
    case SERVERD_DUSK:
    case SERVERD_DAWN:
      if (status < 10)
	ret = dome_observing ();
      else
	ret = dome_standby ();
      break;
    default:			/* SERVERD_DAY, SERVERD_DUSK, SERVERD_MAINTANCE, SERVERD_OFF */
      ret = dome_off ();
    }
  return ret;
}

int
main (int argc, char **argv)
{
  char *stats[] = { "weather", "dome" };

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  char *device_name = DEVICE_NAME;
  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;
  int deamonize = 1;

#ifdef DEBUG
  mtrace ();
#endif

  if (read_config (CONFIG_FILE) == -1)
    {
      fprintf (stderr,
	       "Cannot read configuration file " CONFIG_FILE ", exiting.");
      exit (1);
    }

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"tel_port", 1, 0, 'l'},
	{"port", 1, 0, 'p'},
	{"interactive", 0, 0, 'i'},
	{"serverd_host", 1, 0, 's'},
	{"serverd_port", 1, 0, 'q'},
	{"device_name", 1, 0, 'd'},
	{"help", 0, 0, 0},
	{0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "l:p:is:q:d:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'p':
	  device_port = atoi (optarg);
	  break;
	case 's':
	  serverd_host = optarg;
	  break;
	case 'q':
	  serverd_port = atoi (optarg);
	  break;
	case 'd':
	  device_name = optarg;
	  break;
	case 'i':
	  deamonize = 0;
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

  dome_file = get_device_string_default ("dome", "port", dome_file);

  if (dome_init (dome_file) == -1)
    {
      perror ("dome_port_open");
      devdem_done ();
      return EXIT_FAILURE;
    }


  if (devdem_init (stats, 2, dome_handle_status, deamonize))
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
      dome_done ();
    }
  return EXIT_SUCCESS;
}
