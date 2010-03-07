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

#define EVENT_CLOUD_HEATER      RTS2_LOCAL_EVENT + 1300

#define OPT_HEAT_ON             OPT_LOCAL + 343
#define OPT_HEAT_DUR            OPT_LOCAL + 344
#define OPT_TEMP_IN_COEFF       OPT_LOCAL + 345

namespace rts2sensord
{

/**
 * Class for cloudsensor, version 4.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Cloud4: public SensorWeather
{
	public:
		Cloud4 (int in_argc, char **in_argv);
		virtual ~Cloud4 (void);

		virtual void postEvent (Rts2Event *event);

		virtual int changeMasterState (int new_state);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();

		virtual void valueChanged (Rts2Value *value);

	private:
		char *device_file;
		rts2core::ConnSerial *mrakConn;

		Rts2ValueDoubleStat *tempDiff;
		Rts2ValueDoubleStat *tempIn;
		Rts2ValueDoubleStat *tempOut;

		Rts2ValueDouble *tempInCoeff;

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
		int readSensor (bool update);
};

};

using namespace rts2sensord;

int Cloud4::readSensor (bool update)
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
	if (update == false)
		return 0;

	tempDiff->addValue (tempInCoeff->getValueDouble () * temp0 - temp1, 20);
	tempIn->addValue (temp0, 20);
	tempOut->addValue (temp1, 20);

	tempDiff->calculate ();
	tempIn->calculate ();
	tempOut->calculate ();

	numberMes->setValueInteger (tno);
	mrakStatus->setValueInteger (tstat);

	return 0;
}

Cloud4::Cloud4 (int in_argc, char **in_argv):SensorWeather (in_argc, in_argv)
{
	mrakConn = NULL;

	createValue (tempDiff, "TEMP_DIFF", "temperature difference", true);
	createValue (tempIn, "TEMP_IN", "temperature inside", true);
	createValue (tempOut, "TEMP_OUT", "temperature outside", true);

	createValue (numVal, "num_stat", "number of measurements for weather statistic", false, RTS2_VALUE_WRITABLE);
	numVal->setValueInteger (20);

	createValue (tempInCoeff, "temp_in_coeff", "temperature in coefficient (multiplicator) - TEMP_DIFF = temp_in_coeff * TEMP_IN - TEMP_OUT", false, RTS2_VALUE_WRITABLE);
	tempInCoeff->setValueDouble (1.0);

	createValue (triggerBad, "TRIGBAD", "if temp diff drops bellow this value, set bad weather", true, RTS2_VALUE_WRITABLE);
	triggerBad->setValueDouble (rts2_nan ("f"));

	createValue (triggerGood, "TRIGGOOD", "if temp diff gets above this value, drop bad weather flag", true, RTS2_VALUE_WRITABLE);
	triggerGood->setValueDouble (rts2_nan ("f"));

	createValue (heater, "HEATER", "heater state", true, RTS2_VALUE_WRITABLE);

	createValue (numberMes, "number_mes", "number of measurements", false);
	createValue (mrakStatus, "status", "device status", true, RTS2_DT_HEX);

	createValue (heatStateChangeTime, "heat_state_change_time", "turn heater on until this time", false);
	heatStateChangeTime->setValueDouble (rts2_nan ("f"));

	createValue (heatInterval, "heat_interval", "turn heater on after this amount of time", false, RTS2_VALUE_WRITABLE);
	heatInterval->setValueInteger (-1);

	createValue (heatDuration, "heat_duration", "time duration during which heater remain on", false, RTS2_VALUE_WRITABLE);
	heatDuration->setValueInteger (-1);

	addOption ('f', NULL, 1, "serial port with cloud sensor");
	addOption ('b', NULL, 1, "bad trigger point");
	addOption ('g', NULL, 1, "good trigger point");

	addOption (OPT_HEAT_ON, "heat-interval", 1, "interval between successive turing of the heater");
	addOption (OPT_HEAT_DUR, "heat-duration", 1, "heat duration in seconds");
	addOption (OPT_TEMP_IN_COEFF, "temp-in-coeff", 1, "temperature coefficient");

	setIdleInfoInterval (20);
}

Cloud4::~Cloud4 (void)
{
	delete mrakConn;
}

void Cloud4::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_CLOUD_HEATER:
			{
				int ms = getMasterState () & SERVERD_STATUS_MASK;
				if (!(ms == SERVERD_DUSK || ms == SERVERD_NIGHT || ms == SERVERD_DAWN))
				{
					heater->setValueBool (false);
				}
				else
				{
					int t_diff;
					if (heater->getValueBool ())
						t_diff = heatInterval->getValueInteger ();
					else
						t_diff = heatDuration->getValueInteger ();
					heater->setValueBool (!heater->getValueBool ());
					addTimer (t_diff, new Rts2Event (EVENT_CLOUD_HEATER, this));
					heatStateChangeTime->setValueDouble (getNow () + t_diff);
				}
				readSensor (false);
				sendValueAll (heater);
				sendValueAll (heatStateChangeTime);
			}
			break;
	}
	SensorWeather::postEvent (event);
}

int Cloud4::changeMasterState (int new_state)
{
	if (heatInterval->getValueInteger () > 0 && heatDuration->getValueInteger () > 0)
	{
		int ms = new_state & SERVERD_STATUS_MASK;
		switch (ms)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				addTimer (heatInterval->getValueInteger (), new Rts2Event (EVENT_CLOUD_HEATER, this));
				break;
			default:
				heater->setValueBool (false);
				deleteTimers (EVENT_CLOUD_HEATER);
				readSensor (false);
				break;
		}
	}
	return SensorWeather::changeMasterState (new_state);
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
		case OPT_HEAT_ON:
			heatInterval->setValueCharArr (optarg);
			break;
		case OPT_HEAT_DUR:
			heatDuration->setValueCharArr (optarg);
			break;
		case OPT_TEMP_IN_COEFF:
			tempInCoeff->setValueCharArr (optarg);
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
		setWeatherState (false, "TRIGGOOD unspecified");
	return 0;
}

int Cloud4::info ()
{
	int ret;
	ret = readSensor (true);
	if (ret)
	{
		if (getLastInfoTime () > 60)
			setWeatherTimeout (60, "cannot read data from device");
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
			setWeatherTimeout (300, "crossed TRIGBAD");
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
		// gray zone - if it's bad weather, keep it bad
		else if (getWeatherState () == false)
		{
			setWeatherTimeout (300, "do not rised above TRIGGOOD");
		}
	}
	// record last value
	lastTempDiff = tempDiff->getValueDouble ();
	return SensorWeather::info ();
}

void Cloud4::valueChanged (Rts2Value *value)
{
	if (value == heatInterval || value == heatDuration)
	{
		deleteTimers (EVENT_CLOUD_HEATER);
		if (heatInterval->getValueInteger () > 0 && heatDuration->getValueInteger () > 0)
		{
			addTimer (heatDuration->getValueInteger (), new Rts2Event (EVENT_CLOUD_HEATER, this));
		}
		else
		{
			heater->setValueBool (false);
			readSensor (false);
		}
	}
	SensorWeather::valueChanged (value);
}

int main (int argc, char **argv)
{
	Cloud4 device = Cloud4 (argc, argv);
	return device.run ();
}
