/* 
 * Class for Newport MS-256 monochromator.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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
#include "connection/serial.h"
#include <errno.h>

namespace rts2sensord
{

class MS257:public Sensor
{
	public:
		MS257 (int in_argc, char **in_argv);
		virtual ~ MS257 (void);

		virtual int info ();

	protected:
		virtual int initHardware ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);

	private:
		rts2core::ValueDouble * msVer;
		rts2core::ValueDouble *wavelenght;
		rts2core::ValueInteger *slitA;
		rts2core::ValueInteger *slitB;
		rts2core::ValueInteger *slitC;
		rts2core::ValueInteger *bandPass;
		rts2core::ValueSelection *shutter;
		rts2core::ValueBool *openShutter;
		rts2core::ValueInteger *filter1;
		//  rts2core::ValueInteger *filter2;
		rts2core::ValueInteger *msteps;
		rts2core::ValueInteger *grat;

		rts2core::ConnSerial *ms257Dev;		

		void resetDevice ();
		int writePort (const char *str);
		int readPort (char **rstr, const char *cmd);

		// specialized readPort functions..
		int readPort (int &ret, const char *cmd);
		int readPort (double &ret, const char *cmd);

		template < typename T > int writeValue (const char *valueName, T val, char qStr = '=', rts2core::Value *value = NULL);
		template < typename T > int readValue (const char *valueName, T & val);

		int readRts2Value (const char *valueName, rts2core::Value * val);
		int readRts2ValueFilter (const char *valueName, rts2core::ValueInteger * val);

		const char *dev;
};

}

using namespace rts2sensord;

void MS257::resetDevice ()
{
	sleep (5);
	ms257Dev->flushPortIO ();
	logStream (MESSAGE_DEBUG) << "Device " << dev << " reseted." << sendLog;
}

int MS257::writePort (const char *str)
{
	int ret;
	ret = ms257Dev->writePort (str, strlen (str));
	if (ret)
	{
		resetDevice ();
		return -1;
	}
	ret = ms257Dev->writePort ('\r');
	if (ret)
	{
		resetDevice ();
		return -1;
	}
	return 0;
}

int MS257::readPort (char **rstr, const char *cmd)
{
	static char buf[20];
	int i = 0;
	int ret;

	ret = ms257Dev->readPort (buf, 50, '>');
	if (ret == -1)
		return ret;

	// check that we match \cr\lf[E|value]>
	if (buf[0] != '\r' || buf[1] != '\n')
	{
		logStream (MESSAGE_ERROR) << "Reply string does not start with CR LF"
			<< " (" << std::hex << (int) buf[0] << std::
			hex << (int) buf[1] << ")" << sendLog;
		return -1;
	}
	*rstr = buf + 2;
	buf[i] = '\0';
	if (**rstr == 'E')
	{
		logStream (MESSAGE_ERROR) << "Command: " << cmd << " Error: " << *rstr << sendLog;
		return -1;
	}
	return 0;
}

int MS257::readPort (int &ret, const char *cmd)
{
	int r;
	char *rstr;
	r = readPort (&rstr, cmd);
	if (r)
		return r;
	ret = atoi (rstr);
	return 0;
}

int MS257::readPort (double &ret, const char *cmd)
{
	int r;
	char *rstr;
	r = readPort (&rstr, cmd);
	if (r)
		return r;
	ret = atof (rstr);
	return 0;
}

template < typename T > int MS257::writeValue (const char *valueName, T val, char qStr, rts2core::Value *value)
{
	int ret;
	char *rstr;
	blockExposure ();
	if (value)
		logStream (MESSAGE_INFO) << "changing " << value->getName () << " from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	std::ostringstream _os;
	_os << qStr << valueName << ' ' << val;
	ret = writePort (_os.str ().c_str ());
	if (ret)
	{
		clearExposure ();
		return ret;
	}
	ret = readPort (&rstr, _os.str ().c_str ());
	if (value)
		logStream (MESSAGE_INFO) << value->getName () << " changed from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	clearExposure ();
	return ret;
}

template < typename T > int MS257::readValue (const char *valueName, T & val)
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

int MS257::readRts2Value (const char *valueName, rts2core::Value * val)
{
	int ret;
	int iret;
	double dret;
	switch (val->getValueType ())
	{
		case RTS2_VALUE_INTEGER:
			ret = readValue (valueName, iret);
			((rts2core::ValueInteger *) val)->setValueInteger (iret);
			return ret;
		case RTS2_VALUE_DOUBLE:
			ret = readValue (valueName, dret);
			((rts2core::ValueDouble *) val)->setValueDouble (dret);
			return ret;
	}
	logStream (MESSAGE_ERROR) << "Reading unknow value type " << val->getValueType () << sendLog;
	return -1;
}

int MS257::readRts2ValueFilter (const char *valueName, rts2core::ValueInteger * val)
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

MS257::MS257 (int in_argc, char **in_argv):Sensor (in_argc, in_argv)
{
	ms257Dev = NULL;
	dev = "/dev/ttyS0";

	createValue (msVer, "version", "version of MS257", false);
	createValue (wavelenght, "WAVELENG", "monochromator wavelength", true, RTS2_VALUE_WRITABLE);

	createValue (slitA, "SLIT_A", "Width of the A slit in um", true, RTS2_VALUE_WRITABLE);
	createValue (slitB, "SLIT_B", "Width of the B slit in um", true, RTS2_VALUE_WRITABLE);
	createValue (slitC, "SLIT_C", "Width of the C slit in um", true, RTS2_VALUE_WRITABLE);
	createValue (bandPass, "BANDPASS", "Automatic slit width in nm", true, RTS2_VALUE_WRITABLE);

	createValue (shutter, "shutter", "Shutter settings", false, RTS2_VALUE_WRITABLE);
	shutter->addSelVal ("SLOW");
	shutter->addSelVal ("FAST");
	shutter->addSelVal ("MANUAL");

	createValue (openShutter, "open_shutter", "open shutter - must be in slow mode", false, RTS2_VALUE_WRITABLE);
	openShutter->setValueBool (false);

	createValue (filter1, "FILT_1", "filter 1 position", true, RTS2_VALUE_WRITABLE);
	//  createValue (filter2, "FILT_2", "filter 2 position", true, RTS2_VALUE_WRITABLE);
	createValue (msteps, "MSTEPS", "Current grating position in terms of motor steps", true, RTS2_VALUE_WRITABLE);
	createValue (grat, "GRATING", "Grating position", true, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}

MS257::~MS257 ()
{
	delete ms257Dev;
}

int MS257::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == wavelenght)
	{
		return writeValue ("GW", new_value->getValueDouble (), '!', wavelenght);
	}
	if (old_value == slitA)
	{
		return writeValue ("SLITA", new_value->getValueInteger (), '!', slitA);
	}
	if (old_value == slitB)
	{
		return writeValue ("SLITB", new_value->getValueInteger (), '!', slitB);
	}
	if (old_value == slitC)
	{
		return writeValue ("SLITC", new_value->getValueInteger (), '!', slitC);
	}
	if (old_value == bandPass)
	{
		return writeValue ("BANDPASS", new_value->getValueInteger (), '=', bandPass);
	}
	if (old_value == shutter)
	{
		const char *shttypes[] = { "S", "F", "M" };
		if (new_value->getValueInteger () > 2 || new_value->getValueInteger () < 0)
			return -1;
		return writeValue ("SHTRTYPE", shttypes[new_value->getValueInteger ()], '=');
	}
	if (old_value == openShutter)
	{
		return writeValue ("SHUTTER", ((rts2core::ValueBool *) new_value)->getValueBool () ? 1 : 0, '!');
	}
	if (old_value == filter1)
	{
		return writeValue ("FILT1", new_value->getValueInteger (), '!', filter1);
	}
	/*  if (old_value == filter2)
		{
		  return writeValue ("FILT2", new_value->getValueInteger (), '!');
		} */
	if (old_value == msteps)
	{
		return writeValue ("GS", new_value->getValueInteger (), '!', msteps);
	}
	if (old_value == grat)
	{
		return writeValue ("GRAT", new_value->getValueInteger (), '!', grat);
	}
	return Sensor::setValue (old_value, new_value);
}

int MS257::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int MS257::initHardware ()
{
	int ret;

	ms257Dev = new rts2core::ConnSerial (dev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 200);
	ret = ms257Dev->init ();
	if (ret)
		return ret;

	ret = readRts2Value ("VER", msVer);
	if (ret)
	{
		// try again - this is know problem of MS257 during startup
		ms257Dev->flushPortIO ();
		sleep (1);
		ret = readRts2Value ("VER", msVer);
		if (ret)
			return -1;
	}

	// sets correct units
	ret = writeValue ("UNITS", "nn");
	if (ret)
		return ret;

	// open shutter - init
	ret = writeValue ("SHUTTER", openShutter->getValueBool () ? 1 : 0, '!');
	if (ret)
		return ret;

	return 0;
}

int MS257::info ()
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
	return Sensor::info ();
}

int main (int argc, char **argv)
{
	MS257 device = MS257 (argc, argv);
	return device.run ();
}
