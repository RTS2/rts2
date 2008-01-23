/* 
 * Driver for Ealing DS-21 motor controller.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "sensord.h"

#define DEBUG_ALL

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

class Rts2DevSensorDS21;

/**
 * Holds values and perform operations on them for one axis.
 */
class DS21Axis
{
	private:
		Rts2DevSensorDS21 *master;
		char anum;

		Rts2ValueBool *enabled;
		Rts2ValueLong *position;
		Rts2ValueLong *poserr;
		Rts2ValueInteger *velocity;
		Rts2ValueInteger *acceleration;

		Rts2ValueString *commandSet;
	public:
		/**
		 * @param in_anum Number of the axis being constructed.
		 */
		DS21Axis (Rts2DevSensorDS21 *in_master, char in_anum);
		/**
		 * @return -3 when value was not found in this axis.
		 */
		int setValue (Rts2Value *old_value, Rts2Value *new_value);
		int info ();
};

class Rts2DevSensorDS21: public Rts2DevSensor
{
	private:
		char *dev;
		int dev_port;

		std::list <DS21Axis> axes;

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);

	public:
		Rts2DevSensorDS21 (int in_argc, char **in_argv);
		virtual ~Rts2DevSensorDS21 (void);

		template < typename T > void createAxisValue( T * &val, char anum,
			const char *in_val_name, const char *in_desc, bool writeToFits)
		{
			char *n = new char[strlen (in_val_name) + 3];
			n[0] = anum + '0';
			n[1] = '.';
			strcpy (n+2, in_val_name);
			createValue (val, n, in_desc, writeToFits);
			delete []n;
		}

		virtual int info ();

		int writePort (char anum, const char *msg);
		int readPort (char *buf, int blen);

		int writeReadPort (char anum, const char *msg, char *buf, int blen);

		int writeValue (char anum, const char cmd[3], Rts2Value *val);
		int readValue (char anum, const char *cmd, Rts2Value *val);
};

DS21Axis::DS21Axis (Rts2DevSensorDS21 *in_master, char in_anum)
{
	master = in_master;
	anum = in_anum;

	master->createAxisValue (enabled, anum, "enabled", "if motor is enabled", false);
	master->createAxisValue (position, anum, "POSITION", "motor position", true);
	master->createAxisValue (poserr, anum, "POS_ERROR", "motor position error", true);
	master->createAxisValue (velocity, anum, "velocity", "programmed velocity", false);
	master->createAxisValue (acceleration, anum, "acceleration", "programmed acceleration", false);

	master->createAxisValue (commandSet, anum, "commands", "commands for this motor", false);
}


int
DS21Axis::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	if (old_value == enabled)
	{
		return (master->writePort (anum, ((Rts2ValueBool *) new_value)->getValueBool () ? "MN" : "MF")) ? -2 : 0;
	}
	if (old_value == position)
	{
		return (master->writeValue (anum, "MA", new_value)) ? -2 : 0;
	}
	if (old_value == velocity)
	{
		return (master->writeValue (anum, "SV", new_value)) ? -2 : 0;
	}
	if (old_value == acceleration)
	{
		return (master->writeValue (anum, "SA", new_value)) ? -2 : 0;
	}
	return -3;
}


int
DS21Axis::info ()
{
	master->writePort (anum, "RF");
	master->readValue (anum, "TP", position);
	master->readValue (anum, "TE", poserr);
	master->readValue (anum, "GV", velocity);
	master->readValue (anum, "GA", acceleration);
	master->readValue (anum, "HE", commandSet);
	return 0;
}


int
Rts2DevSensorDS21::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Rts2DevSensor::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevSensorDS21::init ()
{
	struct termios term_options; /* structure to hold serial port configuration */
	int ret;

	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	dev_port = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);

	if (dev_port == -1)
	{
		logStream (MESSAGE_ERROR) << "init cannot open: " << dev
			<< strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (dev_port, F_SETFL, 0);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "init cannot fcntl " <<
			strerror (errno) << sendLog;
	}
	/* get current serial port configuration */
	if (tcgetattr (dev_port, &term_options) < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"init error reading serial port configuration: " <<
			strerror (errno) << sendLog;
		return -1;
	}

	/*
	 * Set the baud rates to 9600
	 */
	if (cfsetospeed (&term_options, B9600) < 0
		|| cfsetispeed (&term_options, B9600) < 0)
	{
		logStream (MESSAGE_ERROR) << "init error setting baud rate: "
			<< strerror (errno) << sendLog;
		return -1;
	}

	/*
	 * Enable the receiver and set local mode...
	 */
	term_options.c_cflag |= (CLOCAL | CREAD);

	/* Choose raw input */
	term_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* set port to 8 data bits, 1 stop bit, no parity */
	term_options.c_cflag &= ~PARENB;
	term_options.c_cflag &= ~CSTOPB;
	term_options.c_cflag &= ~CSIZE;
	term_options.c_cflag |= CS8;

	/* set timeout to 2 seconds */
	term_options.c_cc[VTIME] = 20;
	term_options.c_cc[VMIN] = 0;

	tcflush (dev_port, TCIFLUSH);

	/*
	 * Set the new options for the port...
	 */
	if (tcsetattr (dev_port, TCSANOW, &term_options))
	{
		logStream (MESSAGE_ERROR) << "init tcsetattr " <<
			strerror (errno) << sendLog;
		return -1;
	}

	tcflush (dev_port, TCIOFLUSH);

	axes.push_back (DS21Axis (this, 5));
	axes.push_back (DS21Axis (this, 3));

	return 0;
}


