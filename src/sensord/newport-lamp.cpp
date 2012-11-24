/* 
 * Class for Newport/Oriel 69907 Power source
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

#include "connection/serial.h"

namespace rts2sensord
{

/**
 * Driver for Newport Lamp power supply.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class NewportLamp: public Sensor
{
	public:
		NewportLamp (int in_argc, char **in_argv);
		virtual ~NewportLamp (void);

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int info ();

	private:
		char *lampDev;
		rts2core::ConnSerial *lampSerial;

		rts2core::ValueBool *on;

		rts2core::ValueInteger *status;
		rts2core::ValueInteger *esr;
		rts2core::ValueFloat *amps;
		rts2core::ValueInteger *volts;
		rts2core::ValueInteger *watts;
		rts2core::ValueFloat *apreset;
		rts2core::ValueInteger *ppreset;
		rts2core::ValueFloat *alim;
		rts2core::ValueInteger *plim;
		rts2core::ValueInteger *idn;
		rts2core::ValueInteger *hours;

		int writeCommand (const char *cmd);

		int writeValue (const char *valueName, float val);
		int writeValue (const char *valueName, int val);
		template < typename T > int readValue (const char *valueName, T & val);
		int getStatus (const char *valueName, rts2core::ValueInteger *val);
		int readStatus (const char *valueName, rts2core::ValueInteger *val);
};

};

using namespace rts2sensord;

NewportLamp::NewportLamp (int argc, char **argv):Sensor (argc, argv)
{
	lampSerial = NULL;

	createValue (on, "ON", "lamp on", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	createValue (status, "status", "power supply status", false, RTS2_DT_HEX);
	createValue (esr, "esr", "power supply error register", false, RTS2_DT_HEX);

	createValue (amps, "AMPS", "Amps as displayed on front panel", true);
	createValue (volts, "VOLTS", "Volts as displayed on front panel", true);
	createValue (watts, "WATTS", "Watts as displayed on front panel", true);
	createValue (apreset, "A_PRESET", "Current preset", true, RTS2_VALUE_WRITABLE);
	createValue (ppreset, "P_PRESET", "Power preset", true, RTS2_VALUE_WRITABLE);
	createValue (alim, "A_LIM", "Current limit", true);
	createValue (plim, "P_LIM", "Power limit", true);

	createValue (idn, "identification", "Identification", true);
	createValue (hours, "hours", "Lamp hours", true);

	addOption ('f', NULL, 1, "/dev/ttyS entry of the lamp serial port");
}

NewportLamp::~NewportLamp ()
{
	delete lampSerial;
}

int NewportLamp::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	int ret;
	if (old_value == on)
	{
		double now = getNow ();
		maskState (SENSOR_INPROGRESS, SENSOR_INPROGRESS, "change lamp power state", getNow (), now + 10);
		if (((rts2core::ValueBool *) new_value)->getValueBool ())
			ret = writeCommand ("START") == 0 ? 0 : -2;
		else
			ret = writeCommand ("STOP") == 0 ? 0 : -2;
		if (ret)
		{
			maskState (DEVICE_ERROR_MASK | SENSOR_INPROGRESS, DEVICE_ERROR_HW, "power change finished with error");
			return ret;
		}
		maskState (SENSOR_INPROGRESS, 0, "power change finished");
		ret = readStatus ("STB", status);
		if (ret)
			return ret;
		on->setValueBool (status->getValueInteger () & 0x80);
		sendValueAll (on);
		return ret;
	}
	if (old_value == apreset)
	{
	  	return writeValue ("A-PRESET", new_value->getValueFloat ()) == 0 ? 0 : -2;
	}
	if (old_value == ppreset)
	{
	  	return writeValue ("P-PRESET", new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	return Sensor::setValue (old_value, new_value);
}

int NewportLamp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			lampDev = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
	}
	return 0;
}

int NewportLamp::initHardware ()
{
	int ret;

	lampSerial = new rts2core::ConnSerial (lampDev, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 100);
	ret = lampSerial->init ();
	if (ret)
	  	return ret;
	
	// confirm it is working..
	ret = readStatus ("STB", status);
	if (ret)
	{
		// sometimes lamp init may fail
		lampSerial->flushPortIO ();
		sleep (1);
		ret = readStatus ("STB", status);
		if (ret)
			return ret;
	}

	return info ();
}

int NewportLamp::info ()
{
	int ret;
	ret = readStatus ("STB", status);
	if (ret)
		return ret;
	on->setValueBool (status->getValueInteger () & 0x80);
	ret = readStatus ("ESR", esr);
	if (ret)
		return ret;
	ret = readValue ("AMPS", amps);
	if (ret)
		return ret;
	ret = readValue ("VOLTS", volts);
	if (ret)
		return ret;
	ret = readValue ("WATTS", watts);
	if (ret)
		return ret;
	ret = readValue ("A-PRESET", apreset);
	if (ret)
		return ret;
	ret = readValue ("P-PRESET", ppreset);
	if (ret)
		return ret;
	ret = readValue ("A-LIM", alim);
	if (ret)
		return ret;
	ret = readValue ("P-LIM", plim);
	if (ret)
		return ret;
	ret = readValue ("IDN", idn);
	if (ret)
		return ret; 
	ret = readValue ("LAMP HRS", hours);
	if (ret)
		return ret;
	return Sensor::info (); 
}

int NewportLamp::writeCommand (const char *cmd)
{
	int ret;
	ret = lampSerial->writePort (cmd, strlen (cmd));
	if (ret < 0)
		return ret;
	ret = lampSerial->writePort ('\r');
	if (ret < 0)
	  	return ret;
	return getStatus ("ESR", esr);
}

int NewportLamp::writeValue (const char *valueName, float val)
{
	std::ostringstream _os;
	_os.precision (1);
	_os << valueName << '=' << std::fixed << val << '\r';
	int ret = lampSerial->writePort (_os.str ().c_str (), _os.str ().length ());
	if (ret)
		return ret;
	return getStatus ("ESR", esr);
}

int NewportLamp::writeValue (const char *valueName, int val)
{
	std::ostringstream _os;
	_os << valueName << '=' << val << '\r';
	int ret = lampSerial->writePort (_os.str ().c_str (), _os.str ().length ());
	if (ret)
		return ret;
	return getStatus ("ESR", esr);
}

template < typename T > int NewportLamp::readValue (const char *valueName, T & val)
{
	int ret;
	char buf[50];
	ret = lampSerial->writePort (valueName, strlen (valueName));
	if (ret < 0)
	 	return ret;
	ret = lampSerial->writePort ("?\r", 2);
	if (ret < 0)
	  	return ret;
	ret = lampSerial->readPort (buf, 50, '\r');
	if (ret == -1)
		return ret;
	return val->setValueCharArr (buf);
}

int NewportLamp::getStatus (const char *valueName, rts2core::ValueInteger *val)
{
	int ret;
	char buf[50];
	ret = lampSerial->readPort (buf, 50, '\r');
	if (ret == -1)
		return ret;
	if (strncmp (buf, valueName, strlen (valueName)) != 0)
	{
		logStream (MESSAGE_ERROR) << "status reply string does not start with status name (starts with " 
			<< buf << ", expected is " << valueName << ")" << sendLog;
		lampSerial->flushPortIO ();
		return -1;
	}
	return val->setValueInteger (strtol (buf + strlen (valueName), NULL, 16));
}

int NewportLamp::readStatus (const char *valueName, rts2core::ValueInteger *val)
{
	int ret;
	ret = lampSerial->writePort (valueName, strlen (valueName));
	if (ret < 0)
	 	return ret;
	ret = lampSerial->writePort ("?\r", 2);
	if (ret < 0)
	  	return ret;
	return getStatus (valueName, val);
}

int main (int argc, char **argv)
{
	NewportLamp device = NewportLamp (argc, argv);
	return device.run ();
}
