/*! 
 * @file Telescope deamon.
 * $Id$
 * @author petr
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <mcheck.h>
#include <math.h>
#include <stdarg.h>
#include <pthread.h>
#include <getopt.h>

#include "telescope.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "status.h"

#define SERVERD_PORT    	5557	// default serverd port
#define SERVERD_HOST		"localhost"	// default serverd hostname

#define DEVICE_PORT		5555	// default camera TCP/IP port
#define DEVICE_NAME 		"T1"	// default camera name

#define TEL_PORT		"/dev/ttyS0"	// default telescope port

#define CORRECTION_BUF		10

// macro for telescope calls, will write error
#define tel_call(call)   if ((ret = call) < 0) \
{\
	 devser_write_command_end (DEVDEM_E_HW, "Telescope error: %s", strerror(errno));\
	 return ret;\
}

struct radec
{
  double ra;
  double dec;
};

struct radec correction_buf[CORRECTION_BUF];
int correction_mark = 0;

#define RADEC 	((struct radec *) arg)
void
client_move_cancel (void *arg)
{
  telescope_stop ();
  devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING, "move canceled");
}

void *
start_move (void *arg)
{
  int ret;
  if ((ret = telescope_move_to (RADEC->ra, RADEC->dec)) < 0)
    {
      devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING, "with error");
    }
  else
    {
      devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING, "move finished");
    }
  return NULL;
}

void *
start_correction (void *arg)
{
  telescope_correct (RADEC->ra, RADEC->dec);
  return NULL;
}

void *
start_park (void *arg)
{
  int ret;
  if ((ret = telescope_park ()) < 0)
    {
      devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING, "with error");
    }
  else
    {
      devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING, "parked");
    }
  return NULL;
}

#undef RADEC

/*! 
 * Handle teld command.
 *
 * @param command	command to perform
 * 
 * @return -2 on exit, -1 and set errno on HW failure, 0 when command
 * was successfully performed
 */
int
teld_handle_command (char *command)
{
  int ret;

  if (strcmp (command, "ready") == 0)
    {
      tel_call (telescope_init ("/dev/ttyS0", 0));
    }
  else if (strcmp (command, "set") == 0)
    {
      double ra, dec;
      if (devser_param_test_length (2))
	return -1;
      if (devser_param_next_hmsdec (&ra))
	return -1;
      if (devser_param_next_hmsdec (&dec))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      tel_call (telescope_set_to (ra, dec));
      devdem_priority_block_end ();
    }
  else if (strcmp (command, "move") == 0)
    {
      struct radec coord;
      if (devser_param_test_length (2))
	return -1;
      if (devser_param_next_hmsdec (&(coord.ra)))
	return -1;
      if (devser_param_next_hmsdec (&(coord.dec)))
	return -1;
      if (devdem_priority_block_start ())
	return -1;
      devdem_status_mask (0, TEL_MASK_MOVING, TEL_MOVING,
			  "moving of telescope started");
      if ((ret =
	   devser_thread_create (start_move, (void *) &coord, sizeof coord,
				 NULL, client_move_cancel)) < 0)
	{
	  devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING,
			      "cannot create thread");
	}
      devdem_priority_block_end ();
      return ret;
    }
  else if (strcmp (command, "correct") == 0)
    {
      struct radec coord;
      int correction_number;
      int i;
      if (devser_param_test_length (3))
	return -1;
      if (devser_param_next_integer (&correction_number))
	return -1;
      if (devser_param_next_hmsdec (&(coord.ra)))
	return -1;
      if (devser_param_next_hmsdec (&(coord.dec)))
	return -1;
      if (abs (coord.ra) > 5 || abs (coord.dec) > 5 || correction_number < 0
	  || correction_number > correction_mark)
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL,
				    "corections bigger than sane limit");
	  return -1;
	}
