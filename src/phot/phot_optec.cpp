/*!
 * @file Photometer deamon.
 *
 * @author petr
 */
#define FILTER_STEP  33

#include "phot.h"
#include "kernel/phot.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>

using namespace rts2phot;

class Optec:public Photometer
{
	public:
		Optec (int argc, char **argv);
		virtual ~Optec (void);

		virtual int scriptEnds ();
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual long getCount ();

		virtual int baseInfo ()
		{
			return 0;
		};

		virtual int homeFilter ();
		virtual int startFilterMove (int new_filter);
		virtual long isFilterMoving ();
		virtual int stopIntegrate ();
		virtual int enableMove ();
		virtual int disableMove ();
	protected:
		virtual int startIntegrate ();

		virtual int endIntegrate ();
	private:
		const char *phot_dev;
		int fd;
		time_t filter_move_timeout;

		int phot_command (char command, short arg);
};

int Optec::phot_command (char command, short arg)
{
	char cmd_buf[3];
	int ret;
	cmd_buf[0] = command;
	*((short *) (&(cmd_buf[1]))) = arg;
	ret = write (fd, cmd_buf, 3);
	if (ret == 3)
		return 0;
	return -1;
}

Optec::Optec (int in_argc, char **in_argv):Photometer (in_argc, in_argv)
{
	addOption ('f', "phot_file", 1, "photometer file (default to /dev/phot0)");
	phot_dev = "/dev/phot0";
	fd = -1;
}

Optec::~Optec (void)
{
	close (fd);
}

int Optec::scriptEnds ()
{
	// set filter to black
	startFilterMove (0);
	return Photometer::scriptEnds ();
}

int Optec::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			phot_dev = optarg;
			break;
		default:
			return Photometer::processOption (in_opt);
	}
	return 0;
}

int Optec::init ()
{
	int ret;
	ret = Photometer::init ();
	if (ret)
		return ret;

	fd = open (phot_dev, O_RDWR | O_NONBLOCK);
	if (fd == -1)
	{
		perror ("opening photometr");
		return -1;
	}
	// reset occurs on open of photometer file..
	photType = "Optec";
	return 0;
}

long Optec::getCount ()
{
	int ret;
	unsigned short result[2];
	while (1)
	{
		ret = read (fd, &result, 4);
		if (ret == -1)
			break;
		if (ret == 4)
		{
			switch (result[0])
			{
				case '0':
					filter->setValueInteger (result[1] / FILTER_STEP);
					if ((getState () & PHOT_MASK_FILTER) == PHOT_FILTER_MOVE)
						endFilterMove ();
					break;
				case '-':
					return -1;
					break;
			}
		}
		else
		{
			logStream (MESSAGE_ERROR) <<
				"phot Optec getCount invalid read ret: " << ret << sendLog;
			break;
		}
	}
	// we don't care if we get any counts before we change filter..
	if (ret == -1 && errno == EAGAIN && (result[0] == 'A' || result[0] == 'B'))
	{
		sendCount (result[1], req_time, (result[0] == 'B' ? true : false));
		return (long) (req_time * USEC_SEC);
	}
	return 1000;
}

int Optec::homeFilter ()
{
	return phot_command (PHOT_CMD_RESET, 0);
}

int Optec::startIntegrate ()
{
	return phot_command (PHOT_CMD_INTEGRATE, (short) (req_time * 1000));
}

int Optec::endIntegrate ()
{
	return Photometer::endIntegrate ();
}

int Optec::stopIntegrate ()
{
	return Photometer::stopIntegrate ();
}

int Optec::startFilterMove (int new_filter)
{
	int ret;
	ret = phot_command (PHOT_CMD_MOVEFILTER, new_filter * FILTER_STEP);
	if (ret)
		return ret;
	time (&filter_move_timeout);
	// 20 sec timeout for move
	filter_move_timeout += 20;
	return Photometer::startFilterMove (new_filter);
}

long Optec::isFilterMoving ()
{
	time_t now;
	time (&now);
	// timeout..
	if (filter_move_timeout < now)
		return -1;
	return USEC_SEC;
}

int Optec::enableMove ()
{
	return phot_command (PHOT_CMD_INTEGR_ENABLED, 1);
}

int Optec::disableMove ()
{
	int ret;
	ret = phot_command (PHOT_CMD_INTEGR_ENABLED, 0);
	filter->setValueInteger (0);
	infoAll ();
	return ret;
}

int main (int argc, char **argv)
{
	Optec device (argc, argv);
	return device.run ();
}
