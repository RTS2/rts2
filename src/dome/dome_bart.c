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
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "status.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_NAME		"DCM"
#define DEVICE_PORT		5555	// default dome TCP/IP port

int dome_port = -1;
char *dome_file = "/dev/ttyS0";
int dome_sleep = 13;		// how long to sleep between command to start motor and command to stop it

/*! 
 * Handle dome commands.
 *
 * @param command	command to perform
 * 
 * @return -2 on exit, -1 and set errno on HW failure, 0 when command
 * was successfully performed
 */

void *
dome_open (void *arg)
{
  write (dome_port, "\112\53", 2);
  sleep (dome_sleep);
  write (dome_port, "\113", 1);
  devdem_status_mask (0, DOME_DOME_MASK, DOME_OPENED, "dome opened");
  return NULL;
}

void *
dome_close (void *arg)
{
  write (dome_port, "\52\53", 2);
  sleep (dome_sleep);
  write (dome_port, "\113\112", 2);
  devdem_status_mask (0, DOME_DOME_MASK, DOME_CLOSED, "dome closed");
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
      struct dome_info info;
      info.temperature = nan ("a");
      info.humidity = nan ("a");

      devser_dprintf ("temperature %f", info.temperature);
      devser_dprintf ("humidity %f", info.humidity);
      ret = errno = 0;
    }
  else if (strcmp (command, "open") == 0)
    {
      if (devdem_priority_block_start ())
	return -1;
      devdem_status_mask (0, DOME_DOME_MASK, DOME_OPENING, "opening dome");
      if ((ret = devser_thread_create (dome_open, NULL, 0, NULL, NULL)) < 0)
	{
	  devdem_status_mask (0, DOME_DOME_MASK, DOME_OPENED,
			      "not opening - cannot create thread");
	}
      devdem_priority_block_end ();
      return ret;
    }
  else if (strcmp (command, "close") == 0)
    {
      if (devdem_priority_block_start ())
	return -1;
      devdem_status_mask (0, DOME_DOME_MASK, DOME_CLOSING, "closing dome");
      if ((ret = devser_thread_create (dome_open, NULL, 0, NULL, NULL)) < 0)
	{
	  devdem_status_mask (0, DOME_DOME_MASK, DOME_CLOSED,
			      "not closing - cannot create thread");
	}
      devdem_priority_block_end ();
      return ret;
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
main (int argc, char **argv)
{
  char *stats[] = { "weather", "dome1" };

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

  if (devdem_init (stats, 2, NULL))
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

  dome_port = open (dome_file, O_RDWR);
  if (dome_port == -1)
    {
      perror ("dome_port_open");
      devdem_done ();
      return EXIT_FAILURE;
    }

  if (devdem_run (device_port, dome_handle_command) == -2)
    {
//      telescope_done ();
      close (dome_port);
    }
  return EXIT_SUCCESS;
}
