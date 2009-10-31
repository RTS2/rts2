/* 
 * Sensor daemon for cloudsensor (mrakomer) by Martin Kakona
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2009 Martin Jelinek
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

namespace rts2sensord
{

/**
 * Class for cloudsensor, version 4.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Cloud4: public SensorWeather
{
	private:
		char *device_file;
		rts2core::ConnSerial *mrakConn;

		Rts2ValueDoubleStat *tempDiff;
		Rts2ValueDoubleStat *tempIn;
		Rts2ValueDoubleStat *tempOut;

		// use this value only for logging to detect if we reported trips
		double lastTempDiff;
		
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
		Cloud4 (int in_argc, char **in_argv);
		virtual ~Cloud4 (void);
};

};

using namespace rts2sensord;

int Cloud4::readSensor ()
{
	int ret;
	char buf[128];
	ret = mrakConn->writeRead (heater->getValueBool () ? "h" : "s", 1, buf, 50, '\r');
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	// parse response
	float temp0, temp1;
	int tno, tstat=1;
	int x = sscanf (buf+6, "%d %f %f %*d %*d %*3s", &tno, &temp0, &temp1);
	temp0/=100.0;
	temp1/=100.0;

	if (x != 3) 
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


Cloud4::Cloud4 (int in_argc, char **in_argv)
:SensorWeather (in_argc, in_argv)
{
	mrakConn = NULL;

	createValue (tempDiff, "TEMP_DIFF", "temperature difference");
	createValue (tempIn, "TEMP_IN", "temperature inside", true);
	createValue (tempOut, "TEMP_OUT", "temperature outside", true);

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

	setIdleInfoInterval (20);
}

Cloud4::~Cloud4 (void)
{
	delete mrakConn;
}

int Cloud4::processOption (int in_opt)
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
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}

int Cloud4::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	mrakConn = new rts2core::ConnSerial (device_file, this, rts2core::BS2400, rts2core::C8, rts2core::NONE, 10);
	ret = mrakConn->init ();
	if (ret)
		return ret;
	
	mrakConn->flushPortIO ();

	if (!isnan (triggerGood->getValueDouble ()))
		setWeatherState (false);
	return 0;
}

int Cloud4::info ()
{
	int ret;
	ret = readSensor ();
	if (ret)
	{
		if (getLastInfoTime () > 60)
			setWeatherTimeout (60);
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
			setWeatherTimeout (300);
		}
		else if (tempDiff->getValueDouble () >= triggerGood->getValueDouble ())
		{
			if (getWeatherState () == false && lastTempDiff < triggerGood->getValueDouble ())
			{
				logStream (MESSAGE_INFO) << "setting weather to good. TempDiff: " << tempDiff->getValueDouble ()
					<< " trigger: " << triggerGood->getValueDouble ()
					<< sendLog;
			}
		}
	}
	// record last value
	lastTempDiff = tempDiff->getValueDouble ();
	return SensorWeather::info ();
}

int Cloud4::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == heater || old_value == triggerBad || old_value == triggerGood)
		return 0;
	return SensorWeather::setValue (old_value, new_value);
}

int main (int argc, char **argv)
{
	Cloud4 device = Cloud4 (argc, argv);
	return device.run ();
}
