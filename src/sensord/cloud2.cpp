/* 
 * Sensor daemon for cloudsensor (mrakomer) by Martin Kakona. Version 2.2
 * (motor-equipped) is supported.
 *
 * Copyright (C) 2018 Jan Strobl
 * Based on cloud4 driver.
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
#define EVENT_CLOUD_MEASUREMENT	RTS2_LOCAL_EVENT + 1301

#define OPT_HEAT_ON             OPT_LOCAL + 343
#define OPT_HEAT_DUR            OPT_LOCAL + 344
#define OPT_RAINDET_DEVICE      OPT_LOCAL + 345
#define OPT_RAINDET_VARIABLE    OPT_LOCAL + 346

namespace rts2sensord
{

/**
 * Class for cloudsensor, version 2.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Jan Strobl
 */
class Cloud2: public SensorWeather
{
	public:
		Cloud2 (int in_argc, char **in_argv);
		virtual ~Cloud2 (void);

		virtual void postEvent (rts2core::Event *event);

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

	protected:
		virtual int processOption (int in_opt);
		virtual int initHardware ();

		virtual int info ();

		virtual void valueChanged (rts2core::Value *value);

		virtual void usage ();

		virtual int willConnect (rts2core::NetworkAddress * in_addr);

	private:
		char *device_file;
		rts2core::ConnSerial *mrakConn;

		char *rainDetectorDevice;
		char *rainDetectorVariable;

		rts2core::ValueSelection *rainStateByRainDet;

		rts2core::ValueDoubleStat *tempDiff;
		rts2core::ValueDoubleStat *tempIn;
		rts2core::ValueDoubleStat *tempSky;
		rts2core::ValueDoubleStat *tempSky1;
		rts2core::ValueDoubleStat *tempSky2;
		rts2core::ValueDoubleStat *tempSky3;
		rts2core::ValueDoubleStat *tempGround;

		// use this value only for logging to detect if we reported trips
		double lastTempDiff;
		
		rts2core::ValueInteger *numVal;

		rts2core::ValueDouble *triggerBad;
		rts2core::ValueDouble *triggerGood;

		rts2core::ValueBool *heater;

		rts2core::ValueTime *heatStateChangeTime;
		rts2core::ValueInteger *heatInterval;
		rts2core::ValueInteger *heatDuration;

		rts2core::ValueInteger *measurementIntervalStandard;
		rts2core::ValueInteger *measurementIntervalOn;
		rts2core::ValueInteger *measurementIntervalActual;
		rts2core::ValueBool *measureDuringDay;
		rts2core::ValueBool *measurementBoost;

		/**
		 * Check rain state from external rain sensor.
		 *
		 * @return zero (0) when no rain detected and sensor seems to be OK.
		 */
		int getRainState ();

		/**
		 * Read sensor values, without performing movement. Also keeps the heater on, when desired.
		 */
		int readSensor (bool update);

		/**
		 * Perform measurement of sky. Measure temperature in azimuth/alt: East/45, 90 (zenith), West/45.
		 */
		int measureSky (bool update);
};

};

using namespace rts2sensord;

int Cloud2::readSensor (bool update)
{
	int ret;
	char buf[1024];
	ret = mrakConn->writeRead (heater->getValueBool () ? "h" : "f", 1, buf, 50, '\r');
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	char *buf_start = buf;

	if (buf[0] == '\n')
		buf_start++;

	// Typical response:
	// F 1773 1157 T 0
	// where:
	// F	last command (E in case of error)
	// 1773	temp inside device (influenced by heater) * 100
	// 1157 temp measured by sensor * 100
	// 0	countdown in seconds of keeping heater on (0 means heater off)

	// parse response
	int tempInRaw, tempGroundRaw, heaterCD;
	if (buf_start[0] == heater->getValueBool () ? "H" : "F")
	{
		int x = sscanf (buf_start + 2, "%d %d T %d", &tempInRaw, &tempGroundRaw, &heaterCD);
		if (x != 3) 
		{
			logStream (MESSAGE_ERROR) << "readsensor (): cannot parse reply from cloud sensor, reply was: '" << buf << "', return " << x << sendLog;
			return -1;
		}
	}
	else
	{
		logStream (MESSAGE_ERROR) << "readSensor (): error, reply was: '" << buf << "'" << sendLog;
		return -1;
	}

	if (update == false)
		return 0;

	tempIn->addValue (tempInRaw/100.0, numVal->getValueInteger ());
	tempGround->addValue (tempGroundRaw/100.0, numVal->getValueInteger ());

	tempIn->calculate ();
	tempGround->calculate ();

	return 0;
}


