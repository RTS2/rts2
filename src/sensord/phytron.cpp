/* 
 * Driver for Phytron stepper motor controlers.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#define CMDBUF_LEN  20

namespace rts2sensord
{

/**
 * Class for Phytron stepper motor controllers.
 *
 * MiniLog is documented in: ftp://ftp.phytron.de/manuals/ixe-sls/minilog-ixe-sam-gb.pdf
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Phytron:public Sensor
{
	public:
		Phytron (int in_argc, char **in_argv);
		virtual ~ Phytron (void);

		virtual int init ();
		virtual int info ();

		virtual int commandAuthorized (rts2core::Rts2Connection * conn);

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);

	private:
		rts2core::ConnSerial *phytronDev;

		int writePort (const char *str);
		int readPort ();

		char cmdbuf[CMDBUF_LEN];

		rts2core::ValueInteger* axis0;
		rts2core::ValueInteger* runFreq;
		rts2core::ValueInteger* systemStatus;
		rts2core::ValueInteger* systemStatusExtended;
		rts2core::ValueBool* powerOffIdle;
		rts2core::ValueSelection* phytronParams[47];
		rts2core::ValueBool* power;

		int readValue (const char *cmd);
		int readValue (int ax, int reg, rts2core::ValueInteger *val);
		int readAxis ();
		int setValue (int ax, int reg, rts2core::ValueInteger *val);
		int setPower (bool on);
		int setAxis (int new_val);
		int updateStatus (bool sendValues);
		int sendReset ();
		const char *dev;
};

};

using namespace rts2sensord;

int Phytron::writePort (const char *str)
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

int Phytron::readPort ()
{
	return phytronDev->readPort (cmdbuf, CMDBUF_LEN, '\003') > 0 ? 0 : -1;
}

int Phytron::readValue (const char *cmd)
{
	int ret = writePort (cmd);
	if (ret < 0)
		return ret;

	ret = readPort ();
	if (ret)
	{
		phytronDev->flushPortIO ();
		return ret;
	}

	if (cmdbuf[0] != '\002' || cmdbuf[1] != '\006')
	{
		logStream (MESSAGE_ERROR) << "Invalid header" << sendLog;
		return -1;
	}
	return 0;
}

int Phytron::readValue (int ax, int reg, rts2core::ValueInteger *val)
{
	char buf[50];
	snprintf (buf, 50, "%02iP%2iR", ax, reg);
	int ret = readValue (buf);
	if (ret)
		return ret;
	int ival = atoi (cmdbuf + 2);
	val->setValueInteger (ival);
	return 0;
}

int Phytron::readAxis ()
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

int Phytron::setValue (int ax, int reg, rts2core::ValueInteger *val)
{
	char buf[50];
	snprintf (buf, 50, "%02iP%2iS%i", ax, reg, val->getValueInteger ());
	int ret = writePort (buf);
	if (ret < 0)
		return ret;
	ret = readPort ();
	return ret;
}

int Phytron::setPower (bool on)
{
	int ret = writePort (on ? "0XMA" : "0XMD");
	if (ret < 0)
		return ret;
	ret = readPort ();
	// if updated to on..
	if (on)
		ret = sendReset ();
	updateStatus (true);
	return ret;
}

int Phytron::setAxis (int new_val)
{
	int ret;
	if (powerOffIdle->getValueBool () == true)
	{
		setPower (true);
		power->setValueBool (true);
		sendValueAll (power);
	}
	sprintf (cmdbuf, "01A%i", new_val);
	logStream (MESSAGE_DEBUG) << "commanding axis to " << new_val << sendLog;
	ret = writePort (cmdbuf);
	if (ret)
		return ret;
	ret = readPort ();
	if (ret)
		return ret;
	do
	{
		ret = readAxis ();
		sendValueAll (axis0);
	}
	while (axis0->getValueInteger () != new_val);
	logStream (MESSAGE_DEBUG) << "axis reached " << new_val << sendLog;
	if (powerOffIdle->getValueBool () == true)
	{
		setPower (false);
		power->setValueBool (false);
		sendValueAll (power);
	}
	return 0;
}

int Phytron::sendReset ()
{
	return readValue ("0XC");
}

int Phytron::updateStatus (bool sendValues)
{
	int ret = readValue ("0SB");
	if (ret)
		return ret;
	systemStatus->setValueInteger (strtol (cmdbuf + 2, NULL, 2));

	ret = readValue ("0SE");
	if (ret)
		return ret;
	cmdbuf[6] = '\0';
	systemStatusExtended->setValueInteger (strtol (cmdbuf + 2, NULL, 16));
	power->setValueBool (systemStatusExtended->getValueInteger () & 0x08);
	if (sendValues)
	{
		sendValueAll (systemStatus);
		sendValueAll (systemStatusExtended);
		sendValueAll (power);
	}
	return 0;
}

Phytron::Phytron (int argc, char **argv):Sensor (argc, argv)
{
	phytronDev = NULL;
	dev = "/dev/ttyS0";

	createValue (runFreq, "RUNFREQ", "current run frequency", true, RTS2_VALUE_WRITABLE);
	createValue (axis0, "CURPOS", "current arm position", true, RTS2_VWHEN_RECORD_CHANGE | RTS2_VALUE_WRITABLE, 0);
	createValue (systemStatus, "SB", "system status", false, RTS2_DT_HEX);
	createValue (systemStatusExtended, "SE", "system status extended", false, RTS2_DT_HEX);

	createValue (power, "power", "axis power state", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (powerOffIdle, "power_off_idle", "power off motor if its not moving", false, RTS2_VALUE_WRITABLE);
	powerOffIdle->setValueBool (true);

	// create phytron params
	/*	createValue (phytronParams[0], "P01", "Type of movement", false);
		((rts2core::ValueSelection *) phytronParams[0])->addSelVal ("rotational");
		((rts2core::ValueSelection *) phytronParams[0])->addSelVal ("linear");

		createValue ((rts2core::ValueSelection *)phytronParams[1], "P02", "Measuring units of movement", false);
		((rts2core::ValueSelection *) phytronParams[1])->addSelVal ("step");
		((rts2core::ValueSelection *) phytronParams[1])->addSelVal ("mm");
		((rts2core::ValueSelection *) phytronParams[1])->addSelVal ("inch");
		((rts2core::ValueSelection *) phytronParams[1])->addSelVal ("degree"); */

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");
}

