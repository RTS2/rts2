/* 
 * Driver for Boltwood Cloud Sensor II.
 * Copyright (C) 2011 Matthew Thompson <chameleonator@gmail.com>
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

#include <sys/ioctl.h>
#include <weather/BWCloudSensorII.h>


namespace rts2sensord
{

class CloudSensorII:public SensorWeather, public BWCSII_Listener
{

	public:
		CloudSensorII (int in_argc, char **in_argv);
		virtual ~ CloudSensorII (void);

		virtual void postEvent (rts2core::Event *event);

		//virtual int changeMasterState (int new_state);

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual void csII_reportSensorData(BWCloudSensorII *csII,
                        			   const BWCSII_reportSensorData &report);

	protected:
		virtual int init ();

		virtual int info ();

		//virtual bool isGoodWeather ();

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		virtual void valueChanged (rts2core::Value *value);

		virtual int processOption (int in_opt);

	private:

		rts2core::ValueBool *closeRequested;

		rts2core::ValueString *cloudCond;
		rts2core::ValueString *windCond;
		rts2core::ValueString *rainCond;
		rts2core::ValueString *skyCond;
		rts2core::ValueString *dayCond;

		rts2core::ValueDouble *windSpeed;

		rts2core::ValueDouble *ambientTemperature;
		rts2core::ValueDouble *skyMinusAmbientTemperature;

		rts2core::ValueBool *recentlyWet;
		rts2core::ValueString *wetSensor;
		rts2core::ValueString *rainSensor;

		rts2core::ValueInteger *relativeHumidityPercentage;
		rts2core::ValueDouble *dewPointTemperature;

		const char *device_file;

		BWCloudSensorII *csII;
};

}

using namespace rts2sensord;


int CloudSensorII::info ()
{
	csII->checkForData();

	if (closeRequested->getValue ())
	{
		setWeatherState(false, "Cloud Sensor II reports dome should be closed");
		setWeatherTimeout (300, "weather is bad, sleeping 300 seconds");
	}
	else
	{
		setWeatherState(true, "Cloud Sensor II reports dome should be open");
	}
	

	return SensorWeather::info ();
}


void CloudSensorII::csII_reportSensorData(BWCloudSensorII *csII,
                                          const BWCSII_reportSensorData &report)
{
	closeRequested->setValueBool(report.roofCloseRequested);
	
	const char *cloud, *wind, *rain, *sky, *day;

	switch (report.cloudCond)
	{
		case 0: cloud = "unknown"; break;
		case 1: cloud = "clear"; break;
		case 2: cloud = "cloudy"; break;
		case 3: cloud = "very cloudy"; break;
		default: cloud = "NA"; break;
	}

	switch (report.windCond)
	{
		case 0: wind = "unknown"; break;
		case 1: wind = "ok"; break;
		case 2: wind = "windy"; break;
		case 3: wind = "very windy"; break;
		default: wind = "NA"; break;
	}

	switch (report.rainCond)
    {
		case 0: rain = "unknown";break;
		case 1: rain = "none"; break;
		case 2: rain = "recent"; break;
		case 3: rain = "raining"; break;
		default: rain = "NA"; break;
	}

	switch (report.skyCond)
	{
		case 1: sky = "clear"; break;
		case 2: sky = "cloudy"; break;
		case 3: sky = "very cloudy"; break;
		case 4: sky = "wet"; break;
		default: sky = "NA"; break;
	}

	switch (report.dayCond)
	{
		case 1: day = "night"; break;
		case 2: day = "twilight"; break;
		case 3: day = "day"; break;
		default: day = "NA"; break;
	}

	cloudCond->setValueString (cloud);
	windCond->setValueString (wind);
	rainCond->setValueString (rain);
	skyCond->setValueString (sky);
	dayCond->setValueString (day);

	windSpeed->setValueDouble (report.windSpeed);


	ambientTemperature->setValueDouble (report.ambientTemperature);
	skyMinusAmbientTemperature->setValueDouble (report.skyMinusAmbientTemperature);
	
	recentlyWet->setValueBool (report.recentlyWet);

	const char *wetsensor, *rainsensor;

	switch (report.wetSensor)
	{
		case 'N': wetsensor = "dry"; break;
		case 'W': wetsensor = "wet"; break;
		case 'w': wetsensor = "recently wet"; break;
		default: wetsensor = "NA"; break;
	}

	switch (report.rainSensor)
	{
		case 'N': rainsensor = "none"; break;
		case 'R': rainsensor = "drops now"; break;
		case 'r': rainsensor = "drops recently"; break;
		default: rainsensor = "NA"; break;
	}

	wetSensor->setValueString (wetsensor);
	rainSensor->setValueString (rainsensor);

	relativeHumidityPercentage->setValueInteger (report.relativeHumidityPercentage);
	dewPointTemperature->setValueDouble (report.dewPointTemperature);
}


CloudSensorII::CloudSensorII (int in_argc, char **in_argv)
:SensorWeather (in_argc, in_argv)
{
	device_file = "/dev/ttyUSB0";

	addOption ('f', "device_file", 1, "device file (usually /dev/ttyUSBx)");

	csII = new BWCloudSensorII(this, "RTS2", "/tmp",
			BWCloudSensorII::DF_SERIAL_LOG);

	createValue (closeRequested, "CLOSE_REQUEST", "dome should be closed", false);

	createValue (cloudCond, "CLOUD_COND", "cloud conditions", true);
	createValue (windCond, "WIND_COND", "wind conditions", true);
	createValue (rainCond, "RAIN_COND", "rain conditions", true);
	createValue (skyCond, "SKY_COND", "sky conditions", true);
	createValue (dayCond, "DAY_COND", "day conditions", true);

	createValue (windSpeed, "WIND_SPEED", "wind speed in km/hr", true);

	createValue (ambientTemperature, "TEMP_AMB", "ambient temperature in deg C", true);
	createValue (skyMinusAmbientTemperature, "TEMP_SKY-AMB", "sky-ambient temperature in deg C", false);
	createValue (relativeHumidityPercentage, "HUMID_PERCENT", "relative humidity in %", true);
	createValue (dewPointTemperature, "TEMP_DEW", "dew point temperature in deg C", true);

	createValue (recentlyWet, "RECENT_WET", "recently wet (readings just after wet are suspect)", false);
	createValue (wetSensor, "SENSOR_WET", "wetness sensor's wet detection", false);
	createValue (rainSensor, "SENSOR_RAIN", "rain sensor's raindrop detection", false);

	setIdleInfoInterval (30);		
}


CloudSensorII::~CloudSensorII (void)
{
	csII->close();
	delete csII;
}


int CloudSensorII::commandAuthorized (rts2core::Connection *conn)
{
	return SensorWeather::commandAuthorized (conn);
}


void CloudSensorII::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
	}

	SensorWeather::postEvent (event);
}

/*
int CloudSensorII::changeMasterState (int new_state)
{
	int master = new_state & SERVERD_STATUS_MASK;
	switch (master)
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			break;
		default:
			break;
	}

	return SensorWeather::changeMasterState (new_state);
}
*/

int CloudSensorII::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;
	
	csII->open(device_file);	
	if (!csII->isOpen()) 
	{
		logStream (MESSAGE_ERROR)
			 << "Could not open sensor at "
                         << device_file << sendLog;
		return 1;
	}

	return 0;
}


/*
bool CloudSensorII::isGoodWeather ()
{
	return closeRequested->getValue();
}
*/


int CloudSensorII::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	return SensorWeather::setValue (old_value, new_value);
}


void CloudSensorII::valueChanged (rts2core::Value *value)
{
	SensorWeather::valueChanged (value);
}


int CloudSensorII::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return SensorWeather::processOption (in_opt);
	}
	return 0;
}


int main (int argc, char **argv)
{
	CloudSensorII device = CloudSensorII (argc, argv);
	return device.run ();
}

