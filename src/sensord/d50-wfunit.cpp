/*
 * Driver for arduino unit, located near the WF camera of the D50 telescope.
 * Contains external shutter/cover of the WF camera and two temparature/humidity
 * sensors, measuring values inside & outside of the main (NF) optical tube.
 *
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2014 Martin Jelinek
 * Copyright (C) 2019 Jan Strobl
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

/*
Commands: oc
o : open shutter
c : close shutter
s : get status

example output of any command (including unknown):
C 28.60 50.59 28.63 51.53 //shutter_state t_out h_out t_in h_in
*/

#include "sensord.h"

#include "connection/serial.h"

namespace rts2sensord
{

/**
 * D50WFUnit board control various I/O.
 *
 * @author Petr Kubanek <petr@kubanek.net>, Martin Jelinek <mates@iaa.es>, Jan Strobl
 */
class D50WFUnit:public Sensor
{
	public:
		D50WFUnit (int argc, char **argv);
		virtual ~D50WFUnit ();
		virtual int scriptEnds ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		const char *device_file;
		rts2core::ConnSerial *wfunitConn;

		rts2core::ValueSelection *wfShutter;
		rts2core::ValueDoubleStat *otaTempIn;
		rts2core::ValueDoubleStat *otaHumIn;
		rts2core::ValueDoubleStat *otaTempOut;
		rts2core::ValueDoubleStat *otaHumOut;

		rts2core::ValueInteger *numVal;

		int wfunitCommand (char c);
};

}

using namespace rts2sensord;

D50WFUnit::D50WFUnit (int argc, char **argv): Sensor (argc, argv)
{
	device_file = "/dev/wfunit"; // default value
	wfunitConn = NULL;

	createValue (numVal, "num_stat", "number of measurements for temp/hum values", false, RTS2_VALUE_WRITABLE);
	numVal->setValueInteger (6);

	createValue (otaTempIn, "OTAtempIN", "temperature inside of optical tube", false);
	createValue (otaHumIn, "OTAhumIN", "humidity [%] inside of optical tube", false);
	createValue (otaTempOut, "OTAtempOUT", "temperature outside of optical tube", false);
	createValue (otaHumOut, "OTAhumOUT", "humidity [%] outside of optical tube", false);

	createValue (wfShutter, "WFshutter", "WF shutter/cover state", false, RTS2_VALUE_WRITABLE);
	wfShutter->addSelVal ("CLOSED");
	wfShutter->addSelVal ("OPEN");

	addOption ('f', NULL, 1, "serial port with the module (may be /dev/ttyUSBn, defaults to /dev/wfunit)");

	setIdleInfoInterval (10);
}

D50WFUnit::~D50WFUnit ()
{
	delete wfunitConn;
}

int D50WFUnit::processOption (int opt)
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

int D50WFUnit::initHardware ()
{
	int ret;

	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	wfunitConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 50);
	ret = wfunitConn->init ();
	if (ret)
		return ret;

	wfunitConn->flushPortIO ();
	wfunitConn->setDebug (true);

	// it must be twice here, from some reason the first response after inicialization is never received...
	ret = wfunitCommand ('s');
	ret = wfunitCommand ('s');

	return ret;
}

int D50WFUnit::info ()
{
	wfunitCommand ('s');
	return Sensor::info ();
}

int D50WFUnit::scriptEnds ()
{
changeValue(wfShutter, 0);
return 0;
}

int D50WFUnit::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == wfShutter)
	{
		switch (new_value->getValueInteger ())
		{
			case 0:
				wfunitCommand ('c');
				break;
			case 1:
				wfunitCommand ('o');
				break;
		}
		return 0;
	}

	return Sensor::setValue (old_value, new_value);
}

int D50WFUnit::wfunitCommand (char c)
{
	char buf[200];
	char shutState;
	float temp1, temp2, hum1, hum2;

	wfunitConn->flushPortIO();
	wfunitConn->writeRead (&c, 1, buf, 199, '\n');

	// typical reply is: C 28.60 50.59 28.63 51.53 //shutter_state t_out h_out t_in h_in

	int ret = sscanf (buf, "%s %f %f %f %f //*", &shutState, &temp1, &hum1, &temp2, &hum2);
	if (ret != 5)
	{
		logStream (MESSAGE_ERROR) << "cannot parse reply from unit, reply was: '" << buf << "', return " << ret << sendLog;
		return -1;
	}

	switch (shutState)
	{
		case 'C':
			wfShutter->setValueInteger(0);
			break;
		case 'O':
			wfShutter->setValueInteger(1);
			break;
		default:
			logStream (MESSAGE_ERROR) << "unexpected shutter state in response from unit: '" << buf << "'" << sendLog;
			return -1;
	}

	// check the I2C sensors credibility
	if (hum1 > 110.0)
	{
		temp1 = NAN;
		hum1 = NAN;
	}
	if (hum2 > 110.0)
	{
		temp2 = NAN;
		hum2 = NAN;
	}

	otaTempOut->addValue (temp1, numVal->getValueInteger());
	otaTempOut->calculate ();
	otaHumOut->addValue (hum1, numVal->getValueInteger());
	otaHumOut->calculate ();

	otaTempIn->addValue (temp2, numVal->getValueInteger());
	otaTempIn->calculate ();
	otaHumIn->addValue (hum2, numVal->getValueInteger());
	otaHumIn->calculate ();

	return 0;
}

int main (int argc, char **argv)
{
	D50WFUnit device (argc, argv);
	return device.run ();
}

