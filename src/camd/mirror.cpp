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

int
Rts2DevCameraMirror::mirror_command (char cmd, int arg, char *ret_cmd,
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

Rts2DevCameraMirror::Rts2DevCameraMirror (int argc, char **argv):Rts2DevCamera (argc,
	       argv)
{
  addOption ('m', "mirror_dev", 1, "mirror device");
  mirror_dev = "";
  mirror_fd = -1;
  mirr_log = NULL;
  cmd = 0;
  expChip = -1;
}

Rts2DevCameraMirror::~Rts2DevCameraMirror (void)
{
  if (mirror_fd != -1)
    close (mirror_fd);
  if (mirr_log)
    fclose (mirr_log);
}

int
Rts2DevCameraMirror::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'm':
      mirror_dev = optarg;
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraMirror::init ()
{
  struct termios oldtio, newtio;
  int ret;

  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  if (!mirr_log)
    mirr_log = fopen ("/var/log/rts2-mirror", "a");

  if (*mirror_dev)
    {
      mirror_fd = open (mirror_dev, O_RDWR);
      if (mirror_fd < 0)
	{
	  syslog (LOG_ERR, "Rts2DevCameraMirror::init mirror open: %m");
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

      syslog (LOG_DEBUG, "Rts2DevCameraMirror::init mirror initialized on %s",
	      mirror_dev);
    }

  return 0;
}

int
Rts2DevCameraMirror::idle ()
{
  int ret;
  ret = Rts2DevCamera::idle ();
  switch (cmd)
    {
    case CMD_MIRROR_PLUS:
      ret = isOpened ();
      if (ret == -2)
	return endOpen ();
      break;
    case CMD_MIRROR_MINUS:
      ret = isClosed ();
      if (ret == -2)
	return endClose ();
      break;
    default:
      return ret;
    }
  // set timeouts..
  setTimeoutMin (1000);
}

int
Rts2DevCameraMirror::mirror_get (int *pos)
{
  return mirror_command (CMD_MIRROR_GET, 0, NULL, pos);
}


int
Rts2DevCameraMirror::mirror_set ()
{
  int ret;
  int mpos, new_pos;
  int i;
  int move = 1;

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

  ret = mirror_command (cmd, move, NULL, NULL);
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
      if (mpos == new_pos)
	{
	  steps -= move;
	  return 0;
	}
    }
  return -1;
}

Rts2Conn *
Rts2DevCameraMirror::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnCameraMirror (in_sock, this);
}

int
Rts2DevCameraMirror::startOpen ()
{
  cmd = CMD_MIRROR_PLUS;
  steps = 170;
  return 0;
}

int
Rts2DevCameraMirror::isOpened ()
{
  return mirror_set ();
}

int
Rts2DevCameraMirror::endOpen ()
{
  cmd = 0;
}

int
Rts2DevCameraMirror::startClose ()
{
  cmd = CMD_MIRROR_MINUS;
  steps = 170;
  return 0;
}

int
Rts2DevCameraMirror::isClosed ()
{
  return mirror_set ();
}

int
Rts2DevCameraMirror::endClose ()
{
  cmd = 0;
}

int
Rts2DevCameraMirror::camExpose (int chip, int light, float exptime)
{
  if (mirror_fd != -1 && light)
    {
      expChip = chip;
      expLight = light;
      expExpTime = exptime;
      return startOpen ();
    }
  expChip = -1;
  return Rts2DevCamera::camExpose (chip, light, exptime);
}

long
Rts2DevCameraMirror::camWaitExpose (int chip)
{
  long ret;
  if (expChip == chip)
    {
      // wait for mirror opening
      if (isMoving ())
	return 1000;
      ret = Rts2DevCamera::camExpose (expChip, expLight, expExpTime);
      if (ret)
	return -2;
      expChip = -1;
    }
  ret = Rts2DevCamera::camWaitExpose (chip);
  if (ret == -2)
    {
      if (mirror_fd != -1 && expLight)
	startClose ();
    }
  return ret;
}

int
Rts2DevConnCameraMirror::commandAuthorized ()
{
  if (isCommand ("mirror"))
    {
      char *str_dir;
      int ret = 0;
      CHECK_PRIORITY;
      if (paramNextString (&str_dir) || !paramEnd ())
	return -2;
      if (!strcasecmp (str_dir, "open"))
	ret = master->startOpen ();
      if (!strcasecmp (str_dir, "close"))
	ret = master->startClose ();
      if (ret)
	{
	  sendCommandEnd (DEVDEM_E_HW, "cannot open/close mirror");
	  return -1;
	}
      return 0;
    }
  return Rts2DevConnCamera::commandAuthorized ();
}
