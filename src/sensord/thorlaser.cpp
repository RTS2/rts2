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

class ThorLaser:public Sensor
{
	public:
		ThorLaser (int argc, char **argv);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);
	private:
		char *device_file;
		rts2core::ConnSerial *laserConn;

		int currentChannel;

		Rts2ValueBool *enable[CHAN_NUM];
		Rts2ValueFloat *temp[CHAN_NUM];
		Rts2ValueFloat *current[CHAN_NUM];

		void checkChannel (int chan);

		int getValue (int chan, const char *name, Rts2Value *value);
		int setValue (int chan, const char *name, Rts2Value *value);
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
	laserConn->setDebug (true);

	return 0;
}

int ThorLaser::info ()
{
	for (int i = 0; i < CHAN_NUM; i++)
	{
		getValue (i, "enable", enable[i]);
		getValue (i, "temp", temp[i]);
		getValue (i, "current", current[i]);
	}

	return Sensor::info ();
}

int ThorLaser::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	for (int i = 0; i < CHAN_NUM; i++)
	{
		if (old_value == enable[i])
			return setValue (i, "enable", new_value) ? -2 : 0;
		else if (old_value == temp[i])
			return setValue (i, "temp", new_value) ? -2 : 0;
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

	for (int i = 0; i < CHAN_NUM; i++)
	{
		char buf[50];
		sprintf (buf, "channel_%i", i + 1);
		createValue (enable[i], buf, "channel on/off", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		sprintf (buf, "temp_%i", i);
		createValue (temp[i], buf, "temperature of the channel", true, RTS2_VALUE_WRITABLE);
		sprintf (buf, "current_%i", i);
		createValue (current[i], buf, "current of the channel", true, RTS2_VALUE_WRITABLE);
	}

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSB for ThorLaser USB serial connection");
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
	if (buf[0] != '<' || strncmp (buf + 2, name, strlen (name)))
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

int main (int argc, char **argv)
{
	ThorLaser device (argc, argv);
	return device.run ();
}

