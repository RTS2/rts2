/** 
 * @file Module for mirror control. 
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */

#include <stdio.h>
#include <syslog.h>
#include <termios.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <stdio.h>

#include "mirror.h"

#define BAUDRATE B4800

#define CMD_MIRROR_PLUS			0xC1
#define CMD_MIRROR_MINUS		0xC3
#define CMD_MIRROR_GET			0xC5

#define RAMP_TO			1
#define RAMP_STEP		1

class Rts2DevMirrorFram:public Rts2DevMirror
{
private:
  char *mirror_dev;
  int mirror_fd;
  FILE *mirr_log;

  int mirror_command (char cmd, int arg, char *ret_cmd, int *ret_arg);
  int mirror_get (int *pos);
  // need to set steps before calling this one
  int mirror_set ();

  int steps;
  int set_ret;
public:
    Rts2DevMirrorFram (int argc, char **argv);
    virtual ~ Rts2DevMirrorFram (void);

  virtual int processOption (int in_op);
  virtual int init ();

  virtual int startOpen ();
  virtual int isOpened ();

  virtual int startClose ();
  virtual int isClosed ();

};

int
Rts2DevMirrorFram::mirror_command (char cmd, int arg, char *ret_cmd,
				   int *ret_arg)
{
  char command_buffer[3];
  int ret;
  size_t readed;
  int tries = 0;
  char tc[27];
  time_t t;
  command_buffer[0] = cmd;
  *((int *) &command_buffer[1]) = arg;
  flock (mirror_fd, LOCK_EX);
start:
  ret = write (mirror_fd, command_buffer, 3);
  if (ret != 3)
    {
      t = time (NULL);
      ctime_r (&t, tc);
      fprintf (mirr_log, "%s:write %i\n", tc, ret);
      fflush (mirr_log);
      goto err;
    }
  readed = 0;
  while (readed != 3)
    {
      ret = read (mirror_fd, &command_buffer[readed], 3);
      if (ret <= 0)
	{
	  t = time (NULL);
	  ctime_r (&t, tc);
	  fprintf (mirr_log, "%s:read %i\n", tc, ret);
	  fflush (mirr_log);
	  goto err;
	}
      readed += ret;
    }
  if (ret_cmd)
    *ret_cmd = command_buffer[0];
  if (ret_arg)
    *ret_arg = *((int *) &command_buffer[1]);
  flock (mirror_fd, LOCK_UN);
  return 0;
err:
  tries++;
  if (tries < 4)
    {
      tcflush (mirror_fd, TCIOFLUSH);
      sleep (1);
      goto start;
    }
  flock (mirror_fd, LOCK_UN);
  return -1;
}

Rts2DevMirrorFram::Rts2DevMirrorFram (int argc, char **argv):Rts2DevMirror (argc,
	       argv)
{
  addOption ('f', "mirror_dev", 1, "mirror device");
  mirror_dev = "";
  mirror_fd = -1;
  mirr_log = NULL;
}

Rts2DevMirrorFram::~Rts2DevMirrorFram (void)
{
  if (mirror_fd != -1)
    close (mirror_fd);
  if (mirr_log)
    fclose (mirr_log);
}

int
Rts2DevMirrorFram::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      mirror_dev = optarg;
      break;
    default:
      return Rts2DevMirror::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevMirrorFram::init ()
{
  struct termios oldtio, newtio;
  int ret;

  ret = Rts2DevMirror::init ();
  if (ret)
    return ret;

  if (!mirr_log)
    mirr_log = fopen ("/var/log/rts2-mirror", "a");

  if (*mirror_dev)
    {
      mirror_fd = open (mirror_dev, O_RDWR);
      if (mirror_fd < 0)
	{
	  syslog (LOG_ERR, "Rts2DevMirrorFram::init mirror open: %m");
	  return -1;
	}

      tcgetattr (mirror_fd, &oldtio);

      newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
      newtio.c_iflag = IGNPAR;
      newtio.c_oflag = 0;
      newtio.c_lflag = 0;
      newtio.c_cc[VMIN] = 0;
      newtio.c_cc[VTIME] = 40;

      tcflush (mirror_fd, TCIOFLUSH);
      tcsetattr (mirror_fd, TCSANOW, &newtio);

      syslog (LOG_DEBUG, "Rts2DevMirrorFram::init mirror initialized on %s",
	      mirror_dev);
    }

  return 0;
}

int
Rts2DevMirrorFram::mirror_get (int *pos)
{
  return mirror_command (CMD_MIRROR_GET, 0, NULL, pos);
}


int
Rts2DevMirrorFram::mirror_set ()
{
  int ret;
  int mpos, new_pos;
  int i;
  int move = 1;
  int cmd;

  if (steps <= 0)
    return -2;

  ret = mirror_get (&mpos);
  if (ret)
    return -1;

  if (steps > RAMP_STEP)
    {
      if (steps < RAMP_TO * RAMP_STEP)
	move -= RAMP_STEP;
      else if (move < RAMP_TO)
	move += RAMP_STEP;
    }
  if (move > steps || move < 0)
    {
      move = steps;
    }

  if ((getState (0) & MIRROR_MASK) == MIRROR_A_B)
    {
      cmd = CMD_MIRROR_PLUS;
      new_pos = mpos + move;
    }
  else
    {
      cmd = CMD_MIRROR_MINUS;
      new_pos = mpos - move;
    }

  ret = mirror_command (cmd, move, NULL, NULL);
  if (ret)
    return -1;

  for (i = 0; i < 7; i++)
    {
      ret = mirror_get (&mpos);
      if (ret)
	return -1;
      if (mpos == new_pos)
	{
	  steps -= move;
	  return 0;
	}
    }
  return -1;
}

int
Rts2DevMirrorFram::startOpen ()
{
  steps = 170;
  return Rts2DevMirror::startOpen ();
}

int
Rts2DevMirrorFram::isOpened ()
{
  return mirror_set ();
}

int
Rts2DevMirrorFram::startClose ()
{
  steps = 170;
  return Rts2DevMirror::startClose ();
}

int
Rts2DevMirrorFram::isClosed ()
{
  return mirror_set ();
}

int
main (int argc, char **argv)
{
  Rts2DevMirrorFram *device = new Rts2DevMirrorFram (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize FRAM mirror - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