//      if (correction_mark < 2 && correction_mark - CORRECTION_BUF > correction_number)
      if (correction_mark - CORRECTION_BUF > correction_number)
	{
	  devser_write_command_end (DEVDEM_E_PARAMSVAL, "old corection");
	  return -1;
	}
      if (devdem_priority_block_start ())
	return -1;

      coord.ra += correction_buf[correction_number % CORRECTION_BUF].ra;
      coord.dec += correction_buf[correction_number % CORRECTION_BUF].dec;
      syslog (LOG_INFO, "correction: ra %f dec %f mark %i\n", coord.ra,
	      coord.dec, correction_mark);
      // update all corrections..
      for (i = 0; i < CORRECTION_BUF; i++)
	{
	  correction_buf[i].ra -= coord.ra;
	  correction_buf[i].dec -= coord.dec;
	}

      correction_mark++;
      correction_buf[correction_mark % CORRECTION_BUF].ra = 0;
      correction_buf[correction_mark % CORRECTION_BUF].dec = 0;

      ret =
	devser_thread_create (start_correction, (void *) &coord, sizeof coord,
			      NULL, NULL);
      devdem_priority_block_end ();
      return ret;
    }
  else if (strcmp (command, "base_info") == 0)
    {
      struct telescope_info info;
      tel_call (telescope_base_info (&info));
      devser_dprintf
	("type %s\nserial_number %s\nlongtitude %f\nlatitude %f\naltitude %f\npark_dec %f",
	 info.type, info.serial_number, info.longtitude, info.latitude,
	 info.altitude, info.park_dec);
    }
  else if (strcmp (command, "info") == 0)
    {
      struct telescope_info info;
      tel_call (telescope_info (&info));
      // correction mark is local variable, so we must use the local
      // variant - not one from info!
      devser_dprintf
	("ra %f\ndec %f\nsiderealtime %f\nlocaltime %f\ncorrection_mark %i\nflip %i\naxis0_counts %f\naxis1_counts %f",
	 info.ra, info.dec, info.siderealtime, info.localtime,
	 correction_mark, info.flip, info.axis0_counts, info.axis1_counts);
    }
  else if (strcmp (command, "park") == 0)
    {
      if (devdem_priority_block_start ())
	return -1;
      devdem_status_mask (0, TEL_MASK_MOVING, TEL_MOVING,
			  "parking telescope started");
      if ((ret =
	   devser_thread_create (start_park, NULL, 0, NULL,
				 client_move_cancel)) < 0)
	{
	  devdem_status_mask (0, TEL_MASK_MOVING, TEL_OBSERVING,
			      "not parking - cannot create thread");
	}
      devdem_priority_block_end ();
      return ret;
    }
  else if (strcmp (command, "help") == 0)
    {
      devser_dprintf ("ready - is telescope ready to observe?");
      devser_dprintf ("set - set telescope coordinates");
      devser_dprintf ("move - move telescope");
      devser_dprintf ("info - telescope informations");
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
  char *stats[] = { "telescope" };

  char *tel_port = TEL_PORT;

  char *serverd_host = SERVERD_HOST;
  uint16_t serverd_port = SERVERD_PORT;

  char *device_name = DEVICE_NAME;
  uint16_t device_port = DEVICE_PORT;

  char *hostname = NULL;
  int c;

#ifdef DEBUG
  mtrace ();
#endif

  // init correction buffer
  for (c = 0; c < CORRECTION_BUF; c++)
    {
      correction_buf[c].ra = 0;
      correction_buf[c].dec = 0;
    }

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
	case 'l':
	  tel_port = optarg;
	  break;
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

  telescope_init ("/dev/ttyS0", 0);

  if (devdem_register
      (serverd_host, serverd_port, device_name, DEVICE_TYPE_MOUNT, hostname,
       device_port) < 0)
    {
      perror ("devser_register");
      devdem_done ();
      return EXIT_FAILURE;
    }

  if (devdem_run (device_port, teld_handle_command) == -2)
    {
      telescope_done ();
    }
  return EXIT_SUCCESS;
}
