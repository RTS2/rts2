/* 
 * Sensor for Fine offset weather stations (WH1080, WH1081, W-8681, WH3080, WH3081 etc.)
 * Copyright (C) 2016 Petr Kubanek, Insitute of Physics <kubanek@fzu.cz>
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

#include <usb.h>

#define OPT_WIND_BAD        OPT_LOCAL + 370
#define OPT_PEEKWIND_BAD    OPT_LOCAL + 371
#define OPT_HUM_BAD         OPT_LOCAL + 372

namespace rts2sensord
{

/**
 * Class for FOWS USB connected weather station.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class FOWS:public SensorWeather
{
	public:
		FOWS (int argc, char **argv);
		virtual ~FOWS ();

		virtual int info ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();
	private:
		struct usb_dev_handle *devh;

		// values with FOWS data
		rts2core::ValueFloat *barometer;
		rts2core::ValueFloat *insideTemp;
		rts2core::ValueFloat *insideHumidity;
		rts2core::ValueFloat *outsideTemp;
		rts2core::ValueFloat *windSpeed;
		rts2core::ValueInteger *windDirection;
		rts2core::ValueFloat *wind10min;
		rts2core::ValueFloat *wind2min;
		rts2core::ValueFloat *wind10mingust;
		rts2core::ValueInteger *wind10mingustDirection;
		rts2core::ValueFloat *dewPoint;
		rts2core::ValueFloat *outsideHumidity;
		rts2core::ValueFloat *rainRate;
		rts2core::ValueInteger *uvIndex;
		rts2core::ValueInteger *stormRain;

		rts2core::ValueInteger *maxWindSpeed;
                rts2core::ValueInteger *maxPeekWindSpeed;
                rts2core::ValueInteger *maxHumidity;
		
		/**
		 * Check received buffer for CRC
		 */
		bool checkCrc ();

		/**
		 * Process received data from LOOP command.
		 */
		void processData ();

		/**
		 * Ckeck good/bad weather alarms
		 */
		void checkTriggers ();
};

}


using namespace rts2sensord;

FOWS::FOWS (int argc, char **argv):SensorWeather (argc, argv)
{
	maxHumidity = NULL;
	maxWindSpeed = NULL;
	maxPeekWindSpeed = NULL;

	createValue (barometer, "PRESSURE", "[hPa] barometer reading", false);
	createValue (insideTemp, "TEMP_IN", "[C] inside temperature", false);
	createValue (insideHumidity, "HUM_IN", "[%] inside humidity", false);
	createValue (outsideTemp, "TEMP_OUT", "[C] outside temperature", false);
	createValue (windSpeed, "WIND", "[m/s] wind speed", false);
	createValue (windDirection, "WIND_DIR", "wind direction", false, RTS2_DT_DEGREES);
	createValue (outsideHumidity, "HUM_OUT", "[%] outside humidity", false);
	createValue (rainRate, "RAINRT", "[mm/hour] rain rate", false);
	createValue (uvIndex, "UVINDEX", "UV index", false);
	createValue (stormRain, "RAINST", "[mm] storm rain", false);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSBn)");
	addOption (OPT_WIND_BAD, "wind-bad", 1, "wind speed bad trigger point");
        addOption (OPT_PEEKWIND_BAD, "peekwind-bad", 1, "peek wind speed bad trigger point");
        addOption (OPT_HUM_BAD, "hum-bad", 1, "humidity bad trigger point");

}

FOWS::~FOWS ()
{
	int ret = usb_release_interface(devh, 0);
	logStream (MESSAGE_DEBUG) << "usb_release_interface ret:" << ret << sendLog;
	ret = usb_close(devh);
	logStream (MESSAGE_DEBUG) << "usb_close ret:" << ret << sendLog;
}

int FOWS::info ()
{
        // do not send infotime..
	return 0;
}

int FOWS::processOption (int opt)
{
	switch (opt)
	{
		case OPT_WIND_BAD:
			createValue (maxWindSpeed, "MAX_WIND_TRIGGER", "[m/s] maximum wind speed", false);
			maxWindSpeed->setValueInteger (atoi(optarg));
			logStream (MESSAGE_INFO) << "DAVIS max wind speed:" << maxWindSpeed << sendLog;
                        break;
                case OPT_PEEKWIND_BAD:
			createValue (maxPeekWindSpeed, "MAX_PEEKWIND_TRIGGER", "[m/s] maximum peek wind speed", false);
			maxPeekWindSpeed->setValueInteger (atoi(optarg));
			logStream (MESSAGE_INFO) << "DAVIS max peek wind speed:" << maxPeekWindSpeed << sendLog;
                        break;
                case OPT_HUM_BAD:
			createValue (maxHumidity, "MAX_HUM_TRIGGER", "[%] maximum humidity", false);
                        maxHumidity->setValueInteger (atoi(optarg));
			logStream (MESSAGE_INFO) << "DAVIS max humidity:" << maxHumidity << sendLog;
                        break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int FOWS::initHardware ()
{
}

bool FOWS::isGoodWeather ()
{
	return SensorWeather::isGoodWeather ();
}

void FOWS::checkTriggers ()
{
        if (maxHumidity && outsideHumidity->getValueFloat () >= maxHumidity->getValueInteger ())
        {
         	setWeatherTimeout (600, "humidity has crossed alarm limits");
                valueError (outsideHumidity);
        }
        else
	{
        	valueGood (outsideHumidity);
	}
	
	if (maxWindSpeed && wind2min->getValueFloat () >= maxWindSpeed->getValueInteger ())
        {
		setWeatherTimeout (300, "high wind");
		valueError (windSpeed);
	}
	else
	{
		valueGood (windSpeed);
        }

	if (maxPeekWindSpeed && wind10mingust->getValueFloat () >= maxPeekWindSpeed->getValueInteger ())
        {
		setWeatherTimeout (300, "high peek wind");
		valueError (wind10mingust);
	}
	else
	{
		valueGood (wind10mingust);
        }
	
	if (rainRate->getValueFloat () > 0)
	{
        	setWeatherTimeout (1200, "raining (rainrate)");
                valueError (rainRate);
	}
        else
	{
        	valueGood (rainRate);
	}

}


int main (int argc, char **argv)
{
	FOWS device (argc, argv);
	return device.run ();
}