int
Rts2DevSensorDS21::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	for (std::list <DS21Axis>::iterator iter = axes.begin (); iter != axes.end (); iter++)
	{
		int ret = (*iter).setValue (old_value, new_value);
		if (ret != -3)
			return ret;
	}
	return Rts2DevSensor::setValue (old_value, new_value);
}


Rts2DevSensorDS21::Rts2DevSensorDS21 (int in_argc, char **in_argv)
:Rts2DevSensor (in_argc, in_argv)
{
	dev = "/dev/ttyS0";

	addOption ('f', NULL, 1, "device serial port, default to /dev/ttyS0");
}


Rts2DevSensorDS21::~Rts2DevSensorDS21 (void)
{
	close (dev_port);
}


int
Rts2DevSensorDS21::info ()
{
	int ret;
	for (std::list <DS21Axis>::iterator iter = axes.begin (); iter != axes.end (); iter++)
	{
		ret = (*iter).info ();
		if (ret)
			return ret;
	}
	return Rts2DevSensor::info ();
}


int
Rts2DevSensorDS21::writePort (char anum, const char *msg)
{
	int blen = strlen (msg);
	char *buf = new char[blen + 4];
	buf[0] = anum + '0';
	strcpy (buf + 1, msg);
	buf[blen + 1] = '\r';
	buf[blen + 2] = '\n';
	buf[blen + 3] = '\0';
	int ret = -1;
	do
	{
		ret = write (dev_port, buf, blen + 3);
	} while (ret == -1 && errno == EINTR);
	if (ret != blen + 3)
		return -1;

	#ifdef DEBUG_ALL
	logStream (MESSAGE_DEBUG) << "write on port " << buf << "  " << ret << sendLog;
	#endif
	return 0;
}


int
Rts2DevSensorDS21::readPort (char *buf, int blen)
{
	int ret;
	char *buf_top = buf;
	while (buf_top - buf < blen)
	{
		do
		{
			ret = read (dev_port, buf_top, 1);
			//			#ifdef DEBUG_ALL
			//			std::cout << (*buf_top) << " " << ret << std::endl;
			//			#endif
		} while (ret == -1 && errno == EINTR);
		if (ret == -1 || ret == 0)
			return -1;
		if (*buf_top == '\r')
		{
			*buf_top = '\0';
		}
		if (*buf_top == '\n')
		{
			std::cout << "get \\n" << std::endl;
			break;
		}
		buf_top++;
	}
	if (ret <= 0)
	{
		return -1;
	}
	*buf_top = '\0';
	#ifdef DEBUG_ALL
	logStream (MESSAGE_DEBUG) << "readed from port '" << buf << "' readed " << ret << sendLog;
	#endif
	return 0;
}


int
Rts2DevSensorDS21::writeReadPort (char anum, const char *msg, char *buf, int blen)
{
	int ret;
	ret = writePort (anum, msg);
	if (ret)
		return ret;
	return readPort (buf, blen);

}


int
Rts2DevSensorDS21::writeValue (char anum, const char cmd[3], Rts2Value *val)
{
	int ret;
	const char *sval = val->getValue ();
	char *buf = new char[strlen (sval) + 3];
	memcpy (buf, cmd, 2);
	strcpy (buf + 2, sval);

	ret = writePort (anum, buf);
	delete[] buf;
	return ret;
}


int
Rts2DevSensorDS21::readValue (char anum, const char *cmd, Rts2Value *val)
{
	int ret;
	char buf[500];

	tcflush (dev_port, TCIOFLUSH);

	ret = writeReadPort (anum, cmd, buf, 500);
	if (ret)
		return -1;
	return val->setValueString (buf);
}


int
main (int argc, char **argv)
{
	Rts2DevSensorDS21 device = Rts2DevSensorDS21 (argc, argv);
	return device.run ();
}
