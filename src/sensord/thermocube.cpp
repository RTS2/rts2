/* 
 * Driver for ThermoCube 200/300/400 Thermoelectric Chiller
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#define TC_TARGET_TEMP     0x01
#define TC_CURRENT_TEMP    0x09
#define TC_FAULTS          0x08

#define TC_ON              0x40

namespace rts2sensord
{

/**
 * Driver for ThermoCube cooler/heater.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class ThermoCube:public Sensor
{
	public:
		ThermoCube (int argc, char **argv);
		virtual ~ThermoCube ();

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * newValue);

	private:
		const char *device_file;
		rts2core::ConnSerial *thermocubeConn;

		rts2core::ValueFloat *targetTemperature;
		rts2core::ValueFloat *currentTemperature;

		rts2core::ValueBool *on;

		rts2core::ValueBool *tankLevelLow;
		rts2core::ValueBool *fanFail;
		rts2core::ValueBool *pumpFail;
		rts2core::ValueBool *RTDOpen;
		rts2core::ValueBool *RTDShort;

		int setPower (bool pwr);
		int default_state;
};

}

using namespace rts2sensord;

ThermoCube::ThermoCube (int argc, char **argv):Sensor (argc, argv)
{
	device_file = NULL;

	default_state = 1;

	createValue (targetTemperature, "TARTEMP", "[C] target temperature", false, RTS2_VALUE_WRITABLE);
	createValue (currentTemperature, "CURTEMP", "[C] current temperature");

	createValue (on, "ON", "system is on", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);

	createValue (tankLevelLow, "tank_level_low", "liquid tank level is low (fault)", false);
	createValue (fanFail, "fan_fail", "fan failure", false);
	createValue (pumpFail, "pump_fail", "pump failure", false);
	createValue (RTDOpen, "rtd_open", "RTD opened (failure)", false);
	createValue (RTDShort, "rtd_short", "RTD shorted (failure)", false);

	addOption ('f', NULL, 1, "serial port on which is ThermoCube connected");
	addOption ('s', NULL, 1, "default state of the device (default on = 1, off = 0)");
}

ThermoCube::~ThermoCube ()
{
	delete thermocubeConn;
}

void ThermoCube::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_EVENING:
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
		case SERVERD_MORNING:
			{
				switch (new_state & SERVERD_ONOFF_MASK)
				{
					case SERVERD_ON:
					case SERVERD_STANDBY:
						if (on->getValueBool () == false)
							setPower (true);
						break;
					default:
						if (on->getValueBool () == true)
							setPower (false);
						break;
				}
			}
			break;
		default:
			if (on->getValueBool () == true)
				setPower (false);
			break;
	}
	Sensor::changeMasterState (old_state, new_state);
}

int ThermoCube::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case 's':
			default_state = atoi (optarg);
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int ThermoCube::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}
	
	thermocubeConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 10);
	int ret = thermocubeConn->init ();
	if (ret)
		return ret;

	thermocubeConn->setDebug (getDebug ());

	setPower (default_state);

	return 0;
}

int ThermoCube::info ()
{
	char cmd = 0x80;
	unsigned char buf[2];
	if (on->getValueBool ())
		cmd |= TC_ON;

	// get target temperature
	int ret = thermocubeConn->writePort (cmd | TC_TARGET_TEMP);
	if (ret)
		return ret;
	ret = thermocubeConn->readPort ((char *) buf, 2);
	if (ret != 2)
		return -1;
	int t = buf[0] + buf[1] * 256;
	targetTemperature->setValueFloat (fahrenheitToCelsius (t / 10.0));

	ret = thermocubeConn->writePort (cmd | TC_CURRENT_TEMP);
	if (ret)
		return ret;
	ret = thermocubeConn->readPort ((char *) buf, 2);
	if (ret != 2)
		return -1;
	t = buf[0] + buf[1] * 256;
	currentTemperature->setValueFloat (fahrenheitToCelsius (t / 10.0));

	ret = thermocubeConn->writePort (cmd | TC_FAULTS);
	if (ret)
		return ret;
	ret = thermocubeConn->readPort ((char *) buf, 1);
	if (ret != 1)
		return -1;

	tankLevelLow->setValueBool (buf[0] & 0x01);
	fanFail->setValueBool (buf[0] & 0x02);
	pumpFail->setValueBool (buf[0] & 0x08);
	RTDOpen->setValueBool (buf[0] & 0x10);
	RTDShort->setValueBool (buf[0] & 0x20);

	return Sensor::info ();
}

int ThermoCube::setValue (rts2core::Value * old_value, rts2core::Value * newValue)
{
	if (old_value == targetTemperature)
	{
		unsigned char cmd[3]; 
		cmd[0] = 0xA0 | TC_TARGET_TEMP;
		if (on->getValueBool ())
			cmd[0] |= TC_ON;
		int16_t t = celsiusToFahrenheit (newValue->getValueFloat ()) * 10;
		cmd[1] = t % 256;
		cmd[2] = t / 256;
		return thermocubeConn->writePort ((char *) cmd, 3) ? -2 : 0;
	}
	else if (old_value == on)
	{
		return setPower (((rts2core::ValueBool *) newValue)->getValueBool ()) ? -2 : 0;
	}
	return Sensor::setValue (old_value, newValue);
}

int ThermoCube::setPower (bool pwr)
{
	unsigned char cmd[3]; 
	cmd[0] = 0xA0 | TC_TARGET_TEMP;
	if (pwr)
		cmd[0] |= TC_ON;
	int16_t t = celsiusToFahrenheit (targetTemperature->getValueFloat ()) * 10;
	cmd[1] = t % 256;
	cmd[2] = t / 256;
	int ret = thermocubeConn->writePort ((char *) cmd, 3);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot switch cooling to " << (pwr ? "on" : "off") << sendLog;
		return ret;
	}
	on->setValueBool (pwr);
	sendValueAll (on);
	logStream (MESSAGE_INFO) << "switched ThermoCube cooling to " << (pwr ? "on" : "off") << sendLog;
	return 0;
}

int main (int argc, char **argv)
{
	ThermoCube device (argc, argv);
	return device.run ();
}
