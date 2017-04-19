/* 
 * Sensor daemon for Automated Weather Station by Jakub Kakona & Jan Chroust. 
 * This driver is related to specific version, developed for telescopes BART & D50.
 * Based on Sensor4 driver for Mrakomer device by Martin Kakona.
 * All their devices can be found here: http://www.mlab.cz/ .
 *
 * Copyright (C) 2008-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2009 Martin Jelinek
 * Copyright (C) 2013 Jan Strobl
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

#define OPT_WIND_TRIGBAD	OPT_LOCAL + 360
#define OPT_WIND_TRIGGOOD	OPT_LOCAL + 361
#define OPT_HUM_TRIGBAD		OPT_LOCAL + 362
#define OPT_HUM_TRIGGOOD	OPT_LOCAL + 363
#define OPT_AWS_LOCATION	OPT_LOCAL + 364

namespace rts2sensord
{

/**
 * Class for AWS-mlab.
 *
 * @author Jan Strobl
 */
class AWSmlab: public SensorWeather
{
	public:
		AWSmlab (int in_argc, char **in_argv);
		virtual ~AWSmlab (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int initHardware ();

		virtual int info ();

	private:
		char *device_file;
		rts2core::ConnSerial *AWSConn;

		rts2core::ValueDoubleStat *tempInSimple;
		rts2core::ValueDoubleStat *tempIn;
		rts2core::ValueDoubleStat *humIn;
		rts2core::ValueDoubleStat *tempOut;
		rts2core::ValueDoubleStat *humOut;
		rts2core::ValueDoubleStat *tempPressure;
		rts2core::ValueDoubleStat *pressure;
		rts2core::ValueDoubleStat *windSpeed;
		rts2core::ValueDoubleStat *windSpeedMaxPeak;

		// use this values only for logging to detect if we reported trips
		double lastHumOut, lastWindSpeed, lastWindSpeedMaxPeak;

		rts2core::ValueInteger *numVal;

		rts2core::ValueDouble *triggerWindBad;
		rts2core::ValueDouble *triggerWindGood;
		rts2core::ValueDouble *triggerHumBad;
		rts2core::ValueDouble *triggerHumGood;

		double triggerWindBadPO;
		double triggerWindGoodPO;
		double triggerHumBadPO;
		double triggerHumGoodPO;

		rts2core::ValueInteger *numberMes;

		rts2core::ValueBool *sensorTempEnable;
		rts2core::ValueBool *sensorTempHum1Enable;
		rts2core::ValueBool *sensorTempHum2Enable;
		rts2core::ValueBool *sensorBarEnable;
		rts2core::ValueBool *sensorAnemoEnable;


		/**
		 * Read sensor values.
		 */
		int readSensor (bool update);
};

};

using namespace rts2sensord;

int AWSmlab::readSensor (bool update)
{
	int ret;
	char buf[128];
	ret = AWSConn->writeRead ("s", 1, buf, 80, '\r');

	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	char *buf_start = buf;

	if (buf[0] == '\n')
		buf_start++;

	// parse response
	float temp0, temp1, temp2, hum1, hum2, temp_bar, pressure_bar, anemo, anemo_peak;
	int tno;
	char checksum;
	if (strncmp (buf_start, "$AWS0.2", 7) == 0)
	{
		int x = sscanf (buf_start + 8, "%d %f %f %f %f %f %f %f %f %f *%2hhx", &tno, &temp0, &temp1, &hum1, &temp2, &hum2, &temp_bar, &pressure_bar, &anemo, &anemo_peak, &checksum);
		if (x != 11) 
		{
			logStream (MESSAGE_ERROR) << "cannot parse reply from AWS, reply was: '" << buf << "', return " << x << sendLog;
			return -1;
		}
	}
	else
	{
		logStream (MESSAGE_ERROR) << "invalid AWS version - supported are only AWS0.2: " << buf << sendLog;
	}

	// compute checksum
	char ch = 0;
	for (char *bs = buf_start + 1; *bs && *bs != '*'; bs++)
		ch ^= *bs;
	if (checksum != ch)
		logStream (MESSAGE_ERROR) << "invalid checksum - expected " << checksum << ", calculated " << ch << sendLog;

	if (update == false)
		return 0;

	if (sensorTempEnable->getValueBool())
	{
		if (temp0 == -27315)
			temp0 = NAN;
		else
			temp0 /= 100.0;
		tempInSimple->addValue (temp0, numVal->getValueInteger());
		tempInSimple->calculate ();
	}

	if (sensorTempHum1Enable->getValueBool())
	{
		if (hum1 > 110.0)
		{
			temp1 = NAN;
			hum1 = NAN;
		}
		tempIn->addValue (temp1, numVal->getValueInteger());
		humIn->addValue (hum1, numVal->getValueInteger());
		tempIn->calculate ();
		humIn->calculate ();
	}
	
	if (sensorTempHum2Enable->getValueBool())
	{
		if (hum2 > 110.0)
		{
			temp2 = NAN;
			hum2 = NAN;
		}
		tempOut->addValue (temp2, numVal->getValueInteger());
		humOut->addValue (hum2, numVal->getValueInteger());
		tempOut->calculate ();
		humOut->calculate ();
	}
	
	if (sensorBarEnable->getValueBool())
	{
	
		if (pressure_bar == 0.0 || pressure_bar > 7000.0)
		{
			temp_bar = NAN;
			pressure_bar = NAN;
		}
		tempPressure->addValue (temp_bar, numVal->getValueInteger());
		pressure->addValue (pressure_bar, numVal->getValueInteger());
		tempPressure->calculate ();
		pressure->calculate ();
	}
	
	if (sensorAnemoEnable->getValueBool())
	{
	
		windSpeed->addValue (anemo, numVal->getValueInteger());
		windSpeedMaxPeak->addValue (anemo_peak, numVal->getValueInteger());
		windSpeed->calculate ();
		windSpeedMaxPeak->calculate ();
	}

	numberMes->setValueInteger (tno);

	return 0;
}