int Cloud2::measureSky (bool update)
{
	int ret;
	char buf[1024];
	char *buf_start;

	// check the rain-sensor, mrakomer2 must NOT measure during the rain!
	ret = getRainState ();
	if (ret != 0)
		return ret;

	// check the device, it's fast and easy do it this way
	ret = readSensor (false);
	if (ret < 0)
		return ret;

	// There is a small chance the device is not properly parked before 
	// measurement (wind, ...), readSensor () isn't taking care of it. It 
	// might be an overkill, but we will do the minimal-movement 
	// measurement prior the main measuring procedure to take care of it:
	ret = mrakConn->writeRead ("0", 1, buf, 50, '\r');
	if (ret < 0)
		return ret;

	buf_start = buf;

	if (buf[0] == '\n')
		buf_start++;

	if (buf_start[0] != 'S')
	{
		buf[ret] = '\0';
		logStream (MESSAGE_ERROR) << "measureSky (): error in command '0', reply was: '" << buf << "'" << sendLog;	
		return -1;
	}


	// OK, now the real measurement
	ret = mrakConn->writeRead ("m", 1, buf, 80, '\r');
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	buf_start = buf;

	if (buf[0] == '\n')
		buf_start++;

	// parse response
	int tempInRaw1, tempInRaw2, tempInRaw3, tempInRaw4, tempOutRaw1, tempOutRaw2, tempOutRaw3, tempGroundRaw, heaterCD;
	int x = sscanf (buf_start, "A %d %d B %d %d C %d %d G %d %d T %d", &tempInRaw1, &tempOutRaw1, &tempInRaw2, &tempOutRaw2, &tempInRaw3, &tempOutRaw3, &tempInRaw4, &tempGroundRaw, &heaterCD);
	if (x != 9) 
	{
		logStream (MESSAGE_ERROR) << "measureSky (): cannot parse reply from cloud sensor, reply was: '" << buf << "', return " << x << sendLog;
		return -1;
	}

	if (update == false)
		return 0;


	tempIn->addValue ((tempInRaw1+tempInRaw2+tempInRaw3+tempInRaw4)/400.0 , numVal->getValueInteger ());
	tempSky1->addValue (tempOutRaw1/100.0, numVal->getValueInteger ());
	tempSky2->addValue (tempOutRaw2/100.0, numVal->getValueInteger ());
	tempSky3->addValue (tempOutRaw3/100.0, numVal->getValueInteger ());

	float tempSkyAct = (tempOutRaw1+tempOutRaw2+tempOutRaw3)/300.0;
	float tempGroundAct = tempGroundRaw/100.0;

	tempSky->addValue (tempSkyAct, numVal->getValueInteger ());
	tempGround->addValue (tempGroundAct, numVal->getValueInteger ());

	tempDiff->addValue (tempGroundAct - tempSkyAct, numVal->getValueInteger ());

	tempIn->calculate ();
	tempSky1->calculate ();
	tempSky2->calculate ();
	tempSky3->calculate ();
	tempSky->calculate ();
	tempDiff->calculate ();

	return 0;
}

