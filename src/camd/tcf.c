#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "tcf.h"

#define BAUDRATE B19200

#define FOCUSER_PORT	"/dev/ttyS5"

struct termios oldtio, newtio;

static char *focuser_port = NULL;

void
set_focuser_port (char *in_focuser_port)
{
  focuser_port = in_focuser_port;
}

int
serial_init ()
{
  int fd;

  if (!focuser_port)
    return -1;

  fd = open (focuser_port, O_RDWR | O_NOCTTY);

  if (fd < 0)
    {
      perror (focuser_port);
      return -1;
    }

  tcgetattr (fd, &oldtio);	/* save current port settings */
  bzero (&newtio, sizeof (newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0;	/* inter-character timer unused */
  newtio.c_cc[VMIN] = 2;	/* blocking read until 5 chars received */
  tcflush (fd, TCIFLUSH);
  tcsetattr (fd, TCSANOW, &newtio);

  return fd;
}

void
serial_deinit (int fd)
{
  tcsetattr (fd, TCSANOW, &oldtio);
}


int
input_timeout (int descriptor, unsigned int seconds)
{
  fd_set set;
  struct timeval timeout;

  /* Initialize the file descriptor set. */
  FD_ZERO (&set);
  FD_SET (descriptor, &set);

  /* Initialize the timeout data structure. */
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;

  /* select returns 0 if timeout, 1 if input available, -1 if error. */
  return TEMP_FAILURE_RETRY (select (FD_SETSIZE, &set, NULL, NULL, &timeout));
}

int
tcf_set_manual ()
{
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };
  int to_return;

  fd = serial_init ();

  if (fd < 0)
    return -1;

  res = write (fd, "FMMODE", 6);

  res = input_timeout (fd, 3);

  if (res == 0)
    {
      // timeout is over
      to_return = 0;
    }
  else if (res == 1)
    {
      res = read (fd, ret, 1);

      serial_deinit (fd);

      if ((res > 0) && (ret[0] == '!'))
	to_return = 1;
      else
	to_return = -1;
    }
  else
    to_return = -1;

  serial_deinit (fd);

  return to_return;
}

int
tcf_step_out (int num, int direction)
{
  char command[6];
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };
  char add = ' ';

  fd = serial_init ();

  if (fd < 0)
    return -1;

  if (direction == -1)
    add = 'I';
  else if (direction == 1)
    add = 'O';

  if (num > 7000)
    return -1;
  else if ((num > 999) && (num < 7001))
    sprintf (command, "F%c%d", add, num);
  else if ((num > 99) && (num < 1000))
    sprintf (command, "F%c0%d", add, num);
  else if ((num > 9) && (num < 100))
    sprintf (command, "F%c00%d", add, num);
  else if ((num > -1) && (num < 10))
    sprintf (command, "F%c000%d", add, num);

  res = write (fd, command, 6);

  res = read (fd, ret, 2);

  serial_deinit (fd);

  if ((res > 0) && (ret[0] == '*'))
    {
      return 0;
    }
  else
    return -1;
}

int
tcf_get_pos (int *position)
{
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };
  char out[4];

  fd = serial_init ();

  if (fd < 0)
    return -1;

  res = write (fd, "FPOSRO", 6);

  res = read (fd, ret, 6);

  serial_deinit (fd);

  if (res > 0)
    {
      out[0] = ret[2];
      out[1] = ret[3];
      out[2] = ret[4];
      out[3] = ret[5];
      *position = atoi (out);
      return 0;
    }
  else
    {
      *position = -1;
      return -1;
    }
}

int
tcf_set_center ()
{
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };

  fd = serial_init ();

  if (fd < 0)
    return -1;

  res = write (fd, "FCENTR", 6);

  res = read (fd, ret, 6);

  serial_deinit (fd);

  if ((res > 0) && (strcmp (ret, "CENTER") == 0))
    return 0;
  else
    return -1;
}

int
tcf_set_sleep ()
{
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };

  fd = serial_init ();

  if (fd < 0)
    return -1;

  res = write (fd, "FSLEEP", 6);

  res = read (fd, ret, 3);

  serial_deinit (fd);

  if ((res > 0) && (strcmp (ret, "ZZZ") == 0))
    return 0;
  else
    return -1;
}

int
tcf_set_wakeup ()
{
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };

  fd = serial_init ();

  if (fd < 0)
    return -1;

  res = write (fd, "FWAKUP", 6);

  res = read (fd, ret, 4);

  serial_deinit (fd);

  if ((res > 0) && (strcmp (ret, "WAKE") == 0))
    return 0;
  else
    return -1;
}

int
tcf_set_auto ()
{
  int fd, res;
  char ret[] = { 0, 0, 0, 0, 0, 0, 0 };

  fd = serial_init ();

  if (fd < 0)
    return -1;

  res = write (fd, "FFMODE", 6);

  res = read (fd, ret, 3);

  serial_deinit (fd);

  if ((res > 0) && (strcmp (ret, "END") == 0))
    return 0;
  else
    return -1;
}