AWSmlab::AWSmlab (int in_argc, char **in_argv):SensorWeather (in_argc, in_argv)
{
	AWSConn = NULL;


	createValue (numVal, "num_stat", "number of measurements for weather statistic", false, RTS2_VALUE_WRITABLE);
	numVal->setValueInteger (6);

	createValue (numberMes, "number_mes", "number of measurements", false);

	createValue (sensorTempEnable, "SENSOR_TEMP_ENABLE", "enable/disable usage of simple temperature sensor", false);
	sensorTempEnable->setValueBool (true);
	createValue (sensorTempHum1Enable, "SENSOR_TEMPHUM1_ENABLE", "enable/disable usage of temperature/humidity sensor 1", false);
	sensorTempHum1Enable->setValueBool (true);
	createValue (sensorTempHum2Enable, "SENSOR_TEMPHUM2_ENABLE", "enable/disable usage of temperature/humidity sensor 2", false);
	sensorTempHum2Enable->setValueBool (true);
	createValue (sensorBarEnable, "SENSOR_BAR_ENABLE", "enable/disable usage of temperature/pressure sensor", false);
	sensorBarEnable->setValueBool (true);
	createValue (sensorAnemoEnable, "SENSOR_BAR_ANEMO_ENABLE", "enable/disable usage of anemometer sensor", false);
	sensorAnemoEnable->setValueBool (true);

	triggerWindBadPO = NAN;
	triggerWindGoodPO = NAN;
	triggerHumBadPO = NAN;
	triggerHumGoodPO = NAN;

	addOption ('f', NULL, 1, "serial port with AWS");

	addOption (OPT_WIND_TRIGBAD, "wind-trigbad", 1, "wind speed bad trigger point");
	addOption (OPT_WIND_TRIGGOOD, "wind-triggood", 1, "wind speed good trigger point");
	addOption (OPT_HUM_TRIGBAD, "hum-trigbad", 1, "humidity bad trigger point");
	addOption (OPT_HUM_TRIGGOOD, "hum-triggood", 1, "humidity good trigger point");

	addOption (OPT_AWS_LOCATION, "location", 1, "limit set of sensors according to usage on telescope (0=generic [default], 1=BART, 2=D50");

	setIdleInfoInterval (20);
}

AWSmlab::~AWSmlab (void)
{
	delete AWSConn;
}

