/** 
 * @file module for focuser
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */

#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

#include "focuser.h"

#define BAUDRATE B4800

#define CMD_MIRROR_PLUS			0xC1
#define CMD_MIRROR_MINUS		0xC3
#define CMD_MIRROR_GET			0xC5

static int mirror_fd = 0;

int
command (char cmd, int arg, char *ret_cmd, int *ret_arg)
{
  char command_buffer[3];
  int ret;
  command_buffer[0] = cmd;
  *((int *) &command_buffer[1]) = arg;
  usleep (20);
  ret = write (mirror_fd, command_buffer, 3);
  printf ("write: %i\n", ret);
  if (ret != 3)
    return -1;
  ret = read (mirror_fd, command_buffer, 3);
  printf ("read: %i\n", ret);
  if (ret != 3)
    return -1;
  if (ret_cmd)
    *ret_cmd = command_buffer[0];
  if (ret_arg)
    *ret_arg = *((int *) &command_buffer[1]);
  return 0;
}

int
mirror_open_dev (char *dev)
{
  struct termios oldtio, newtio;
  mirror_fd = open (dev, O_RDWR);
  if (mirror_fd < 0)
    {
      perror ("focus_open");
      return -1;
    }
  tcgetattr (mirror_fd, &oldtio);

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 5;

  tcflush (mirror_fd, TCIOFLUSH);
  tcsetattr (mirror_fd, TCSANOW, &newtio);

  return 0;
}

int
mirror_get (int *pos)
{
  return command (CMD_MIRROR_GET, 0, NULL, pos);
}

int
mirror_set (int steps)
{
  int ret;
  int mpos, new_pos;
  int i;
  int cmd;

  ret = mirror_get (&mpos);
  if (ret)
    return -1;
  printf ("step: %i\n", steps);
  if (steps > 0)
    {
      cmd = CMD_MIRROR_PLUS;
    }
  else
    {
      cmd = CMD_MIRROR_MINUS;
      steps = -steps;
    }
  while (steps > 0)
    {
      int move;
      move = (steps >= 2) ? 2 : steps;
      ret = command (cmd, move, NULL, NULL);
      if (ret)
	return -1;
      if (cmd == CMD_MIRROR_PLUS)
	{
	  new_pos = mpos + move;
	}
      else
	{
	  new_pos = mpos - move;
	}
      for (i = 0; i < 7; i++)
	{
	  ret = mirror_get (&mpos);
	  if (ret)
	    return -1;
	  printf ("pos: %i  target: %i\r", mpos, new_pos);
	  fflush (stdout);
	  if (mpos == new_pos)
	    {
	      usleep (10);
	      break;
	    }
	  usleep (200);
	}
      usleep (500);
      steps -= move;
    }

  return -1;
}

int
mirror_open ()
{
  if (!mirror_fd)
    return -1;
  return mirror_set (170);
}

int
mirror_close ()
{
  if (!mirror_fd)
    return -1;
  return mirror_set (-170);
}
