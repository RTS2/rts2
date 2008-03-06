/* 
 * Class for Newport serial I/O.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "newport.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

void
Rts2DevSensorNewport::resetDevice ()
{
	sleep (5);
	tcflush (dev_port, TCIOFLUSH);
	logStream (MESSAGE_DEBUG) << "Device " << dev << " reseted." << sendLog;
}


int
Rts2DevSensorNewport::writePort (const char *str)
{
	int ret;
	ret = write (dev_port, str, strlen (str));
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Writing '" << str << "' (" << strlen (str) <<
		")" << sendLog;
	#endif
	if (ret != (int) strlen (str))
	{
		resetDevice ();
		return -1;
	}
	ret = write (dev_port, "\n", 1);
	if (ret != 1)
	{
		resetDevice ();
		return -1;
	}
	return 0;
}


int
Rts2DevSensorNewport::readPort (char **rstr, const char *cmd)
{
	static char buf[20];
	int i = 0;
	int ret;

	while (true)
	{
		ret = read (dev_port, buf + i, 1);
		if (ret != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot read from device: " << errno <<
				" (" << strerror (errno) << ")" << sendLog;
			resetDevice ();
			return -1;
		}
		if (buf[i] == '>')
			break;
		i++;
		if (i >= 20)
		{
			logStream (MESSAGE_ERROR) <<
				"Runaway reply string (more then 20 characters)" << sendLog;
			resetDevice ();
			return -1;
		}
	}
	// check that we match \cr\lf[E|value]>
	if (buf[0] != '\n' || buf[1] != '\n')
	{
		logStream (MESSAGE_ERROR) << "Reply string does not start with CR LF"
			<< " (" << std::hex << (int) buf[0] << std::
			hex << (int) buf[1] << ")" << sendLog;
		return -1;
	}
	*rstr = buf + 2;
	buf[i] = '\0';
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_ERROR) << "Read from port " << buf << sendLog;
	#endif
	if (**rstr == 'E')
	{
		logStream (MESSAGE_ERROR) << "Cmd: " << cmd << " Error: " << *rstr <<
			sendLog;
		return -1;
	}
	return 0;
}


int
Rts2DevSensorNewport::readPort (int &ret, const char *cmd)
{
	int r;
	char *rstr;
	r = readPort (&rstr, cmd);
	if (r)
		return r;
	ret = atoi (rstr);
	return 0;
}


int
Rts2DevSensorNewport::readPort (double &ret, const char *cmd)
{
	int r;
	char *rstr;
	r = readPort (&rstr, cmd);
	if (r)
		return r;
	ret = atof (rstr);
	return 0;
}


int
Rts2DevSensorNewport::readRts2Value (const char *valueName, Rts2Value * val)
{
	int ret;
	int iret;
	double dret;
	switch (val->getValueType ())
	{
		case RTS2_VALUE_INTEGER:
			ret = readValue (valueName, iret);
			((Rts2ValueInteger *) val)->setValueInteger (iret);
			return ret;
		case RTS2_VALUE_DOUBLE:
			ret = readValue (valueName, dret);
			((Rts2ValueDouble *) val)->setValueDouble (dret);
			return ret;
	}
	logStream (MESSAGE_ERROR) << "Reading unknow value type " << val->
		getValueType () << sendLog;
	return -1;
}


int
Rts2DevSensorNewport::processOption (int in_opt)
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
Rts2DevSensorNewport::init ()
{
	struct termios term_options; /* structure to hold serial port configuration */
	int ret;

	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	dev_port = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);

	if (dev_port == -1)
	{
		logStream (MESSAGE_ERROR) << "Newport serial port init cannot open: " << dev
			<< " " << strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (dev_port, F_SETFL, 0);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Newport serial port init cannot fcntl "
			<< strerror (errno) << sendLog;
	}
	/* get current serial port configuration */
	if (tcgetattr (dev_port, &term_options) < 0)
	{
		logStream (MESSAGE_ERROR) << "Newport serial port init error reading serial port configuration: "
			<< strerror (errno) << sendLog;
		return -1;
	}

	/*
	 * Set the baud rates to 9600
	 */
	if (cfsetospeed (&term_options, B9600) < 0
		|| cfsetispeed (&term_options, B9600) < 0)
	{
		logStream (MESSAGE_ERROR) << "Newport serial port init error setting baud rate: "
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

	/* set timeout  to 20 seconds */
	term_options.c_cc[VTIME] = 200;
	term_options.c_cc[VMIN] = 0;

	tcflush (dev_port, TCIFLUSH);

	/*
	 * Set the new options for the port...
	 */
	if (tcsetattr (dev_port, TCSANOW, &term_options))
	{
		logStream (MESSAGE_ERROR) << "Newport serial port init tcsetattr "
			<< strerror (errno) << sendLog;
		return -1;
	}

	return 0;
}


Rts2DevSensorNewport::Rts2DevSensorNewport (int in_argc, char **in_argv)
:Rts2DevSensor (in_argc, in_argv)
{
	dev_port = -1;
	dev = "/dev/ttyS0";

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}


Rts2DevSensorNewport::~Rts2DevSensorNewport ()
{
	close (dev_port);
}
