#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <mcheck.h>
#include <math.h>
#include <stdarg.h>

#include <argz.h>

#include "lx200.h"
#include "../utils/hms.h"
#include "../utils/devdem.h"

#define PORT    5555
#define MAXMSG  512

// macro for length test
#define test_length(npars) if (argz_count (argv, argc) != npars + 1) { \
        ret = -1; \
	errno = EINVAL; \
	goto end; \
}

/*! Handle teld command.
*
* @param buffer buffer, containing unparsed command
* @param fd descriptor of input/output socket
* @return -2 on exit, -1 and set errno on HW failure, 0 otherwise
*/
int
teld_handle_command (char *buffer, int fd)
{
  char *argv;
  size_t argc;
  int ret;
  double dval;

  if ((ret = argz_create_sep (buffer, ' ', &argv, &argc)))
    {
      errno = ret;
      return -1;
    }
  if (!argv)
    return 0;
  if (strcmp (argv, "ready") == 0)
    {
      if ((ret = tel_is_ready ()) >= 0)
	devdem_dprintf (fd, "ready 1");
      else
	devdem_dprintf (fd, "ready 0");
      goto end;
    }
  if (strcmp (argv, "set") == 0)
    {
      double ra, dec;
      char *param;
      test_length (2);
      param = argz_next (argv, argc, argv);
      ra = hmstod (param);
      param = argz_next (argv, argc, param);
      dec = hmstod (param);
      if (isnan (ra) || isnan (dec))
	{
	  errno = EINVAL;
	  ret = -1;
	}
      else
	ret = tel_set_to (ra, dec);
      goto end;
    }
  if (strcmp (argv, "move") == 0)
    {
      double ra, dec;
      char *param;
      test_length (2);
      param = argz_next (argv, argc, argv);
      ra = hmstod (param);
      param = argz_next (argv, argc, param);
      dec = hmstod (param);
      if (isnan (ra) || isnan (dec))
	{
	  errno = EINVAL;
	  ret = -1;
	}
      else
	ret = tel_move_to (ra, dec);
      goto end;
    }
  if (strcmp (argv, "ra") == 0)
    {
      if ((ret = tel_read_ra (&dval)) >= 0)
	devdem_dprintf (fd, "ra %f\n", dval);
      goto end;
    }
  if (strcmp (argv, "dec") == 0)
    {
      if ((ret = tel_read_dec (&dval)) >= 0)
	devdem_dprintf (fd, "dec %f\n", dval);
      goto end;
    }
  if (strcmp (argv, "park") == 0)
    {
      ret = tel_park ();
      goto end;
    }
// extended functions
  if (strcmp (argv, "lon") == 0)
    {
      if ((ret = tel_read_longtitude (&dval)) >= 0)
	devdem_dprintf (fd, "dec %f\n", dval);
      goto end;
    }
  if (strcmp (argv, "lat") == 0)
    {
      if ((ret = tel_read_latitude (&dval)) >= 0)
	devdem_dprintf (fd, "dec %f\n", dval);
      goto end;
    }
  if (strcmp (argv, "lst") == 0)
    {
      if ((ret = tel_read_siderealtime (&dval)) >= 0)
	devdem_dprintf (fd, "dec %f\n", dval);
      goto end;
    }
  if (strcmp (argv, "loct") == 0)
    {
      if ((ret = tel_read_localtime (&dval)) >= 0)
	devdem_dprintf (fd, "dec %f\n", dval);
      goto end;
    }
  if (strcmp (argv, "exit") == 0)
    {
      close (fd);
      ret = -2;
      goto end;
    }
  if (strcmp (argv, "help") == 0)
    {
      devdem_dprintf (fd, "ready - is telescope ready to observe?\n");
      devdem_dprintf (fd, "set - set telescope coordinates\n");
      devdem_dprintf (fd, "move - move telescope\n");
      devdem_dprintf (fd, "ra - telescope right ascenation\n");
      devdem_dprintf (fd, "dec - telescope declination\n");
      devdem_dprintf (fd, "park - park telescope\n");
      devdem_dprintf (fd, "lon - telescope longtitude\n");
      devdem_dprintf (fd, "lat - telescope latitude\n");
      devdem_dprintf (fd, "lst - telescope local sidereal time\n");
      devdem_dprintf (fd, "loct - telescope local time\n");
      devdem_dprintf (fd, "exit - exit from main loop\n");
      devdem_dprintf (fd, "help - print, what you are reading just now\n");
      ret = errno = 0;
      goto end;
    }
  ret = -1;
  errno = EINVAL;
end:
  free (argv);
  return ret;
}

int
main (void)
{
#ifdef DEBUG
  mtrace ();
#endif
  // open syslog
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  if (tel_connect ("/dev/ttyS0") < 0)
    {
      syslog (LOG_ERR, "tel_connect: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }
  return devdem_run (PORT, teld_handle_command);
}