int Cloud2::getRainState ()
{
	rts2core::Connection *connRain = NULL;
	rts2core::Value *rain_tmp = NULL;

	connRain = getOpenConnection (rainDetectorDevice);
	if (connRain == NULL) {
		logStream (MESSAGE_ERROR) << "Rain detector: device not connected!" << sendLog;
		rainStateByRainDet->setValueInteger (2);
		return -1;	// error connecting to rain detector
	}

	rain_tmp = connRain->getValue (rainDetectorVariable);
	if (rain_tmp == NULL) {
		logStream (MESSAGE_ERROR) << "Rain detector: wrong rain-variable name?" << sendLog;
		rainStateByRainDet->setValueInteger (2);
		return -1;	// error connecting to rain detector (unknown variable)
	}
	
	if (rain_tmp->getValueInteger () == 1) {	// 1 means true, see getValueBool ()
		rainStateByRainDet->setValueInteger (1);
		return 1;	// it's raining
	}

	// It seems like there is no rain... 
	// To add a bit more reliability, we check if the infotime of rain-sensor is sufficiently fresh.
	rain_tmp = connRain->getValue ("infotime");
      	if (getNow () - rain_tmp->getValueDouble () > 10.0 )
	{
		logStream (MESSAGE_ERROR) << "Rain detector: infotime too old: " << getNow () - rain_tmp->getValueDouble () << "s" << sendLog;
		rainStateByRainDet->setValueInteger (2);
		return -1;
	}

	// OK, it seems like there is no rain!
	rainStateByRainDet->setValueInteger (0);
	return 0;
}


Cloud2::Cloud2 (int in_argc, char **in_argv):SensorWeather (in_argc, in_argv)
{
	mrakConn = NULL;

	createValue (rainStateByRainDet, "RAIN_BY_RAINDET", "rain status from rain detector", false);
	rainStateByRainDet->addSelVal ("dry");			// 0
	rainStateByRainDet->addSelVal ("wet (raining)");	// 1
	rainStateByRainDet->addSelVal ("N/A");			// 2
	rainStateByRainDet->setValueInteger (2);

	createValue (tempDiff, "TEMP_DIFF", "temperature difference", true);
	createValue (tempIn, "TEMP_IN", "temperature inside of the detector", false);
	createValue (tempSky, "TEMP_SKY", "average sky temperature (from all 3 positions)", true);
	createValue (tempSky1, "TEMP_SKY1", "sky temperature 1 - 45deg East", false);
	createValue (tempSky2, "TEMP_SKY2", "sky temperature 2 - zenith", false);
	createValue (tempSky3, "TEMP_SKY3", "sky temperature 3 - 45deg West", false);
	createValue (tempGround, "TEMP_GROUND", "measured temperature of ground", false);
	tempDiff->setValueDouble (NAN);
	tempIn->setValueDouble (NAN);
	tempSky->setValueDouble (NAN);
	tempSky1->setValueDouble (NAN);
	tempSky2->setValueDouble (NAN);
	tempSky3->setValueDouble (NAN);
	tempGround->setValueDouble (NAN);

	createValue (numVal, "num_stat", "number of measurements for weather statistics", false, RTS2_VALUE_WRITABLE);
	numVal->setValueInteger (3);

	createValue (triggerBad, "TRIGBAD", "if temp diff drops bellow this value, set bad weather", false, RTS2_VALUE_WRITABLE);
	triggerBad->setValueDouble (NAN);

	createValue (triggerGood, "TRIGGOOD", "if temp diff gets above this value, drop bad weather flag", false, RTS2_VALUE_WRITABLE);
	triggerGood->setValueDouble (NAN);

	createValue (heater, "HEATER", "heater state", false, RTS2_VALUE_WRITABLE);

	createValue (heatStateChangeTime, "heat_ch_time", "turn heater on until this time", false);
	heatStateChangeTime->setValueDouble (NAN);

	createValue (heatInterval, "heat_interval", "turn heater on after this amount of time [s]", false, RTS2_VALUE_WRITABLE);
	heatInterval->setValueInteger (-1);

	createValue (heatDuration, "heat_duration", "time duration during which heater remain on [s]", false, RTS2_VALUE_WRITABLE);
	heatDuration->setValueInteger (-1);

	createValue (measurementIntervalStandard, "msr_interval_low", "measurements interval when not in ON state [s]", false, RTS2_VALUE_WRITABLE);
	measurementIntervalStandard->setValueInteger (300);

	createValue (measurementIntervalOn, "msr_interval_high", "measurements interval when in ON state [s]", false, RTS2_VALUE_WRITABLE);
	measurementIntervalOn->setValueInteger (60);

	createValue (measurementIntervalActual, "msr_interval", "actual measurements interval, set automatically with state [s]", false, RTS2_VALUE_WRITABLE);
	measurementIntervalActual->setValueInteger (10);	// we want set it low, it will be reset during the first run

	createValue (measureDuringDay, "msr_during_day", "measure also during day", false, RTS2_VALUE_WRITABLE);
	measureDuringDay->setValueBool (true);

	createValue (measurementBoost, "msr_boost", "temporary measurement interval boost", false);
	measurementBoost->setValueBool (true);	// we have lack of data at the beginning

	addOption ('f', NULL, 1, "serial port with cloud sensor");
	addOption ('b', NULL, 1, "bad trigger point");
	addOption ('g', NULL, 1, "good trigger point");

	addOption (OPT_HEAT_ON, "heat-interval", 1, "interval between successive turning of the heater");
	addOption (OPT_HEAT_DUR, "heat-duration", 1, "heat duration in seconds");
	addOption (OPT_RAINDET_DEVICE, "rain-detector", 1, "name of external rain detector device");
	addOption (OPT_RAINDET_VARIABLE, "rain-variable", 1, "name of rain-variable in rain detector device");
	rainDetectorDevice = NULL;
	rainDetectorVariable = NULL;

	setIdleInfoInterval (20);

	addTimer (measurementIntervalActual->getValueInteger (), new rts2core::Event (EVENT_CLOUD_MEASUREMENT, this));
}

