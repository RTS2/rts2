#include "sensord.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

class Rts2DevSensorMS257:public Rts2DevSensor
{
	private:
		Rts2ValueDouble * msVer;
		Rts2ValueDouble *wavelenght;
		Rts2ValueInteger *slitA;
		Rts2ValueInteger *slitB;
		Rts2ValueInteger *slitC;
		Rts2ValueInteger *bandPass;
		Rts2ValueSelection *shutter;
		Rts2ValueInteger *filter1;
		//  Rts2ValueInteger *filter2;
		Rts2ValueInteger *msteps;
		Rts2ValueInteger *grat;

		int dev_port;

		void resetDevice ();
		int writePort (const char *str);
		int readPort (char **rstr, const char *cmd);

		// specialized readPort functions..
		int readPort (int &ret, const char *cmd);
		int readPort (double &ret, const char *cmd);

		template < typename T > int writeValue (const char *valueName, T val,
			char qStr = '=');
		template < typename T > int readValue (const char *valueName, T & val);

		int readRts2Value (const char *valueName, Rts2Value * val);
		int readRts2ValueFilter (const char *valueName, Rts2ValueInteger * val);

		char *dev;
	protected:
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
		virtual int processOption (int in_opt);

	public:
		Rts2DevSensorMS257 (int in_argc, char **in_argv);
		virtual ~ Rts2DevSensorMS257 (void);

		virtual int init ();
		virtual int info ();
};

void
Rts2DevSensorMS257::resetDevice ()
{
	sleep (5);
	tcflush (dev_port, TCIOFLUSH);
	logStream (MESSAGE_DEBUG) << "Device " << dev << " reseted." << sendLog;
}


int
Rts2DevSensorMS257::writePort (const char *str)
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
Rts2DevSensorMS257::readPort (char **rstr, const char *cmd)
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
Rts2DevSensorMS257::readPort (int &ret, const char *cmd)
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
Rts2DevSensorMS257::readPort (double &ret, const char *cmd)
{
	int r;
	char *rstr;
	r = readPort (&rstr, cmd);
	if (r)
		return r;
	ret = atof (rstr);
	return 0;
}


template < typename T > int
Rts2DevSensorMS257::writeValue (const char *valueName, T val, char qStr)
{
	int ret;
	char *rstr;
	blockExposure ();
	std::ostringstream _os;
	_os << qStr << valueName << ' ' << val;
	ret = writePort (_os.str ().c_str ());
	if (ret)
	{
		clearExposure ();
		return ret;
	}
	ret = readPort (&rstr, _os.str ().c_str ());
	clearExposure ();
	return ret;
}


template < typename T > int
Rts2DevSensorMS257::readValue (const char *valueName, T & val)
{
	int ret;
	char buf[strlen (valueName + 1)];
	*buf = '?';
	strcpy (buf + 1, valueName);
	ret = writePort (buf);
	if (ret)
		return ret;
	ret = readPort (val, buf);
	return ret;
}


int
Rts2DevSensorMS257::readRts2Value (const char *valueName, Rts2Value * val)
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
Rts2DevSensorMS257::readRts2ValueFilter (const char *valueName,
Rts2ValueInteger * val)
{
	int ret;
	char *cval;
	char **pval = &cval;
	int iret;
	ret = readValue (valueName, pval);
	if (ret)
		return ret;
	if ((*cval != 'M' && *cval != 'A') || cval[1] != ':')
	{
		logStream (MESSAGE_ERROR) << "Unknow filter state: " << cval << sendLog;
		return -1;
	}
	iret = atoi (cval + 2);
	val->setValueInteger (iret);
	return 0;
}


Rts2DevSensorMS257::Rts2DevSensorMS257 (int in_argc, char **in_argv):
Rts2DevSensor (in_argc, in_argv)
{
	dev_port = -1;
	dev = "/dev/ttyS0";

	createValue (msVer, "version", "version of MS257", false);
	createValue (wavelenght, "WAVELENG", "monochromator wavelength", true);

	createValue (slitA, "SLIT_A", "Width of the A slit in um", true);
	createValue (slitB, "SLIT_B", "Width of the B slit in um", true);
	createValue (slitC, "SLIT_C", "Width of the C slit in um", true);
	createValue (bandPass, "BANDPASS", "Automatic slit width in nm", true);

	createValue (shutter, "shutter", "Shutter settings", false);
	shutter->addSelVal ("SLOW");
	shutter->addSelVal ("FAST");
	shutter->addSelVal ("MANUAL");

	createValue (filter1, "FILT_1", "filter 1 position", true);
	//  createValue (filter2, "FILT_2", "filter 2 position", true);
	createValue (msteps, "MSTEPS",
		"Current grating position in terms of motor steps", true);
	createValue (grat, "GRATING", "Grating position", true);

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}