int AWSmlab::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		case OPT_WIND_TRIGBAD:
			triggerWindBadPO = atof (optarg);
			break;
		case OPT_WIND_TRIGGOOD:
			triggerWindGoodPO = atof (optarg);
			break;
		case OPT_HUM_TRIGBAD:
			triggerHumBadPO = atof (optarg);
			break;
		case OPT_HUM_TRIGGOOD:
			triggerHumGoodPO = atof (optarg);
			break;
		case OPT_AWS_LOCATION:
			switch (atoi (optarg))
			{
				case 1:	// BART
					sensorTempEnable->setValueBool (false);
					sensorTempHum1Enable->setValueBool (true);
					sensorTempHum2Enable->setValueBool (true);
					sensorBarEnable->setValueBool (true);
					sensorAnemoEnable->setValueBool (true);
					break;
				case 2: // D50
					sensorTempEnable->setValueBool (true);
					sensorTempHum1Enable->setValueBool (true);
					sensorTempHum2Enable->setValueBool (false);
					sensorBarEnable->setValueBool (false);
					sensorAnemoEnable->setValueBool (false);
					break;
				default:
					sensorTempEnable->setValueBool (true);
					sensorTempHum1Enable->setValueBool (true);
					sensorTempHum2Enable->setValueBool (true);
					sensorBarEnable->setValueBool (true);
					sensorAnemoEnable->setValueBool (true);
			}
			break;
		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}

int AWSmlab::initHardware ()
{
	if (sensorTempEnable->getValueBool())
	{
		createValue (tempInSimple, "TEMP_SIMPLE", "simple temperature sensor (located e.g. in rack)", false);
	}
	if (sensorTempHum1Enable->getValueBool())
	{
		createValue (tempIn, "TEMP_DOME", "temperature inside dome", true);
		createValue (humIn, "HUM_DOME", "humidity inside dome", true);
	}
	if (sensorTempHum2Enable->getValueBool())
	{
		createValue (tempOut, "TEMP_OUT", "temperature outside", true);
		createValue (humOut, "HUM_OUT", "humidity outside", true);
		createValue (triggerHumBad, "HUM_TRIGBAD", "if HUM_OUT gets above this value, set bad weather", false, RTS2_VALUE_WRITABLE);
		triggerHumBad->setValueDouble (triggerHumBadPO);
		createValue (triggerHumGood, "HUM_TRIGGOOD", "if HUM_OUT drops below this value, drop bad weather flag", false, RTS2_VALUE_WRITABLE);
		triggerHumGood->setValueDouble (triggerHumGoodPO);
	}
	if (sensorBarEnable->getValueBool())
	{
		createValue (tempPressure, "TEMP_BAR", "temperature inside, barometer sensor", false);
		createValue (pressure, "PRESSURE", "atmospheric pressure", true);
	}
	if (sensorAnemoEnable->getValueBool())
	{
		createValue (windSpeed, "WIND_SPEED", "average wind speed during measure interval, [m/s]", true);
		createValue (windSpeedMaxPeak, "WIND_SPEED_PEAK", "maximal peak wind speed during measure interval, [m/s]", true);
		createValue (triggerWindBad, "WIND_TRIGBAD", "if WIND_SPEED gets above this value, set bad weather", false, RTS2_VALUE_WRITABLE);
		triggerWindBad->setValueDouble (triggerWindBadPO);
		createValue (triggerWindGood, "WIND_TRIGGOOD", "if WIND_SPEED drops below this value, drop bad weather flag", false, RTS2_VALUE_WRITABLE);
		triggerWindGood->setValueDouble (triggerWindGoodPO);
	}

	AWSConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 10);
	AWSConn->setDebug ();
	int ret = AWSConn->init ();
	if (ret)
		return ret;
	
	AWSConn->flushPortIO ();

	//if (!std::isnan (triggerGood->getValueDouble ()))
	//	setWeatherState (false, "TRIGGOOD unspecified");
	return 0;
}