Cloud2::~Cloud2 (void)
{
	delete mrakConn;
}

void Cloud2::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_CLOUD_HEATER:
			{
				int dayStatus = getMasterState () & SERVERD_STATUS_MASK;
				if (!(dayStatus == SERVERD_DUSK || dayStatus == SERVERD_NIGHT || dayStatus == SERVERD_DAWN))
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
			}
			break;
		case EVENT_CLOUD_MEASUREMENT:
			{
				int dayStatus = getMasterState () & SERVERD_STATUS_MASK;
				int onOffStatus = getMasterState () & SERVERD_ONOFF_MASK;
				if (!(dayStatus == SERVERD_DUSK || dayStatus == SERVERD_NIGHT || dayStatus == SERVERD_DAWN) && measureDuringDay->getValueBool () == false)
				{
					// not measuring, set the values to NAN
					tempDiff->setValueDouble (NAN);
					//tempIn->setValueDouble (NAN);
					tempSky->setValueDouble (NAN);
					tempSky1->setValueDouble (NAN);
					tempSky2->setValueDouble (NAN);
					tempSky3->setValueDouble (NAN);
					//tempGround->setValueDouble (NAN);

					measurementIntervalActual->setValueInteger (measurementIntervalStandard->getValueInteger ());
					measurementBoost->setValueBool (false);
				}
				else
				{
					int ret = measureSky (true);
					if (ret != 0)
					{
						// it's raining or the device is not working properly...
						tempDiff->setValueDouble (NAN);
						//tempIn->setValueDouble (NAN);
						tempSky->setValueDouble (NAN);
						tempSky1->setValueDouble (NAN);
						tempSky2->setValueDouble (NAN);
						tempSky3->setValueDouble (NAN);
						//tempGround->setValueDouble (NAN);
					}

					
					// we want to shorten the interval if we don't have enough data
					if (std::isnan (tempDiff->getValueDouble ()) || tempDiff->getNumMes () < numVal->getValueInteger ())
					{
						measurementBoost->setValueBool (true);
						measurementIntervalActual->setValueInteger (measurementIntervalOn->getValueInteger ());
					}
					else if (measurementBoost->getValueBool () == true)
					{
						measurementBoost->setValueBool (false);
						if (onOffStatus == SERVERD_ON && (dayStatus == SERVERD_DUSK || dayStatus == SERVERD_NIGHT || dayStatus == SERVERD_DAWN))
							measurementIntervalActual->setValueInteger (measurementIntervalOn->getValueInteger ());
						else
							measurementIntervalActual->setValueInteger (measurementIntervalStandard->getValueInteger ());
					}
				}

				deleteTimers (EVENT_CLOUD_MEASUREMENT);
				addTimer (measurementIntervalActual->getValueInteger (), new rts2core::Event (EVENT_CLOUD_MEASUREMENT, this));
			}
			break;
	}
	SensorWeather::postEvent (event);
}