Rts2DevSensorMS257::~Rts2DevSensorMS257 ()
{
	close (dev_port);
}


int
Rts2DevSensorMS257::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == wavelenght)
	{
		return writeValue ("GW", new_value->getValueDouble (), '!');
	}
	if (old_value == slitA)
	{
		return writeValue ("SLITA", new_value->getValueInteger (), '!');
	}
	if (old_value == slitB)
	{
		return writeValue ("SLITB", new_value->getValueInteger (), '!');
	}
	if (old_value == slitC)
	{
		return writeValue ("SLITC", new_value->getValueInteger (), '!');
	}
	if (old_value == bandPass)
	{
		return writeValue ("BANDPASS", new_value->getValueInteger (), '=');
	}
	if (old_value == shutter)
	{
		char *shttypes[] = { "S", "F", "M" };
		if (new_value->getValueInteger () > 2
			|| new_value->getValueInteger () < 0)
			return -1;
		return writeValue ("SHTRTYPE", shttypes[new_value->getValueInteger ()],
			'=');
	}
	if (old_value == filter1)
	{
		return writeValue ("FILT1", new_value->getValueInteger (), '!');
	}
	/*  if (old_value == filter2)
		{
		  return writeValue ("FILT2", new_value->getValueInteger (), '!');
		} */
	if (old_value == msteps)
	{
		return writeValue ("GS", new_value->getValueInteger (), '!');
	}
	if (old_value == grat)
	{
		return writeValue ("GRAT", new_value->getValueInteger (), '!');
	}
	return Rts2DevSensor::setValue (old_value, new_value);
}


int
Rts2DevSensorMS257::processOption (int in_opt)
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
Rts2DevSensorMS257::init ()
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
	if (cfsetospeed (&term_options, B9600) < 0
		|| cfsetispeed (&term_options, B9600) < 0)
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

	tcflush (dev_port, TCIOFLUSH);

	/*
	 * Set the new options for the port...
	 */
	if (tcsetattr (dev_port, TCSANOW, &term_options))
	{
		logStream (MESSAGE_ERROR) << "filter ifw init tcsetattr " <<
			strerror (errno) << sendLog;
		return -1;
	}

	// char *rstr;

	// try to read ready sign first..
	//readPort (&rstr);

	ret = readRts2Value ("VER", msVer);
	if (ret)
		return -1;

	// sets correct units
	ret = writeValue ("UNITS", "nn");
	if (ret)
		return ret;

	// open shutter - init
	ret = writeValue ("SHUTTER", 0, '!');
	if (ret)
		return ret;

	return 0;
}


int
Rts2DevSensorMS257::info ()
{
	int ret;
	ret = readRts2Value ("PW", wavelenght);
	if (ret)
		return ret;
	ret = readRts2Value ("SLITA", slitA);
	if (ret)
		return ret;
	ret = readRts2Value ("SLITB", slitB);
	if (ret)
		return ret;
	ret = readRts2Value ("SLITC", slitC);
	if (ret)
		return ret;
	ret = readRts2Value ("BANDPASS", bandPass);
	if (ret)
		return ret;
	char *shttype;
	char **shttype_p = &shttype;
	ret = readValue ("SHTRTYPE", shttype_p);
	switch (**shttype_p)
	{
		case 'S':
			shutter->setValueInteger (0);
			break;
		case 'F':
			shutter->setValueInteger (1);
			break;
		case 'M':
			shutter->setValueInteger (2);
			break;
		default:
			logStream (MESSAGE_ERROR) << "Unknow shutter value: " << *shttype <<
				sendLog;
			return -1;
	}
	ret = readRts2ValueFilter ("FILT1", filter1);
	if (ret)
		return ret;
	/*  ret = readRts2Value ("FILT2", filter2);
	  if (ret)
		return ret;
	*/
	ret = readRts2Value ("PS", msteps);
	if (ret)
		return ret;
	usleep (100);
	ret = readRts2ValueFilter ("GRAT", grat);
	if (ret)
		return ret;
	return Rts2DevSensor::info ();
}


int
main (int argc, char **argv)
{
	Rts2DevSensorMS257 device = Rts2DevSensorMS257 (argc, argv);
	return device.run ();
}
