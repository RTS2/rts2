/* 
 * Sensor daemon for cloudsensor (mrakomer) by Martin Kakona
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
 * Class for cloudsensor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2SensorMrakomer: public Rts2DevSensor
{
	private:
		char *device_file;
		Rts2ConnSerial *mrakConn;

		Rts2ValueDoubleStat *tempDiff;
		Rts2ValueDoubleStat *tempIn;
		Rts2ValueDoubleStat *tempOut;

		Rts2ValueBool *heater;
		Rts2ValueDouble *sleepPeriod;

		Rts2ValueInteger *numberMes;
		Rts2ValueInteger *mrakStatus;

		/**
		 * Read sensor values.
		 */
		int readSensor ();
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();

	public:
		Rts2SensorMrakomer (int in_argc, char **in_argv);
		virtual ~Rts2SensorMrakomer (void);
};

int
Rts2SensorMrakomer::readSensor ()
{
	int ret;
	char buf[51];
	ret = mrakConn->writeRead (heater->getValueBool () ? "h" : "x", 1, buf, 50, '\r');
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	// parse response
	float temp0, temp1;
	int tno, tstat;
	int x = sscanf (buf, "%d %f %f %d", &tno, &temp0, &temp1, &tstat);
	if (x != 4)
	{
		logStream (MESSAGE_ERROR) << "cannot parse reply from cloud senso, reply was: '"
			<< buf << "', return " << x << sendLog;
		return -1;
	}
	tempDiff->addValue (temp0 - temp1, 20);
	tempIn->addValue (temp0, 20);
	tempOut->addValue (temp1, 20);

	tempDiff->calculate ();
	tempIn->calculate ();
	tempOut->calculate ();

	numberMes->setValueInteger (tno);
	mrakStatus->setValueInteger (tstat);

	return 0;
}


Rts2SensorMrakomer::Rts2SensorMrakomer (int in_argc, char **in_argv)
:Rts2DevSensor (in_argc, in_argv)
{
	mrakConn = NULL;

	createValue (tempDiff, "TEMP_DIFF", "temperature difference");
	createValue (tempIn, "TEMP_IN", "temperature inside", true);
	createValue (tempOut, "TEMP_OUT", "tempreature outside", true);

	createValue (heater, "HEATER", "heater state", true);
	createValue (sleepPeriod, "sleep", "sleep period between measurments", false);

	createValue (numberMes, "number_mes", "number of measurements", false);
	createValue (mrakStatus, "status", "device status", true, RTS2_DT_HEX);

	addOption ('f', NULL, 1, "serial port with cloud sensor");
}


Rts2SensorMrakomer::~Rts2SensorMrakomer (void)
{
	delete mrakConn;
}


int
Rts2SensorMrakomer::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Rts2DevSensor::processOption (in_opt);
	}
	return 0;
}


int
Rts2SensorMrakomer::init ()
{
	int ret;
	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	mrakConn = new Rts2ConnSerial (device_file, this, BS2400, C8, NONE, 5);
	ret = mrakConn->init ();
	if (ret)
		return ret;
	
	mrakConn->flushPortIO ();

	return 0;
}


int
Rts2SensorMrakomer::info ()
{
	int ret;
	ret = readSensor ();
	if (ret)
		return -1;
	return Rts2DevSensor::info ();
}


int
main (int argc, char **argv)
{
	Rts2SensorMrakomer device = Rts2SensorMrakomer (argc, argv);
	return device.run ();
}
