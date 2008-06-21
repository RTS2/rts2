/* 
 * Driver for Phytron stepper motor controlers.
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

#include "sensord.h"
#include "../utils/rts2connserial.h"

#define CMDBUF_LEN  20

/**
 * Class for Phytron stepper motor controllers.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevSensorPhytron:public Rts2DevSensor
{
	private:
		Rts2ConnSerial *phytronDev;

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
		const char *dev;

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
	ret = phytronDev->writePort ('\002');
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot send prefix" << sendLog;
		return -1;
	}
	int len = strlen (str);
	ret = phytronDev->writePort (str, len);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot send body" << sendLog;
		return -1;
	}
	// send postfix
	ret = phytronDev->writePort ('\003');
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Cannot send postfix" << sendLog;
		return -1;
	}
	return 0;
}


int
Rts2DevSensorPhytron::readPort ()
{
	return phytronDev->readPort (cmdbuf, CMDBUF_LEN, '\003') > 0 ? 0 : -1;
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
	phytronDev = NULL;
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
	delete phytronDev;
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
	int ret;

	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	phytronDev = new Rts2ConnSerial (dev, this, BS57600, C8, NONE, 200);
	ret = phytronDev->init ();
	if (ret)
		return ret;
	phytronDev->flushPortIO ();
	
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
