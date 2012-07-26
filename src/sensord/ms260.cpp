/* 
 * Class for Newport MS-260 monochromator.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

class MS260:public Sensor
{
	public:
		MS260 (int argc, char **argv);
		virtual ~ MS260 (void);

		virtual int info ();

	protected:
		virtual int initHardware ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);

	private:
		rts2core::ValueString *model;

		rts2core::ValueDouble *wavelenght;
		rts2core::ValueInteger *slit1;
		rts2core::ValueInteger *slit2;
		rts2core::ValueInteger *slit3;
		rts2core::ValueInteger *bandPass;
		rts2core::ValueBool *openShutter;
		rts2core::ValueInteger *filter;
		//  rts2core::ValueInteger *filter2;
		rts2core::ValueInteger *grat;
		rts2core::ValueDouble *gratfactor;

		rts2core::ConnSerial *ms260Dev;		

		void resetDevice ();
		int writePort (const char *str);
		int readPort (char *buf, int blen);

		// specialized readPort functions..
		int readPort (int &ret);
		int readPort (double &ret);

		template < typename T > int writeValue (const char *valueName, T val, rts2core::Value *value = NULL);
		template < typename T > int readValue (const char *valueName, T & val);

		int readRts2Value (const char *valueName, rts2core::Value * val);

		const char *dev;
};

}

using namespace rts2sensord;

void MS260::resetDevice ()
{
	sleep (5);
	ms260Dev->flushPortIO ();
	logStream (MESSAGE_DEBUG) << "Device " << dev << " reseted." << sendLog;
}

int MS260::writePort (const char *str)
{
	int ret;
	ret = ms260Dev->writePort (str, strlen (str));
	if (ret)
	{
		resetDevice ();
		return -1;
	}
	ret = ms260Dev->writePort ("\r\n", 2);
	if (ret)
	{
		resetDevice ();
		return -1;
	}
	return 0;
}

int MS260::readPort (char *buf, int blen)
{
	int ret;

	ret = ms260Dev->readPort (buf, blen, "\r\n");
	if (ret == -1)
		return ret;
	return 0;
}

int MS260::readPort (int &ret)
{
	int r;
	char buf[20];
	r = readPort (buf, 20);
	if (r)
		return r;
	ret = atoi (buf);
	return 0;
}

int MS260::readPort (double &ret)
{
	int r;
	char buf[20];
	r = readPort (buf, 20);
	if (r)
		return r;
	ret = atof (buf);
	return 0;
}

template < typename T > int MS260::writeValue (const char *valueName, T val, rts2core::Value *value)
{
	int ret;
	char buf[20];
	blockExposure ();
	if (value)
		logStream (MESSAGE_INFO) << "changing " << value->getName () << " from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	std::ostringstream _os;
	_os << valueName << ' ' << val;
	ret = writePort (_os.str ().c_str ());
	if (ret)
	{
		clearExposure ();
		return ret;
	}
	ret = readPort (buf, 20);
	if (value)
		logStream (MESSAGE_INFO) << value->getName () << " changed from " << value->getDisplayValue () << " to " << val << "." << sendLog;
	clearExposure ();
	return ret;
}

template < typename T > int MS260::readValue (const char *valueName, T & val)
{
	int ret;
	char buf[strlen (valueName + 2)];
	strcpy (buf, valueName);
	buf[strlen (valueName)] = '?';
	buf[strlen (valueName) + 1] = '\0';
	ret = writePort (buf);
	if (ret)
		return ret;
	char rbuf[50];
	ret = readPort (rbuf, 50);
	if (ret)
		return ret;
	ret = readPort (val);
	return ret;
}

int MS260::readRts2Value (const char *valueName, rts2core::Value * val)
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
	logStream (MESSAGE_ERROR) << "unsupported value type" << val->getValueType () << sendLog;
	return -1;
}

MS260::MS260 (int argc, char **argv):Sensor (argc, argv)
{
	ms260Dev = NULL;
	dev = "/dev/ttyS0";

	createValue (model, "model", "monochromator model", false);

	createValue (wavelenght, "WAVELENG", "monochromator wavelength", true, RTS2_VALUE_WRITABLE);

	createValue (slit1, "SLIT_1", "Width of the A slit in um", true, RTS2_VALUE_WRITABLE);
	createValue (slit2, "SLIT_2", "Width of the B slit in um", true, RTS2_VALUE_WRITABLE);
	createValue (slit3, "SLIT_3", "Width of the C slit in um", true, RTS2_VALUE_WRITABLE);
	createValue (bandPass, "BANDPASS", "Automatic slit width in nm", true, RTS2_VALUE_WRITABLE);

	createValue (openShutter, "SHUTTER", "open / close shutter", false, RTS2_VALUE_WRITABLE);
	openShutter->setValueBool (false);

	createValue (filter, "FILTER", "filter position", true, RTS2_VALUE_WRITABLE);
	createValue (grat, "GRATING", "Grating position", true, RTS2_VALUE_WRITABLE);
	createValue (gratfactor, "GRAT1FACTOR", "grating factor", true, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}

MS260::~MS260 ()
{
	delete ms260Dev;
}

int MS260::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == wavelenght)
	{
		return writeValue ("GOWAVE", new_value->getValueDouble (), wavelenght);
	}
	if (old_value == slit1)
	{
		return writeValue ("SLIT1MICRONS", new_value->getValueInteger (), slit1);
	}
	if (old_value == slit2)
	{
		return writeValue ("SLIT2MICRONS", new_value->getValueInteger (), slit2);
	}
	if (old_value == slit3)
	{
		return writeValue ("SLIT3MICRONS", new_value->getValueInteger (), slit3);
	}
	if (old_value == bandPass)
	{
		return writeValue ("BANDPASS", new_value->getValueInteger (), bandPass);
	}
	if (old_value == openShutter)
	{
		return writeValue ("SHUTTER", ((rts2core::ValueBool *) new_value)->getValueBool () ? 'O' : 'C');
	}
	if (old_value == filter)
	{
		return writeValue ("FILTER", new_value->getValueInteger (), filter);
	}
	if (old_value == grat)
	{
		return writeValue ("GRAT", new_value->getValueInteger (), grat);
	}
	return Sensor::setValue (old_value, new_value);
}

int MS260::processOption (int in_opt)
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

int MS260::initHardware ()
{
	int ret;

	ms260Dev = new rts2core::ConnSerial (dev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 200);
	ms260Dev->setDebug (getDebug ());
	ret = ms260Dev->init ();
	if (ret)
		return ret;

	writePort ("INFO?");
	char buf[50];
	ret = readPort (buf, 50);
	if (ret)
		return ret;
	ret = readPort (buf, 50);
	if (ret)
		return ret;
	model->setValueCharArr (buf);

	// sets correct units
	ret = writeValue ("UNITS", "NM");
	if (ret)
		return ret;

	// open shutter - init
	ret = writeValue ("SHUTTER", openShutter->getValueBool () ? 1 : 0);
	if (ret)
		return ret;
	return 0;
}

int MS260::info ()
{
	int ret;
	char buf[50];
	ret = readRts2Value ("WAVE", wavelenght);
	if (ret)
		return ret;
	ret = writePort ("SHUTTER?");
	if (ret)
		return ret;
	ret = readPort (buf, 50);
	if (ret)
		return ret;
	ret = readPort (buf, 50);
	if (ret)
		return ret;
	openShutter->setValueBool (buf[0] == 'O');
	ret = readRts2Value ("SLIT1MICRONS", slit1);
	if (ret)
		return ret;
	ret = readRts2Value ("SLIT2MICRONS", slit2);
	if (ret)
		return ret;
	ret = readRts2Value ("SLIT3MICRONS", slit3);
	if (ret)
		return ret;
	ret = readRts2Value ("BANDPASS", bandPass);
	if (ret)
		return ret;
	ret = readRts2Value ("FILTER", filter);
	if (ret)
		return ret;
	ret = writePort ("GRAT?");
	if (ret)
		return ret;
	ret = readPort (buf, 50);
	if (ret)
		return ret;
	ret = readPort (buf, 50);
	if (ret)
		return ret;
	grat->setValueInteger (buf[0] - '0');
	ret = readRts2Value ("GRAT1FACTOR", gratfactor);
	if (ret)
		return ret;
	return Sensor::info ();
}

int main (int argc, char **argv)
{
	MS260 device = MS260 (argc, argv);
	return device.run ();
}
