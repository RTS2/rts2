/*! 
 * @file Photometer deamon. 
 * $Id$
 * @author petr
 */

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <getopt.h>
#include <time.h>

#include "phot_info.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "kernel/phot.h"
#include "status.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_NAME		"PHOT"
#define DEVICE_PORT		5559	// default camera TCP/IP port


int fd = 0;

char *phot_dev = "/dev/phot0";

void
phot_command (char command, int arg)
{
  char cmd_buf[3];
  cmd_buf[0] = command;
  *((int *) (&(cmd_buf[1]))) = arg;
  write (fd, cmd_buf, 3);
}

void
phot_init ()
{
  if (fd)
    return;
  fd = open (phot_dev, O_RDWR);
  if (fd == -1)
    {
      perror ("opening photometr\n");
      fd = 0;
    }
  // reset occurs on photometer start, so there is no need to wait for
  // reset
}

void
phot_home_filter ()
{
  devser_thread_cancel_all ();
  phot_command (PHOT_CMD_RESET, 0);
}

void
clean_integrate_cancel (void *agr)
{
  phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
  devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_INTEGRATE,
		      "Integration finished");
}

void *
start_integrate (void *arg)
{
  int it_t;
  int result;
  int ret;
  it_t = *((float *) arg);

  phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
  phot_command (PHOT_CMD_INTEGRATE, it_t);
  while ((ret = read (fd, &result, sizeof (int))) != -1)
    {
      if (ret && result == 'A')
	{
	  result = 0;
	  read (fd, &result, 2);
	}
      devser_dprintf ("count %i %i", result >> 16, ret);
    }
  return NULL;
}

/*! 
 * Handle photometer command.
 *
 * @param command	command to perform
 * 
 * @return -2 on exit, -1 and set errno on HW failure, 0 when command
 * was successfully performed
 */
int
phot_handle_command (char *command)
{
  int ret;

  if (strcmp (command, "ready") == 0)
    {
      phot_init ();
      ret = errno = 0;
    }

  else if (strcmp (command, "info") == 0)
    {
      phot_init ();
      ret = errno = 0;
    }
  else if (strcmp (command, "home") == 0)
    {
      phot_init ();
      phot_home_filter ();
      ret = errno = 0;
    }
  else if (strcmp (command, "integrate") == 0)
    {
      float it;			// integration time in seconds
      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_float (&it))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      devser_thread_cancel_all ();

      devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_INTEGRATE,
			  "Integration started");
      if ((ret =
	   devser_thread_create (start_integrate, (void *) &it, sizeof (it),
				 NULL, clean_integrate_cancel)) == -1)
	{
	  devdem_status_mask (0, PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
			      "thread create error");
	}
      devdem_priority_block_end ();
      return ret;
    }

  else if (strcmp (command, "help") == 0)
    {
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
  char *stats[] = { "phot" };

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  char *device_name = DEVICE_NAME;
  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;

#ifdef DEBUG
  mtrace ();
#endif

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{"phot_port", 1, 0, 'l'},
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
	case 'l':
	  device_port = atoi (optarg);
	  break;
	case 'p':
	  phot_dev = optarg;
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
	  printf ("Options:\n");
	  printf ("\tphot_port|l <port_num>\t\tdevice TCP/IP port\n");
	  printf ("\tport|p <port_num>\t\tPhotometer IO port base\n");
	  printf ("\tserverd_host|s <port_num>\t\thostname of the serverd\n");
	  printf ("\tserverd_port|q <port_num>\t\tport of the serverd\n");
	  printf ("\tdevice_name|d <device_name>\t\tdevice name\n");
	  printf ("\thelp\t\tprint this help and exits\n");
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
      (serverd_host, serverd_port, device_name, DEVICE_TYPE_PHOT, hostname,
       device_port) < 0)
    {
      perror ("devser_register");
      devdem_done ();
      return EXIT_FAILURE;
    }

  devdem_run (device_port, phot_handle_command);

  return EXIT_SUCCESS;
}
