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

#include <argz.h>

#include "lx200.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"
#include "../status.h"

#define PORT    5555

// macro for length test
#define test_length(npars) if (argz_count (argv, argc) != npars + 1) { \
        devdem_write_command_end ("Unknow nmbr of params: expected %i,got %i",\
		DEVDEM_E_PARAMSNUM, npars, argz_count (argv, argc) ); \
	return -1; \
}

// macro for telescope calls, will write error
#define tel_call(call)   if ((ret = call) < 0) \
{\
	 devdem_write_command_end ("Telescope error: %s", DEVDEM_E_HW, strerror(errno));\
	 return ret;\
}


/*! Handle teld command.
*
* @param buffer buffer, containing unparsed command
* @return -2 on exit, -1 and set errno on HW failure, 0 when command
* was successfully performed
*/
int
teld_handle_command (char *argv, size_t argc)
{
  int ret;
  double dval;
  char *param;

  if (strcmp (argv, "ready") == 0)
    {
      tel_call (tel_is_ready ());
    }
  else if (strcmp (argv, "set") == 0)
    {
      double ra, dec;
      test_length (2);
      param = argz_next (argv, argc, argv);
      ra = hmstod (param);
      param = argz_next (argv, argc, param);
      dec = hmstod (param);
      if (isnan (ra) || isnan (dec))
	{
	  devdem_write_command_end ("Expected ra dec, got: %f %f",
				    DEVDEM_E_PARAMSVAL, ra, dec);
	  ret = -1;
	}
      else
	tel_call (tel_set_to (ra, dec));
    }
  else if (strcmp (argv, "move") == 0)
    {
      double ra, dec;
      test_length (2);
      param = argz_next (argv, argc, argv);
      ra = hmstod (param);
      param = argz_next (argv, argc, param);
      dec = hmstod (param);
      if (isnan (ra) || isnan (dec))
	{
	  devdem_write_command_end ("Expected ra dec, got: %f %f",
				    DEVDEM_E_PARAMSVAL, ra, dec);
	  ret = -1;
	}
      else
	tel_call (tel_move_to (ra, dec));
    }
  else if (strcmp (argv, "ra") == 0)
    {
      tel_call (tel_read_ra (&dval));
      devdem_dprintf ("ra %f\n", dval);
    }
  else if (strcmp (argv, "dec") == 0)
    {
      tel_call (tel_read_dec (&dval));
      devdem_dprintf ("dec %f\n", dval);
    }
  else if (strcmp (argv, "park") == 0)
    {
      tel_call (tel_park ());
    }
// extended functions
  else if (strcmp (argv, "lon") == 0)
    {
      tel_call (tel_read_longtitude (&dval));
      devdem_dprintf ("dec %f\n", dval);
    }
  else if (strcmp (argv, "lat") == 0)
    {
      tel_call (tel_read_latitude (&dval));
      devdem_dprintf ("dec %f\n", dval);
    }
  else if (strcmp (argv, "lst") == 0)
    {
      tel_call (tel_read_siderealtime (&dval));
      devdem_dprintf ("dec %f\n", dval);
    }
  else if (strcmp (argv, "loct") == 0)
    {
      tel_call (tel_read_localtime (&dval));
      devdem_dprintf ("dec %f\n", dval);
    }
  else if (strcmp (argv, "exit") == 0)
    {
      return -2;
    }
  else if (strcmp (argv, "help") == 0)
    {
      devdem_dprintf ("ready - is telescope ready to observe?\n");
      devdem_dprintf ("set <ra> <dec> - set telescope coordinates\n");
      devdem_dprintf ("move <ra> <dec> - move telescope\n");
      devdem_dprintf ("ra - telescope right ascenation\n");
      devdem_dprintf ("dec - telescope declination\n");
      devdem_dprintf ("park - park telescope\n");
      devdem_dprintf ("lon - telescope longtitude\n");
      devdem_dprintf ("lat - telescope latitude\n");
      devdem_dprintf ("lst - telescope local sidereal time\n");
      devdem_dprintf ("loct - telescope local time\n");
      devdem_dprintf ("exit - exit from main loop\n");
      devdem_dprintf ("help - print, what you are reading just now\n");
      ret = errno = 0;
    }
  else
    {
      devdem_write_command_end ("Unknow command: '%s'", DEVDEM_E_COMMAND,
				argv);
      return -1;
    }
  return ret;
}

void
exit_handler (int err, void *args)
{
  if (getpid () == devdem_parent_pid)
    tel_cleanup (err, args);
  syslog (LOG_INFO, "Exit");
}

int
main (void)
{
#ifdef DEBUG
  mtrace ();
#endif
  // open syslog
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  on_exit (exit_handler, NULL);

  if (tel_connect ("/dev/ttyS0") < 0)
    {
      syslog (LOG_ERR, "tel_connect: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  return devdem_run (PORT, teld_handle_command);
}
