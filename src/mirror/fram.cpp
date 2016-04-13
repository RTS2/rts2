/**
 * @file Module for mirror control.
 *
 * @author Petr Kubanek <petr@lascaux.asu.cas.cz>
 */

#include <iostream>
#include <stdio.h>
#include <termios.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <stdio.h>

#include "mirror.h"

#define BAUDRATE B4800

#define CMD_MIRROR_PLUS     0xC1
#define CMD_MIRROR_MINUS    0xC3
#define CMD_MIRROR_GET      0xC5

#define RAMP_TO     1
#define RAMP_STEP   1
#define STEPS_OPEN_CLOSE  180

namespace rts2mirror
{

class Fram:public Mirror
{
	public:
		Fram (int argc, char **argv);
		virtual ~Fram (void);

		virtual int processOption (int in_op);
		virtual int init ();

		virtual int movePosition (int pos);
		virtual int isMoving ();

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
};

}

using namespace rts2mirror;

int Fram::mirror_command (char cmd, int arg, char *ret_cmd, int *ret_arg)
{
	char command_buffer[3];
	int ret;
	size_t readed;
	char tc[27];
	time_t t;
	command_buffer[0] = cmd;
	*((int *) &command_buffer[1]) = arg;
	logStream (MESSAGE_DEBUG) << "cmd " << cmd << " arg " << arg << sendLog;

	ret = write (mirror_fd, command_buffer, 3);
	if (ret != 3)
	{
		t = time (NULL);
		ctime_r (&t, tc);
		fprintf (mirr_log, "%s:write %i\n", tc, ret);
		fflush (mirr_log);
		return -1;
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
			return -1;
		}
		readed += ret;
	}
	logStream (MESSAGE_DEBUG) << "ret_cmd: " << command_buffer[0]
		<< " ret_arg: " << command_buffer[1] << sendLog;
	if (ret_cmd)
		*ret_cmd = command_buffer[0];
	if (ret_arg)
		*ret_arg = *((int *) &command_buffer[1]);
	flock (mirror_fd, LOCK_UN);
	return 0;
}

Fram::Fram (int in_argc, char **in_argv):Mirror (in_argc, in_argv)
{
	addOption ('f', "mirror_dev", 1, "mirror device");
	mirror_dev = NULL;
	mirror_fd = -1;
	mirr_log = NULL;
}

Fram::~Fram (void)
{
	if (mirror_fd != -1)
		close (mirror_fd);
	if (mirr_log)
		fclose (mirr_log);
}

int Fram::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			mirror_dev = optarg;
			break;
		default:
			return Mirror::processOption (in_opt);
	}
	return 0;
}

int Fram::init ()
{
	struct termios oldtio, newtio;
	int ret;

	ret = Mirror::init ();
	if (ret)
		return ret;

	if (!mirr_log)
		mirr_log = fopen ("/var/log/rts2-mirror", "a");

	if (!mirror_dev)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevMirrorFram::init /dev entry wasn't passed as parameter - exiting" << sendLog;
		return -1;
	}

	mirror_fd = open (mirror_dev, O_RDWR);
	if (mirror_fd < 0)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevMirrorFram::init mirror open: " << strerror (errno) << sendLog;
		return -1;
	}

	ret = tcgetattr (mirror_fd, &oldtio);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Fram::init tcgetattr " << strerror (errno) << sendLog;
		return -1;
	}

	newtio = oldtio;

	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN] = 0;
	newtio.c_cc[VTIME] = 4;

	tcflush (mirror_fd, TCIOFLUSH);
	ret = tcsetattr (mirror_fd, TCSANOW, &newtio);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Fram::init tcsetattr " << strerror (errno) << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "Fram::init mirror initialized on " << mirror_dev << sendLog;

	return movePosition (0);
}

int Fram::mirror_get (int *pos)
{
	return mirror_command (CMD_MIRROR_GET, 0, NULL, pos);
}

int Fram::mirror_set ()
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

	if ((getState () & MIRROR_MASK) == MIRROR_MOVE)
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

int Fram::movePosition (int pos)
{
	steps = STEPS_OPEN_CLOSE;
	return 0;
}

int Fram::isMoving ()
{
	return mirror_set ();
}

int main (int argc, char **argv)
{
	Fram device (argc, argv);
	return device.run ();
}
