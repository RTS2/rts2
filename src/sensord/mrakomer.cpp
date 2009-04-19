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

namespace rts2sensor
{

/**
 * Class for cloudsensor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Mrakomer: public SensorWeather
{
	private:
		char *device_file;
		Rts2ConnSerial *mrakConn;

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

		Rts2ValueTime *heatStateChangeTime;
		Rts2ValueInteger *heatInterval;
		Rts2ValueInteger *heatDuration;

		/**
		 * Read sensor values.
		 */
		int readSensor ();
	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();

		virtual int idle ();

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

	public:
		Mrakomer (int in_argc, char **in_argv);
		virtual ~Mrakomer (void);
};

};

using namespace rts2sensor;

int
Mrakomer::readSensor ()
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


int
Mrakomer::processOption (int in_opt)
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


int
Mrakomer::init ()
{
	int ret;
	ret = SensorWeather::init ();
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
Mrakomer::info ()
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


int
Mrakomer::idle ()
{
	int ms = getMasterState () & SERVERD_STATUS_MASK;
	if (heatInterval->getValueInteger () > 0 && heatDuration->getValueInteger () > 0
	  && (ms == SERVERD_DUSK || ms == SERVERD_NIGHT || ms == SERVERD_DAWN))
	{
		if (isnan (heatStateChangeTime->getValueDouble ()))
		{
			heatStateChangeTime->setValueDouble (getNow () + heatDuration->getValueInteger ());
			heater->setValueBool (true);
			setIdleInfoInterval (heatDuration->getValueInteger ());
			setIdleInfoInterval (heatInterval->getValueInteger ());
		}
		else if (heatStateChangeTime->getValueDouble () < getNow ())
		{
			if (heater->getValueBool ())
				heatStateChangeTime->setValueDouble (getNow () + heatInterval->getValueInteger ());
			else
			  	heatStateChangeTime->setValueDouble (getNow () + heatDuration->getValueInteger ());
			heater->setValueBool (!heater->getValueBool ());
		}
		sendValueAll (heater);
		sendValueAll (heatStateChangeTime);
	}
	return SensorWeather::idle ();
}


int
Mrakomer::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == heater || old_value == triggerBad || old_value == triggerGood)
	{
		return 0;
	}
	if (old_value == heatInterval || old_value == heatDuration)
	{
		if (new_value->getValueInteger () <= 0)
		{
			heater->setValueBool (false); 
		}
		return 0;
	}
	return SensorWeather::setValue (old_value, new_value);
}


Mrakomer::Mrakomer (int argc, char **argv):SensorWeather (argc, argv)
{
	mrakConn = NULL;

	createValue (tempDiff, "TEMP_DIFF", "temperature difference", true);
	createValue (tempIn, "TEMP_IN", "temperature inside", true);
	createValue (tempOut, "TEMP_OUT", "tempreature outside", true);

	createValue (numVal, "num_stat", "number of measurements for weather statistic", false);
	numVal->setValueInteger (20);

	createValue (triggerBad, "TRIGBAD", "if temp diff drops bellow this value, set bad weather", false);
	triggerBad->setValueDouble (nan ("f"));

	createValue (triggerGood, "TRIGGOOD", "if temp diff gets above this value, drop bad weather flag", false);
	triggerGood->setValueDouble (nan ("f"));

	createValue (heater, "HEATER", "heater state", false);

	createValue (numberMes, "number_mes", "number of measurements", false);
	createValue (mrakStatus, "status", "device status", false, RTS2_DT_HEX);

	createValue (heatStateChangeTime, "heat_state_change_time", "turn heater on until this time", false);
	heatStateChangeTime->setValueDouble (nan("f"));

	createValue (heatInterval, "heat_interval", "turn heater on after this amount of time", false);
	heatInterval->setValueInteger (-1);

	createValue (heatDuration, "heat_duration", "time duration during which heater remain on", false);
	heatDuration->setValueInteger (-1);

	addOption ('f', NULL, 1, "serial port with cloud sensor");
	addOption ('b', NULL, 1, "bad trigger point");
	addOption ('g', NULL, 1, "good trigger point");

	setTimeout (20);
}


Mrakomer::~Mrakomer (void)
{
	delete mrakConn;
}


int
main (int argc, char **argv)
{
	Mrakomer device = Mrakomer (argc, argv);
	return device.run ();
}
