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

		Rts2ValueInteger *numVal;

		Rts2ValueDouble *triggerBad;
		Rts2ValueDouble *triggerGood;

		Rts2ValueBool *heater;

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

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

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
	tempDiff->addValue (temp0 - temp1, numVal->getValueInteger ());
	tempIn->addValue (temp0, numVal->getValueInteger ());
	tempOut->addValue (temp1, numVal->getValueInteger ());

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

	createValue (numVal, "num_stat", "number of measurements for weather statistic");
	numVal->setValueInteger (20);

	createValue (triggerBad, "TRIGBAD", "if temp diff drops bellow this value, set bad weather", true);
	triggerBad->setValueDouble (nan ("f"));

	createValue (triggerGood, "TRIGGOOD", "if temp diff gets above this value, drop bad weather flag", true);
	triggerGood->setValueDouble (nan ("f"));

	createValue (heater, "HEATER", "heater state", true);

	createValue (numberMes, "number_mes", "number of measurements", false);
	createValue (mrakStatus, "status", "device status", true, RTS2_DT_HEX);

	addOption ('f', NULL, 1, "serial port with cloud sensor");
	addOption ('b', NULL, 1, "bad trigger point");
	addOption ('g', NULL, 1, "good trigger point");

	setTimeout (20);
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
		case 'b':
			triggerBad->setValueDouble (atof (optarg));
			break;
		case 'g':
			triggerGood->setValueDouble (atof (optarg));
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

	mrakConn = new Rts2ConnSerial (device_file, this, BS2400, C8, NONE, 30);
	ret = mrakConn->init ();
	if (ret)
		return ret;
	
	mrakConn->flushPortIO ();

	if (!isnan (triggerGood->getValueDouble ()))
		setWeatherState (false);

	return 0;
}


int
Rts2SensorMrakomer::info ()
{
	int ret;
	ret = readSensor ();
	if (ret)
	{
		if (getLastInfoTime () > 60)
			setWeatherState (false);
		return -1;
	}
	if (tempDiff->getNumMes () >= numVal->getValueInteger ())
	{
		// trips..
		if (tempDiff->getValueDouble () <= triggerBad->getValueDouble ())
		{
			if (getWeatherState () == true)
			{
				logStream (MESSAGE_INFO) << "setting weather to bad. TempDiff: " << tempDiff->getValueDouble ()
					<< " trigger: " << triggerBad->getValueDouble ()
					<< sendLog;
			}
			setWeatherState (false);
		}
		else if (tempDiff->getValueDouble () >= triggerGood->getValueDouble ())
		{
			if (getWeatherState () == false)
			{
				logStream (MESSAGE_INFO) << "setting weather to good. TempDiff: " << tempDiff->getValueDouble ()
					<< " trigger: " << triggerGood->getValueDouble ()
					<< sendLog;
			}
			setWeatherState (true);
		}
	}
	return Rts2DevSensor::info ();
}


int
Rts2SensorMrakomer::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == heater)
		return 0;
	return Rts2DevSensor::setValue (old_value, new_value);
}


int
main (int argc, char **argv)
{
	Rts2SensorMrakomer device = Rts2SensorMrakomer (argc, argv);
	return device.run ();
}