Phytron::~Phytron (void)
{
	delete phytronDev;
}

int Phytron::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == axis0)
		return setAxis (new_value->getValueInteger ());
	if (old_value == runFreq)
		return setValue (1, 14, (rts2core::ValueInteger *)new_value);
	if (old_value == power)
		return setPower (((rts2core::ValueBool *) new_value)->getValueBool ()) < 0 ? -2 : 0;
	return Sensor::setValue (old_value, new_value);
}

int Phytron::processOption (int in_opt)
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

int Phytron::init ()
{
	int ret;

	ret = Sensor::init ();
	if (ret)
		return ret;

	phytronDev = new rts2core::ConnSerial (dev, this, rts2core::BS57600, rts2core::C8, rts2core::NONE, 200);
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

int Phytron::info ()
{
	int ret;
	ret = readAxis ();
	if (ret)
		return ret;
	ret = updateStatus (false);
	if (ret)
		return ret;
	return Sensor::info ();
}

int Phytron::commandAuthorized (rts2core::Rts2Connection * conn)
{
	if (conn->isCommand ("reset"))
	{
		if (sendReset ())
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "hardware error - cannot reset X axe");
			return -1;
		}
		return 0;
	}
	return Sensor::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	Phytron device (argc, argv);
	return device.run ();
}
