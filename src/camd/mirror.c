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

#include <stdio.h>

#include "focuser.h"

#define BAUDRATE B4800

#define CMD_MIRROR_PLUS			0xC1
#define CMD_MIRROR_MINUS		0xC3
#define CMD_MIRROR_GET			0xC5

#define RAMP_TO			10
#define RAMP_STEP		1

static int mirror_fd = 0;

//! sempahore id structure, we have two semaphores...
int mirror_semid = -1;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun
{
  int val;			/* value for SETVAL */
  struct mirror_semid_ds *buf;	/* buffer for IPC_STAT, IPC_SET */
  unsigned short int *array;	/* array for GETALL, SETALL */
  struct seminfo *__buf;	/* buffer for IPC_INFO */
};
#endif

int
command (char cmd, int arg, char *ret_cmd, int *ret_arg)
{
  char command_buffer[3];
  int ret;
  command_buffer[0] = cmd;
  *((int *) &command_buffer[1]) = arg;
  flock (mirror_fd, LOCK_EX);
  usleep (20);
  ret = write (mirror_fd, command_buffer, 3);
  printf ("write: %i\n", ret);
  if (ret != 3)
    goto err;
  ret = read (mirror_fd, command_buffer, 3);
  printf ("read: %i\n", ret);
  if (ret != 3)
    goto err;
  if (ret_cmd)
    *ret_cmd = command_buffer[0];
  if (ret_arg)
    *ret_arg = *((int *) &command_buffer[1]);
  flock (mirror_fd, LOCK_UN);
  return 0;
err:
  flock (mirror_fd, LOCK_UN);
  return -1;
}

int
mirror_open_dev (char *dev)
{
  struct termios oldtio, newtio;
  union semun sem_un;
  unsigned short int sem_arr[] = { 1 };

  mirror_fd = open (dev, O_RDWR);
  if (mirror_fd < 0)
    {
      perror ("focus_open");
      return -1;
    }

  if ((mirror_semid = semget (ftok (dev, 0), 1, 0644)) == -1)
    {
      if ((mirror_semid = semget (ftok (dev, 0), 1, IPC_CREAT | 0644)) == -1)
	{
	  syslog (LOG_ERR, "semget: %m");
	  return -1;
	}
      sem_un.array = sem_arr;

      if (semctl (mirror_semid, 0, SETALL, sem_un) < 0)
	{
	  syslog (LOG_ERR, "semctl init: %m");
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

    }

  syslog (LOG_DEBUG, "mirror:Initialization complete");

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
  int move = 1;

  struct sembuf sem_buf;
  sem_buf.sem_num = 0;
  sem_buf.sem_op = -1;
  sem_buf.sem_flg = SEM_UNDO;

  if (semop (mirror_semid, &sem_buf, 1) == -1)
    return -1;
  sem_buf.sem_op = +1;

  ret = mirror_get (&mpos);
  if (ret)
    goto err;
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
      if (steps < RAMP_TO * RAMP_STEP)
	move -= RAMP_STEP;
      else if (move < RAMP_TO)
	move += RAMP_STEP;
      ret = command (cmd, move, NULL, NULL);
      if (ret)
	goto err;
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
	    goto err;
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

  semop (mirror_semid, &sem_buf, 1);
  return 0;
err:
  semop (mirror_semid, &sem_buf, 1);

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
