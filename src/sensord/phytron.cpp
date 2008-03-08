#include "sensord.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#define CMDBUF_LEN  20

class Rts2DevSensorPhytron:public Rts2DevSensor
{
	private:
		int dev_port;

		int writePort (const char *str);
		int readPort ();

		char cmdbuf[CMDBUF_LEN];

		Rts2ValueInteger* axis0;
		Rts2ValueInteger* runFreq;
		Rts2ValueSelection* phytronParams[47];

		int readValue (int ax, int reg, Rts2ValueInteger *val);
		int readAxis ();
		int setValue (int ax, int reg, Rts2ValueInteger *val);
		int setAxis (int new_val);
		char *dev;
	protected:
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
		virtual int processOption (int in_opt);

	public:
		Rts2DevSensorPhytron (int in_argc, char **in_argv);
		virtual ~ Rts2DevSensorPhytron (void);

		virtual int init ();
		virtual int info ();
};

int
Rts2DevSensorPhytron::writePort (const char *str)
{
	int ret;
	// send prefix
	ret = write (dev_port, "\002", 1);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot send prefix" << sendLog;
		return -1;
	}
	int len = strlen (str);
	ret = write (dev_port, str, len);
	if (ret != len)
	{
		logStream (MESSAGE_ERROR) << "Cannot send body" << sendLog;
		return -1;
	}
	// send postfix
	ret = write (dev_port, "\003", 1);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "Cannot send postfix" << sendLog;
		return -1;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Writing '" << str << "' (" << strlen (str) <<
		")" << sendLog;
	#endif
	return 0;
}


int
Rts2DevSensorPhytron::readPort ()
{
	int readed = 0;
	char *buf_top = cmdbuf;
	while (readed < CMDBUF_LEN)
	{
		int ret = read (dev_port, buf_top, 1);
		if (ret != 1)
		{
			logStream (MESSAGE_ERROR) << "Cannot read from port, returned " <<
				ret << " error " << strerror (errno) << " readed " << readed <<
				sendLog;
			return -1;
		}
		readed++;
		if (*buf_top == '\003')
		{
			*buf_top = '\0';
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "Readed " << cmdbuf << sendLog;
			#endif
			return 0;
		}
		buf_top++;
	}
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "String does not ends " << cmdbuf << sendLog;
	#endif
	return -1;
}


int
Rts2DevSensorPhytron::readValue (int ax, int reg, Rts2ValueInteger *val)
{
	char buf[50];
	snprintf (buf, 50, "%02iP%2iR", ax, reg);
	int ret = writePort (buf);
	if (ret < 0)
		return ret;

	ret = readPort ();
	if (ret)
		return ret;

	if (cmdbuf[0] != '\002' || cmdbuf[1] != '\006')
	{
		logStream (MESSAGE_ERROR) << "Invalid header" << sendLog;
		return -1;
	}
	int ival = atoi (cmdbuf + 2);
	val->setValueInteger (ival);
	return 0;
}


int
Rts2DevSensorPhytron::readAxis ()
{
	int ret;
	ret = readValue (1, 21, axis0);
	if (ret)
		return ret;
	ret = readValue (1, 14, runFreq);
	if (ret)
		return ret;
	return ret;
}


int
Rts2DevSensorPhytron::setValue (int ax, int reg, Rts2ValueInteger *val)
{
	char buf[50];
	snprintf (buf, 50, "%02iP%2iS%i", ax, reg, val->getValueInteger ());
	int ret = writePort (buf);
	if (ret < 0)
		return ret;
	ret = readPort ();
	return ret;
}


int
Rts2DevSensorPhytron::setAxis (int new_val)
{
	int ret;
	sprintf (cmdbuf, "01A%i", new_val);
	logStream (MESSAGE_DEBUG) << "Change axis to " << new_val << sendLog;
	ret = writePort (cmdbuf);
	if (ret)
		return ret;
	ret = readPort ();
	if (ret)
		return ret;
	do
	{
		ret = readAxis ();
	}
	while (axis0->getValueInteger () != new_val);
	return 0;
}


Rts2DevSensorPhytron::Rts2DevSensorPhytron (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
	dev_port = -1;
	dev = "/dev/ttyS0";

	createValue (runFreq, "RUNFREQ", "current run frequency", true);
	createValue (axis0, "CURPOS", "current arm position", true, RTS2_VWHEN_RECORD_CHANGE, 0, false);

	// create phytron params
	/*	createValue (phytronParams[0], "P01", "Type of movement", false);
		((Rts2ValueSelection *) phytronParams[0])->addSelVal ("rotational");
		((Rts2ValueSelection *) phytronParams[0])->addSelVal ("linear");

		createValue ((Rts2ValueSelection *)phytronParams[1], "P02", "Measuring units of movement", false);
		((Rts2ValueSelection *) phytronParams[1])->addSelVal ("step");
		((Rts2ValueSelection *) phytronParams[1])->addSelVal ("mm");
		((Rts2ValueSelection *) phytronParams[1])->addSelVal ("inch");
		((Rts2ValueSelection *) phytronParams[1])->addSelVal ("degree"); */

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}


Rts2DevSensorPhytron::~Rts2DevSensorPhytron (void)
{
	close (dev_port);
	dev_port = -1;
}


int
Rts2DevSensorPhytron::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == axis0)
		return setAxis (new_value->getValueInteger ());
	if (old_value == runFreq)
		return setValue (1, 14, (Rts2ValueInteger *)new_value);
	return Rts2DevSensor::setValue (old_value, new_value);
}


int
Rts2DevSensorPhytron::processOption (int in_opt)
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
Rts2DevSensorPhytron::init ()
{
	struct termios term_options; /* structure to hold serial port configuration */
	int ret;

	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	dev_port = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);

	if (dev_port == -1)
	{
		logStream (MESSAGE_ERROR) << "filter ifw init cannot open: " << dev
			<< strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (dev_port, F_SETFL, 0);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "filter ifw init cannot fcntl " <<
			strerror (errno) << sendLog;
	}
	/* get current serial port configuration */
	if (tcgetattr (dev_port, &term_options) < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"filter ifw init error reading serial port configuration: " <<
			strerror (errno) << sendLog;
		return -1;
	}

	/*
	 * Set the baud rates to 9600
	 */
	if (cfsetospeed (&term_options, B57600) < 0
		|| cfsetispeed (&term_options, B57600) < 0)
	{
		logStream (MESSAGE_ERROR) << "filter ifw init error setting baud rate: "
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
		logStream (MESSAGE_ERROR) << "phytron init tcsetattr " <<
			strerror (errno) << sendLog;
		return -1;
	}
	ret = readAxis ();
	if (ret)
		return ret;

	// char *rstr;

	return 0;
}


int
Rts2DevSensorPhytron::info ()
{
	int ret;
	ret = readAxis ();
	if (ret)
		return ret;
	return Rts2DevSensor::info ();
}


int
main (int argc, char **argv)
{
	Rts2DevSensorPhytron device (argc, argv);
	return device.run ();
}
