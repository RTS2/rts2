/* 
 * Driver for ThorLaser laser source.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/connserial.h"

#define CHAN_NUM   4

namespace rts2sensord
{

/**
 * Main class for ThorLabs laser controller. Works with version 1.02.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ThorLaser:public Sensor
{
	public:
		ThorLaser (int argc, char **argv);

		virtual int commandAuthorized (Rts2Conn * conn);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);
	private:
		char *device_file;
		rts2core::ConnSerial *laserConn;

		int currentChannel;

		Rts2ValueBool *systemEnable;

		Rts2ValueFloat *defaultTemp;

		Rts2ValueBool *enable[CHAN_NUM];
		Rts2ValueFloat *temp[CHAN_NUM];
		Rts2ValueFloat *target[CHAN_NUM];
		Rts2ValueFloat *current[CHAN_NUM];

		Rts2ValueInteger *wavelength[CHAN_NUM];
		Rts2ValueFloat *pout[CHAN_NUM];
		Rts2ValueFloat *iop[CHAN_NUM];
		Rts2ValueFloat *imon[CHAN_NUM];
		Rts2ValueFloat *ith[CHAN_NUM];
		Rts2ValueString *serialNum[CHAN_NUM];

		void checkChannel (int chan);

		int getValue (int chan, const char *name, Rts2Value *value);
		int setValue (int chan, const char *name, Rts2Value *value);

		void getSpec (int chan);
		void getSpecVal (Rts2Value *val, const char *name);

		void resetValues();
};

}

using namespace rts2sensord;

int ThorLaser::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int ThorLaser::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	laserConn = new rts2core::ConnSerial (device_file, this, rts2core::BS115200, rts2core::C8, rts2core::NONE, 10);
	ret = laserConn->init ();
	if (ret)
		return ret;
	
	laserConn->flushPortIO ();
	laserConn->setDebug (false);

	return 0;
}

int ThorLaser::info ()
{
	getValue (0, "system", systemEnable);

	for (int i = 0; i < CHAN_NUM; i++)
	{
		getValue (i, "enable", enable[i]);
		getValue (i, "temp", temp[i]);
		getValue (i, "target", target[i]);
		getValue (i, "current", current[i]);
		getSpec (i);
	}

	return Sensor::info ();
}

int ThorLaser::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	if (old_value == systemEnable)
		return setValue (currentChannel, "system", new_value) ? -2 : 0;
	for (int i = 0; i < CHAN_NUM; i++)
	{
		if (old_value == enable[i])
			return setValue (i, "enable", new_value) ? -2 : 0;
		else if (old_value == target[i])
			return setValue (i, "target", new_value) ? -2 : 0;
		else if  (old_value == current[i])
			return setValue (i, "current", new_value) ? -2 : 0;
	}
	return Sensor::setValue (old_value, new_value);
}

ThorLaser::ThorLaser (int argc, char **argv): Sensor (argc, argv)
{
	device_file = NULL;
	laserConn = NULL;

	currentChannel = -1;

	createValue (systemEnable, "system_enable", "system power state", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	createValue (defaultTemp, "default_temperature", "default laser temperature", false, RTS2_VALUE_WRITABLE);
	defaultTemp->setValueFloat (25.0);

	int i, j;
	for (i = 0, j = 1; i < CHAN_NUM; i++, j++)
	{
		char buf[50];
		sprintf (buf, "channel_%i", j);
		createValue (enable[i], buf, "channel on/off", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		sprintf (buf, "temp_%i", j);
		createValue (temp[i], buf, "[C] temperature of the channel", true);
		sprintf (buf, "target_%i", j);
		createValue (target[i], buf, "[C] target temperature of the channel", true, RTS2_VALUE_WRITABLE);
		sprintf (buf, "current_%i", j);
		createValue (current[i], buf, "current of the channel", true, RTS2_VALUE_WRITABLE);
		
		sprintf (buf, "wavel_%i", j);
		createValue (wavelength[i], buf, "[nm] wavelength of the channel", true);
		sprintf (buf, "pout_%i", j);
		createValue (pout[i], buf, "[mW] power output", true);
		sprintf (buf, "iop_%i", j);
		createValue (iop[i], buf, "[mA] recomended operating current", true);
		sprintf (buf, "imon_%i", j);
		createValue (imon[i], buf, "[mA] current of monitoring photodiode ", true);
		sprintf (buf, "ith_%i", j);
		createValue (ith[i], buf, "[mA] current threshold", true);
		sprintf (buf, "sn_%i", j);
		createValue (serialNum[i], buf, "channel unit serial number", true);
	}

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
}

int ThorLaser::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("reset"))
	{
		resetValues ();
		return 0;
	}
	return Sensor::commandAuthorized (conn);
}

void ThorLaser::checkChannel (int chan)
{
	if (chan == currentChannel)
		return;
	char buf[50];
	if (chan < 0 || chan > 3)
		throw rts2core::Error ("invalid channel specified");
	sprintf (buf, "channel=%i\r", chan + 1);
	int ret = laserConn->writeRead (buf, strlen (buf), buf, 50, '\r');
	if (ret < 0)
		throw rts2core::Error ("cannot read reply to set channel from the device");
	currentChannel = chan;
}

int ThorLaser::getValue (int chan, const char *name, Rts2Value *value)
{
	checkChannel (chan);

	char buf[50];
	int ret = sprintf (buf, "%s?\r", name);

	ret = laserConn->writeRead (buf, ret, buf, 50, '\r');
	if (ret < 0)
		return ret;
	if ((buf[0] != '<' && buf[0] != '>') || strncmp (buf + 2, name, strlen (name)))
	{
		buf[ret] = '\0';
		logStream (MESSAGE_ERROR) << "invalid reply while quering for value " << name << " : " << buf << sendLog;
		return -1;
	}
	ret = laserConn->readPort (buf, 50, '\r');
	if (ret < 0)
		return ret;
	if (value->getValueBaseType () == RTS2_VALUE_BOOL)
		((Rts2ValueBool *) value)->setValueBool (buf[0] == '1');
	else	
		value->setValueCharArr (buf);
	return 0;
}

int ThorLaser::setValue (int chan, const char *name, Rts2Value *value)
{
	checkChannel (chan);

	char buf[50];
	int ret = sprintf (buf, "%s=%s\r", name, value->getValue ());

	ret = laserConn->writeRead (buf, strlen (buf), buf, 50, '\r');
	if (ret < 0)
		return ret;
	return 0;
}

void ThorLaser::getSpec (int chan)
{
	checkChannel (chan);
	char buf[50];
	int ret = laserConn->writeRead ("specs?\r", 7, buf, 50, '\r');
	if (ret < 0)
		throw rts2core::Error ("cannot write to port, while asking for specs");
	getSpecVal (wavelength[chan], "WavelLength");
	getSpecVal (pout[chan], "POut");
	getSpecVal (iop[chan], "IOp");
	getSpecVal (imon[chan], "IMon");
	getSpecVal (ith[chan], "ITh");
	getSpecVal (serialNum[chan], "SerialNumber");
}

void ThorLaser::getSpecVal (Rts2Value *val, const char *name)
{
	int ret;
	char buf[50];
	ret = laserConn->readPort (buf, 50, '\r');
	if (ret < 0)
		throw rts2core::Error ("cannot read specs from the port");
	buf[ret - 1] = '\0';
	if (strncmp (buf, name, strlen (name)))
		throw rts2core::Error (std::string ("invalid reply - expected ") + name + ", received " + buf);
	char *ep = strchr (buf, '=');
	if (ep == NULL)
		throw rts2core::Error ("did not received = in reply to spec");
	while (*ep != '\0' && isblank (*ep))
		ep++;
	val->setValueCharArr (ep + 1);
}

void ThorLaser::resetValues ()
{
	systemEnable->setValueBool (false);
	setValue (currentChannel, "system", systemEnable);

	for (int i = 0; i < CHAN_NUM; i++)
	{
		enable[i]->setValueBool (false);
		target[i]->setValueFloat (defaultTemp->getValueFloat ());
		current[i]->setValueFloat (0);

		setValue (i, "enable", enable[i]);
		setValue (i, "target", target[i]);
		setValue (i, "current", current[i]);
	}
}

int main (int argc, char **argv)
{
	ThorLaser device (argc, argv);
	return device.run ();
}

