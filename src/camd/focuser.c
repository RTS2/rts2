/** 
 * @file module for focuser
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>

#include "focuser.h"

#define BAUDRATE B9600

#define CMD_FOCUS_MOVE_PLUS		0xC3
#define CMD_FOCUS_MOVE_MINUS		0xC1
#define CMD_FOCUS_GET			0xC5

#define CMD_MIRROR_PLUS			0xC0
#define CMD_MIRROR_MINUS		0xC2
#define CMD_MIRROR_GET			0xC4

#define DEVICE				"/dev/ttyS1"

int focus_fd;

int
command (char cmd, int arg)
{
  char command_buffer[3];
  int ret;
  command_buffer[0] = cmd;
  *((int *) &command_buffer[1]) = arg;
  ret = write (focus_fd, command_buffer, 3);
  return (ret == 3 ? 0 : -1);
}

int
focus_open ()
{
  struct termios oldtio, newtio;
  focus_fd = open (DEVICE, O_RDWR);
  if (focus_fd == -1)
    {
      perror ("focus_open");
      return -1;
    }
  tcgetattr (focus_fd, &oldtio);

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 0;

  tcflush (focus_fd, TCIOFLUSH);
  tcsetattr (focus_fd, TCSANOW, &newtio);

  return 0;
}

int
focus_set (int steps)
{
  if (steps > 0)
    return command (CMD_FOCUS_MOVE_MINUS, steps);
  else
    return command (CMD_FOCUS_MOVE_MINUS, -steps);
}

int
focus_get ()
{
  char command_buffer[3];
  command (CMD_FOCUS_GET, 0);
  read (focus_fd, command_buffer, 3);
  return *((int *) &command_buffer[1]);
}

int
mirror_set (int steps)
{
  if (steps > 0)
    return command (CMD_MIRROR_PLUS, steps);
  else
    return command (CMD_MIRROR_MINUS, -steps);
}

int
mirror_get ()
{
  char command_buffer[3];
  command (CMD_MIRROR_GET, 0);
  read (focus_fd, command_buffer, 3);
  return *((int *) &command_buffer[1]);
}

int
main (int argc, char **argv)
{
  focus_open ();
  printf ("focuse_get: %i\n", focus_get ());
  printf ("mirror_get: %i\n", mirror_get ());
  mirror_set (10);
  focus_set (10);
  return 0;
}
