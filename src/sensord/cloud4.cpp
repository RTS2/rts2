/* 
 * Sensor daemon for cloudsensor (mrakomer) by Martin Kakona. Versions 4.0 and
 * 4.1 are supported.
 *
 * Copyright (C) 2008-2009,20101 Petr Kubanek <petr@kubanek.net>
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

#include "connection/serial.h"

#define EVENT_CLOUD_HEATER      RTS2_LOCAL_EVENT + 1300

#define OPT_HEAT_ON             OPT_LOCAL + 343
#define OPT_HEAT_DUR            OPT_LOCAL + 344
#define OPT_TEMP_IN_COEFF       OPT_LOCAL + 345
#define OPT_TEMP_AMB_COEFF      OPT_LOCAL + 346

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

		virtual void postEvent (rts2core::Event *event);

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();

		virtual void valueChanged (rts2core::Value *value);

		virtual void usage ();

	private:
		char *device_file;
		rts2core::ConnSerial *mrakConn;

		rts2core::ValueDoubleStat *tempDiff;
		rts2core::ValueDoubleStat *tempIn;
		rts2core::ValueDoubleStat *tempSky;
		rts2core::ValueDoubleStat *tempSky1;
		rts2core::ValueDoubleStat *tempSky2;
		rts2core::ValueDoubleStat *tempAmb;

		rts2core::ValueDouble *tempInCoeff;
		rts2core::ValueDouble *tempAmbCoeff;
		bool tempInCoeffSet;

		// use this value only for logging to detect if we reported trips
		double lastTempDiff;
		
		rts2core::ValueInteger *numVal;

		rts2core::ValueDouble *triggerBad;
		rts2core::ValueDouble *triggerGood;

		rts2core::ValueBool *heater;

		rts2core::ValueInteger *numberMes;
		rts2core::ValueInteger *mrakStatus;

		rts2core::ValueTime *heatStateChangeTime;
		rts2core::ValueInteger *heatInterval;
		rts2core::ValueInteger *heatDuration;

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
	ret = mrakConn->writeRead (heater->getValueBool () ? "h" : "s", 1, buf, 80, '\r');
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	char *buf_start = buf;

	if (buf[0] == '\n')
		buf_start++;

	// parse response
	float temp0, temp1, temp2, temp_amb, temp_sky;
	temp2 = temp_amb = NAN;
	int tno, tstat=1;
	char checksum;
	if (strncmp (buf_start, "$M4.0", 5) == 0)
	{
		int x = sscanf (buf_start + 6, "%d %f %f %*d %*d *%2hhx", &tno, &temp0, &temp1, &checksum);
		if (x != 4) 
		{
			logStream (MESSAGE_ERROR) << "cannot parse reply from cloud sensor, reply was: '" << buf << "', return " << x << sendLog;
			return -1;
		}
	}
	else if (strncmp (buf_start, "$M4.1", 5) == 0)
	{
		int x = sscanf (buf_start + 6, "%d %f %f %f %f %*d %*d *%2hhx", &tno, &temp0, &temp1, &temp2, &temp_amb, &checksum);
		if (x != 6) 
		{
			logStream (MESSAGE_ERROR) << "cannot parse reply from cloud senso, reply was: '" << buf << "', return " << x << sendLog;
			return -1;
		}
		if (temp2 == -27315) 
			temp2 = NAN;
		if (temp_amb == -27315) 
			temp_amb = NAN;
	}
	else
	{
		logStream (MESSAGE_ERROR) << "invalid mrakomer version - supported are only M4.0 and M4.1: " << buf << sendLog;
		return -1;
	}

	// compute checksum
	char ch = 0;
	for (char *bs = buf_start + 1; *bs && *bs != '*'; bs++)
		ch ^= *bs;
	if (checksum != ch)
		logStream (MESSAGE_ERROR) << "invalid checksum - expected " << checksum << ", calculated " << ch << sendLog;

	if (update == false)
		return 0;

	temp0 /= 100.0;
	temp1 /= 100.0;
	if (!std::isnan (temp2))
		temp2 /= 100.0;
	if (!std::isnan (temp_amb))
		temp_amb /= 100.0;

	if (!std::isnan (temp2))
		temp_sky = (temp1 + temp2) / 2.0;
	else
		temp_sky = temp1;

	if (!std::isnan (temp_amb))
		tempDiff->addValue (temp_amb * tempAmbCoeff->getValueDouble () + temp0 * tempInCoeff->getValueDouble () - temp_sky, numVal->getValueInteger ());
	else
	{
		if (tempInCoeffSet == false)
		{
			tempInCoeff->setValueDouble (tempAmbCoeff->getValueDouble ());
			tempInCoeffSet = true;
		}
		tempDiff->addValue (temp0 * tempInCoeff->getValueDouble () - temp_sky, numVal->getValueInteger ());
	}

	tempIn->addValue (temp0, numVal->getValueInteger ());
	tempSky->addValue (temp_sky, numVal->getValueInteger ());
	tempSky1->addValue (temp1, numVal->getValueInteger ());
	if (!std::isnan (temp2))
	{
		if (tempSky2 == NULL)
		{
		 	createValue (tempSky2, "TEMP_SKY2", "sky temperature (sensor 2)", false);
			updateMetaInformations (tempSky2);
		}
		tempSky2->addValue (temp2, numVal->getValueInteger ());
	}
	if (!std::isnan (temp_amb))
	{
		if (tempAmb == NULL)
		{
		 	createValue (tempAmb, "TEMP_AMB", "ambient temperature (outside sensor)", true);
			updateMetaInformations (tempAmb);
		}
		tempAmb->addValue (temp_amb, numVal->getValueInteger ());
	}


	tempDiff->calculate ();
	tempIn->calculate ();
	tempSky->calculate ();
	tempSky1->calculate ();
	if (tempSky2)
		tempSky2->calculate ();
	if (tempAmb)
		tempAmb->calculate ();

	numberMes->setValueInteger (tno);
	mrakStatus->setValueInteger (tstat);

	return 0;
}

Cloud4::Cloud4 (int in_argc, char **in_argv):SensorWeather (in_argc, in_argv)
{
	mrakConn = NULL;

	createValue (tempDiff, "TEMP_DIFF", "temperature difference", true);
	createValue (tempIn, "TEMP_IN", "temperature inside of the detector", true);
	createValue (tempSky, "TEMP_SKY", "sky temperature computed from all available sensors", true);
	createValue (tempSky1, "TEMP_SKY1", "sky temperature (sensor 1)", false);

	tempSky2 = tempAmb = NULL;

	createValue (numVal, "num_stat", "number of measurements for weather statistics", false, RTS2_VALUE_WRITABLE);
	numVal->setValueInteger (6);

	createValue (tempInCoeff, "temp_in_coeff", "temperature IN coefficient: TEMP_DIFF = temp_in_coeff * TEMP_IN + temp_amb_coeff * TEMP_AMB - TEMP_SKY", false, RTS2_VALUE_WRITABLE);
	tempInCoeff->setValueDouble (0.0);

	createValue (tempAmbCoeff, "temp_amb_coeff", "temperature AMB coefficient: TEMP_DIFF = temp_in_coeff * TEMP_IN + temp_amb_coeff * TEMP_AMB - TEMP_SKY", false, RTS2_VALUE_WRITABLE);
	tempAmbCoeff->setValueDouble (1.0);

	tempInCoeffSet = false;

	createValue (triggerBad, "TRIGBAD", "if temp diff drops bellow this value, set bad weather", true, RTS2_VALUE_WRITABLE);
	triggerBad->setValueDouble (NAN);

	createValue (triggerGood, "TRIGGOOD", "if temp diff gets above this value, drop bad weather flag", true, RTS2_VALUE_WRITABLE);
	triggerGood->setValueDouble (NAN);

	createValue (heater, "HEATER", "heater state", true, RTS2_VALUE_WRITABLE);

	createValue (numberMes, "number_mes", "number of last measurement (internal from cloud device)", false);
	createValue (mrakStatus, "status", "device status", true, RTS2_DT_HEX);

	createValue (heatStateChangeTime, "heat_state_change_time", "turn heater on until this time", false);
	heatStateChangeTime->setValueDouble (NAN);

	createValue (heatInterval, "heat_interval", "turn heater on after this amount of time", false, RTS2_VALUE_WRITABLE);
	heatInterval->setValueInteger (-1);

	createValue (heatDuration, "heat_duration", "time duration during which heater remain on", false, RTS2_VALUE_WRITABLE);
	heatDuration->setValueInteger (-1);

	addOption ('f', NULL, 1, "serial port with cloud sensor");
	addOption ('b', NULL, 1, "bad trigger point");
	addOption ('g', NULL, 1, "good trigger point");

	addOption (OPT_HEAT_ON, "heat-interval", 1, "interval between successive turning of the heater");
	addOption (OPT_HEAT_DUR, "heat-duration", 1, "heat duration in seconds");
	addOption (OPT_TEMP_IN_COEFF, "temp-in-coeff", 1, "temperature IN coefficient (see definition above), defaults to 0.0");
	addOption (OPT_TEMP_AMB_COEFF, "temp-amb-coeff", 1, "temperature AMB coefficient (see definition above), defaults to 1.0");

	setIdleInfoInterval (20);
}

Cloud4::~Cloud4 (void)
{
	delete mrakConn;
}

void Cloud4::postEvent (rts2core::Event * event)
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
					deleteTimers (EVENT_CLOUD_HEATER);
					heater->setValueBool (!heater->getValueBool ());
					addTimer (t_diff, new rts2core::Event (EVENT_CLOUD_HEATER, this));
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

void Cloud4::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	if (heatInterval->getValueInteger () > 0 && heatDuration->getValueInteger () > 0)
	{
		int ms = new_state & SERVERD_STATUS_MASK;
		switch (ms)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
			case SERVERD_STANDBY | SERVERD_DUSK:
			case SERVERD_STANDBY | SERVERD_NIGHT:
			case SERVERD_STANDBY | SERVERD_DAWN:
				addTimer (heatInterval->getValueInteger (), new rts2core::Event (EVENT_CLOUD_HEATER, this));
				break;
			default:
				heater->setValueBool (false);
				deleteTimers (EVENT_CLOUD_HEATER);
				readSensor (false);
				break;
		}
	}
	SensorWeather::changeMasterState (old_state, new_state);
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
			tempInCoeffSet = true;
			break;
		case OPT_TEMP_AMB_COEFF:
			tempAmbCoeff->setValueCharArr (optarg);
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
	mrakConn->setDebug (getDebug ());
	ret = mrakConn->init ();
	if (ret)
		return ret;
	
	mrakConn->flushPortIO ();

	if (!std::isnan (triggerGood->getValueDouble ()))
		setWeatherState (false, "TRIGGOOD unspecified");
	return 0;
}

int Cloud4::info ()
{
	int ret;
	ret = readSensor (true);
	if (ret)
	{
		// OK, this is because of a partially broken hardware we have now, the response is sometimes wrong...
		// Before we will solve this the correct way, let's use this hotfix... It isn't a bad thing anyway...?
		// We don't need to solve potential errors in other calls of this function, as they are all related to heating - and the heating is being switched of automatically by the device's internal timer.
		int retryAttempt;
		for ( retryAttempt = 0; retryAttempt < 2; retryAttempt ++)
		{
			sleep (3);
			mrakConn->flushPortIO ();
			logStream (MESSAGE_ERROR) << "There was a problem in readSensor (), retrying..." << sendLog;
			ret = readSensor (true);
			if (!ret)
				break;
		}
		if (retryAttempt >= 2)
		{
			if (getLastInfoTime () > 60)
				setWeatherTimeout (60, "cannot read data from device");
			return -1;
		}
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
			valueError (tempDiff);
		}
		else if (tempDiff->getValueDouble () >= triggerGood->getValueDouble ())
		{
			if (getWeatherState () == false && lastTempDiff < triggerGood->getValueDouble ())
			{
				logStream (MESSAGE_INFO) << "setting weather to good. TempDiff: " << tempDiff->getValueDouble ()
					<< " trigger: " << triggerGood->getValueDouble ()
					<< sendLog;
				valueGood (tempDiff);
			}
		}
		// gray zone - if it's bad weather, keep it bad
		else if (getWeatherState () == false)
		{
			setWeatherTimeout (300, "did not rise above TRIGGOOD");
			valueWarning (tempDiff);
		}
		else
		{
			valueGood (tempDiff);
		}
	}
	else
	{
		setWeatherTimeout (85, "waiting for enough measurements from cloud sensor");
	}
	// record last value
	lastTempDiff = tempDiff->getValueDouble ();
	return SensorWeather::info ();
}

void Cloud4::valueChanged (rts2core::Value *value)
{
	if (value == heatInterval || value == heatDuration)
	{
		deleteTimers (EVENT_CLOUD_HEATER);
		if (heatInterval->getValueInteger () > 0 && heatDuration->getValueInteger () > 0)
		{
			addTimer (heatDuration->getValueInteger (), new rts2core::Event (EVENT_CLOUD_HEATER, this));
		}
		else
		{
			heater->setValueBool (false);
			readSensor (false);
		}
	}
	if (value == tempInCoeff)
		tempInCoeffSet = true;
	SensorWeather::valueChanged (value);
}

void Cloud4::usage ()
{
	std::cout << "  " << getAppName () << " -f /dev/ttyUSB0 --heat-interval 300 --heat-duration 30 -g 9.0 -b 7.0 --server localhost -d CLOUD" << std::endl;
			
	std::cout << "Definition:" << std::endl
	<< "  TEMP_DIFF = (temp_in_coeff * TEMP_IN) + (temp_amb_coeff * TEMP_AMB) - TEMP_SKY" << std::endl;
}


int main (int argc, char **argv)
{
	Cloud4 device = Cloud4 (argc, argv);
	return device.run ();
}