void Cloud2::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	int ms = new_state & (SERVERD_STATUS_MASK | SERVERD_ONOFF_MASK);
	switch (ms)
	{
		case SERVERD_ON | SERVERD_DUSK:
		case SERVERD_ON | SERVERD_NIGHT:
		case SERVERD_ON | SERVERD_DAWN:
			measurementIntervalActual->setValueInteger (measurementIntervalOn->getValueInteger ());
			break;
		default:
			if (measurementBoost->getValueBool () == true)
				measurementIntervalActual->setValueInteger (measurementIntervalOn->getValueInteger ());
				
			else
				measurementIntervalActual->setValueInteger (measurementIntervalStandard->getValueInteger ());
			break;
	}

	deleteTimers (EVENT_CLOUD_MEASUREMENT);
	addTimer (measurementIntervalActual->getValueInteger (), new rts2core::Event (EVENT_CLOUD_MEASUREMENT, this));


	// Solving heating separately...
	if (heatInterval->getValueInteger () > 0 && heatDuration->getValueInteger () > 0)
	{
		ms = new_state & SERVERD_STATUS_MASK;
		switch (ms)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				deleteTimers (EVENT_CLOUD_HEATER);
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

int Cloud2::processOption (int in_opt)
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
		case OPT_RAINDET_DEVICE:
			rainDetectorDevice = optarg;
			break;
		case OPT_RAINDET_VARIABLE:
			rainDetectorVariable = optarg;
			break;
		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}

int Cloud2::willConnect (rts2core::NetworkAddress * in_addr)
{
  if (in_addr->getType () == DEVICE_TYPE_SENSOR && in_addr->isAddress (rainDetectorDevice)) {
      logStream (MESSAGE_DEBUG) << "Cloud2::willConnect to DEVICE_TYPE_SENSOR: "<< rainDetectorDevice << ", variable: "<< rainDetectorVariable<< sendLog;
      return 1;
    }
    return SensorWeather::willConnect (in_addr);
}


int Cloud2::initHardware ()
{
	int ret;

	if (rainDetectorDevice == NULL || rainDetectorVariable == NULL )
	{
		logStream (MESSAGE_ERROR) << "Parameters --rain-detector and --rain-variable must be set explicitly from command-line!" << sendLog;
		return -1;
	}

	mrakConn = new rts2core::ConnSerial (device_file, this, rts2core::BS2400, rts2core::C8, rts2core::NONE, 40);
	mrakConn->setDebug ();
	ret = mrakConn->init ();
	if (ret)
		return ret;
	
	mrakConn->flushPortIO ();

	if (!std::isnan (triggerGood->getValueDouble ()))
		setWeatherState (false, "TRIGGOOD unspecified");
	return 0;
}

int Cloud2::info ()
{
	int ret;
	ret = readSensor (true);
	if (ret)
	{
		if (getLastInfoTime () > 60)
			setWeatherTimeout (60, "cannot read data from device");
		return -1;
	}

	if (!std::isnan (tempDiff->getValueDouble ()) && tempDiff->getNumMes () >= numVal->getValueInteger ())
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
			}
			valueGood (tempDiff);
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

void Cloud2::valueChanged (rts2core::Value *value)
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
	SensorWeather::valueChanged (value);
}

void Cloud2::usage ()
{
	std::cout << "  " << getAppName () << " -f /dev/ttyUSB0 --heat-interval 120 --heat-duration 10 --rain-detector RAIN --rain-variable rain -g 9.0 -b 7.0 --server localhost -d CLOUD" << std::endl;
			
	std::cout << "Definition:" << std::endl
	<< "  TEMP_DIFF = (temp_in_coeff * TEMP_IN) + (temp_amb_coeff * TEMP_AMB) - TEMP_SKY" << std::endl;
}


int main (int argc, char **argv)
{
	Cloud2 device = Cloud2 (argc, argv);
	return device.run ();
}
