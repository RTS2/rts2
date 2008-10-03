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

#include "../utils/rts2connserial.h"

/**
 * Driver for Newport Lamp power supply.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevSensorNewportLamp: public Rts2DevSensor
{
	private:
		char *lampDev;
		Rts2ConnSerial *lampSerial;

		Rts2ValueBool *on;

		Rts2ValueInteger *status;
		Rts2ValueInteger *esr;
		Rts2ValueFloat *amps;
		Rts2ValueInteger *volts;
		Rts2ValueInteger *watts;
		Rts2ValueFloat *alim;
		Rts2ValueInteger *plim;
		Rts2ValueString *idn;
		Rts2ValueInteger *hours;

		int writeCommand (const char *cmd);

		template < typename T > int writeValue (const char *valueName, T val);
		template < typename T > int readValue (const char *valueName, T & val);

	protected:
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();

	public:
		Rts2DevSensorNewportLamp (int in_argc, char **in_argv);
		virtual ~Rts2DevSensorNewportLamp (void);
};


int
Rts2DevSensorNewportLamp::writeCommand (const char *cmd)
{
	int ret;
	ret = lampSerial->writePort (cmd, strlen (cmd));
	if (ret < 0)
		return ret;
	ret = lampSerial->writePort ('\r');
	if (ret < 0)
	  	return ret;
	return 0;
}

template < typename T > int
Rts2DevSensorNewportLamp::writeValue (const char *valueName, T val)
{
	return 0;
}


template < typename T > int
Rts2DevSensorNewportLamp::readValue (const char *valueName, T & val)
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


Rts2DevSensorNewportLamp::Rts2DevSensorNewportLamp (int in_argc, char **in_argv)
:Rts2DevSensor (in_argc, in_argv)
{
	lampSerial = NULL;

	createValue (on, "ON", "lamp on", true);

	createValue (status, "status", "power supply status", false, RTS2_DT_HEX);
	createValue (esr, "esr", "power supply error register", false, RTS2_DT_HEX);

	createValue (amps, "AMPS", "Amps as displayed on front panel", true);
	createValue (volts, "VOLTS", "Volts as displayed on front panel", true);
	createValue (watts, "WATTS", "Watts as displayed on front panel", true);
	createValue (alim, "A_LIM", "Current limit", true);
	createValue (plim, "P_LIM", "Power limit", true);

	createValue (idn, "identification", "Identification", true);
	createValue (hours, "hours", "Lamp hours", true);

	addOption ('f', NULL, 1, "/dev/ttyS entry of the lamp serial port");
}


Rts2DevSensorNewportLamp::~Rts2DevSensorNewportLamp ()
{
	delete lampSerial;
}

int
Rts2DevSensorNewportLamp::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == on)
	{
		if (((Rts2ValueBool *) new_value)->getValueBool ())
			return writeCommand ("START") == 0 ? 0 : -2;
		else
			return writeCommand ("STOP") == 0 ? 0 : -2;
	}
	if (old_value == amps)
	{
	  	return writeValue ("A-PRESET", new_value->getValueDouble ()) == 0 ? 0 : -2;
	}
	return Rts2DevSensor::setValue (old_value, new_value);
}


int
Rts2DevSensorNewportLamp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			lampDev = optarg;
			break;
		default:
			return Rts2DevSensor::processOption (in_opt);
	}
	return 0;
}

int
Rts2DevSensorNewportLamp::init ()
{
	int ret;

	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;
	
	lampSerial = new Rts2ConnSerial (lampDev, this, BS9600, C8, NONE, 40);
	ret = lampSerial->init ();
	if (ret)
	  	return ret;
	
	// confir it is working..
	return info ();
}


int
Rts2DevSensorNewportLamp::info ()
{
	int ret;
	ret = readValue ("AMPS", amps);
	if (ret)
		return ret;
	ret = readValue ("VOLTS", volts);
	if (ret)
		return ret;
	ret = readValue ("WATTS", watts);
	if (ret)
		return ret;
	ret = readValue ("LAMP HRS", hours);
	if (ret)
		return ret;
	return Rts2DevSensor::info (); 
}


int
main (int argc, char **argv)
{
	Rts2DevSensorNewportLamp device = Rts2DevSensorNewportLamp (argc, argv);
	return device.run ();
}