int AWSmlab::info ()
{
	int ret;
	ret = readSensor (true);
	if (ret)
	{
		if (getLastInfoTime () > 60)
			setWeatherTimeout (85, "cannot read data from device");
		return -1;
	}

	if (sensorTempHum2Enable->getValueBool() && !std::isnan (triggerHumGood->getValueDouble ()) && !std::isnan (triggerHumBad->getValueDouble ()))
	{
		// humidity checks...
		if (humOut->getNumMes () >= numVal->getValueInteger ())
		{
			// trips..
			if (humOut->getValueDouble () >= triggerHumBad->getValueDouble ())
			{
				if (getWeatherState () == true)
				{
					logStream (MESSAGE_INFO) << "setting weather to bad. humOut: " << humOut->getValueDouble ()
						<< " trigger: " << triggerHumBad->getValueDouble ()
						<< sendLog;
				}
				setWeatherTimeout (300, "humidity has crossed HUM_TRIGBAD");
				valueError (humOut);
			}
			else if (humOut->getValueDouble () <= triggerHumGood->getValueDouble ())
			{
				if (getWeatherState () == false && lastHumOut > triggerHumGood->getValueDouble ())
				{
					logStream (MESSAGE_INFO) << "setting weather to good. humOut: " << humOut->getValueDouble ()
						<< " trigger: " << triggerHumGood->getValueDouble ()
						<< sendLog;
				}
				valueGood (humOut);
			}
			// gray zone - if it's bad weather, keep it bad
			else if (getWeatherState () == false)
			{
				setWeatherTimeout (300, "humidity has not faded below HUM_TRIGGOOD");
				valueWarning (humOut);
			}
			else
			{
				valueGood (humOut);
			}
		}
		else
		{
			setWeatherTimeout (85, "waiting for enough measurements from sensors");
		}
		// record last value
		lastHumOut = humOut->getValueDouble ();

	}
	if (sensorAnemoEnable->getValueBool() && !std::isnan (triggerWindGood->getValueDouble ()) && !std::isnan (triggerWindBad->getValueDouble ()))
	{
		// wind speed (average and peek) checks...
		if (windSpeed->getNumMes () >= numVal->getValueInteger ())
		{
			// trips..
			if (windSpeed->getValueDouble () >= triggerWindBad->getValueDouble ())
			{
				if (getWeatherState () == true)
				{
					logStream (MESSAGE_INFO) << "setting weather to bad. windSpeed: " << windSpeed->getValueDouble ()
						<< " trigger: " << triggerWindBad->getValueDouble () 
						<< sendLog;
				}
				setWeatherTimeout (300, "wind has crossed WIND_TRIGBAD");
				valueError (windSpeed);
			}
			else if (windSpeed->getValueDouble () <= triggerWindGood->getValueDouble ())
			{
				if (getWeatherState () == false && lastWindSpeed > triggerWindGood->getValueDouble ())
				{
					logStream (MESSAGE_INFO) << "setting weather to good. windSpeed: " << windSpeed->getValueDouble ()
						<< " trigger: " << triggerWindGood->getValueDouble ()
						<< sendLog;
				}
				valueGood (windSpeed);
			}
			// gray zone - if it's bad weather, keep it bad
			else if (getWeatherState () == false)
			{
				setWeatherTimeout (300, "wind has not faded below WIND_TRIGGOOD");
				valueWarning (windSpeed);
			}
			else
			{
				valueGood (windSpeed);
			}

			// trips..
			if (windSpeedMaxPeak->getMax () >= triggerWindBad->getValueDouble () * 1.5)
			{
				if (getWeatherState () == true)
				{
					logStream (MESSAGE_INFO) << "setting weather to bad. windSpeedMaxPeak: " << windSpeedMaxPeak->getMax ()
						<< " trigger: " << triggerWindBad->getValueDouble () * 1.5 
						<< sendLog;
				}
				setWeatherTimeout (300, "wind peak crossed WIND_TRIGBAD * 1.5");
				valueError (windSpeedMaxPeak);
			}
			else if (windSpeedMaxPeak->getMax () <= triggerWindGood->getValueDouble () * 1.5)
			{
				if (getWeatherState () == false && lastWindSpeedMaxPeak > triggerWindGood->getValueDouble () * 1.5)
				{
					logStream (MESSAGE_INFO) << "setting weather to good. windSpeedMaxPeak: " << windSpeedMaxPeak->getMax ()
						<< " trigger: " << triggerWindGood->getValueDouble () * 1.5
						<< sendLog;
				}
				valueGood (windSpeedMaxPeak);
			}
			// gray zone - if it's bad weather, keep it bad
			else if (getWeatherState () == false)
			{
				setWeatherTimeout (300, "wind peak has not faded below WIND_TRIGGOOD * 1.5");
				valueWarning (windSpeedMaxPeak);
			}
			else
			{
				valueGood (windSpeedMaxPeak);
			}
		}
		else
		{
			setWeatherTimeout (85, "waiting for enough measurements from sensors");
		}
		// record last value
		lastWindSpeed= windSpeed->getValueDouble ();
		lastWindSpeedMaxPeak = windSpeedMaxPeak->getMax ();
	}

	return SensorWeather::info ();
}

int main (int argc, char **argv)
{
	AWSmlab device (argc, argv);
	return device.run ();
}

