/* 
 * Sensor for Davis Vantage weather station
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#define EVENT_LOOP          RTS2_LOCAL_EVENT + 1601

namespace rts2sensord
{

/**
 * Class for Davis Vantage serial connected weather station.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Davis:public SensorWeather
{
	public:
		Davis (int argc, char **argv);
		virtual ~Davis ();

		virtual void addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set);
		virtual void selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set);

		virtual int info ();

		virtual void postEvent (rts2core::Event *event);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();
	private:
		char *device_file;
		rts2core::ConnSerial *davisConn;

		char dataBuff[101];
		int lastReceivedChar;

		// values with Davis data
		rts2core::ValueSelection *barTrend;
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
		rts2core::ValueInteger *rainRate;
		rts2core::ValueInteger *uvIndex;
		rts2core::ValueInteger *stormRain;
		
		/**
		 * Check received buffer for CRC
		 */
		bool checkCrc ();

		/**
		 * Process received data from LOOP command.
		 */
		void processData ();
};

}


using namespace rts2sensord;

Davis::Davis (int argc, char **argv):SensorWeather (argc, argv)
{
	device_file = NULL;
	davisConn = NULL;

	lastReceivedChar = 0;

	createValue (barTrend, "bar_trend", "barometric trend in last 3 hours", false);
	barTrend->addSelVal ("not known");
	barTrend->addSelVal ("Falling Rapidly");
	barTrend->addSelVal ("Falling Slowly");
	barTrend->addSelVal ("Steady");
	barTrend->addSelVal ("Rising Slowly");
	barTrend->addSelVal ("Rising Rapidly");

	createValue (barometer, "PRESSURE", "[hPa] barometer reading", false);
	createValue (insideTemp, "TEMP_IN", "[C] inside temperature", false);
	createValue (insideHumidity, "HUM_IN", "[%] inside humidity", false);
	createValue (outsideTemp, "TEMP_OUT", "[C] outside temperature", false);
	createValue (windSpeed, "WIND", "[m/s] wind speed", false);
	createValue (windDirection, "WIND_DIR", "wind direction", false, RTS2_DT_DEGREES);
	createValue (wind10min, "WIND10", "[m/s] average wind speed for last 10 minutes", false);
	createValue (wind2min, "WIND2", "[m/s] average wind speed for last 2 minutes", false);
	createValue (wind10mingust, "WIND10G", "[m/s] wind 10 minutes gust speed", false);
	createValue (wind10mingustDirection, "WIND10GD", "wind gust 10 minutes direction", false);
	createValue (dewPoint, "DEWPT", "[C] dew point", false);
	createValue (outsideHumidity, "HUM_OUT", "[%] outside humidity", false);
	createValue (rainRate, "RAINRT", "[mm/hour] rain rate", false);
	createValue (uvIndex, "UVINDEX", "UV index", false);
	createValue (stormRain, "RAINST", "[mm] storm rain", false);

	addOption ('f', NULL, 1, "serial port with the module (ussually /dev/ttyUSBn)");
}

Davis::~Davis ()
{
	delete davisConn;
}

void Davis::addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	davisConn->add (&read_set, &write_set, &exp_set);
	SensorWeather::addSelectSocks (read_set, write_set, exp_set);
}

void Davis::selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	if (davisConn->receivedData (&read_set))
	{
		int ret = davisConn->readPort (dataBuff + lastReceivedChar, 100 - lastReceivedChar);
		if (ret > 0)
		{
			// check that the first received character is ACK
			if (lastReceivedChar == 0 && dataBuff[0] != 0x06)
			{
				logStream (MESSAGE_ERROR) << "data buffer does not start with ACK" << sendLog;
				sleep (4);
				davisConn->flushPortIO ();
				davisConn->writePort ("LOOP 1\n", 7);
				return;
			}
			lastReceivedChar += ret;
			if (lastReceivedChar >= 100)
			{
				if (checkCrc ())
				{
					processData ();
                                        updateInfoTime ();
				}
				else
				{
					logStream (MESSAGE_ERROR) << "invalid CRC received!" << sendLog;
				}
				// make sure we start with empty received buffer
				lastReceivedChar = 0;
				addTimer (2, new rts2core::Event (EVENT_LOOP));
			}
		}
	}

	SensorWeather::selectSuccess (read_set, write_set, exp_set);
}

int Davis::info ()
{
        // do not send infotime..
	return 0;
}

void Davis::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_LOOP:
			if (lastReceivedChar == 0)
				davisConn->writePort ("LOOP 1\n", 7);
			break;
	}
	SensorWeather::postEvent (event);
}

int Davis::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int Davis::initHardware ()
{
	if (device_file == NULL)
	{
		logStream (MESSAGE_ERROR) << "you must specify device file (TTY port)" << sendLog;
		return -1;
	}

	davisConn = new rts2core::ConnSerial (device_file, this, rts2core::BS19200, rts2core::C8, rts2core::NONE, 30);
	int ret = davisConn->init ();
	if (ret)
		return ret;
	davisConn->setDebug (getDebug ());
	davisConn->writePort ('\r');
	ret = davisConn->readPort (dataBuff, 2);
	if (ret != 2)
	{
		logStream (MESSAGE_ERROR) << "Davis not connected" << sendLog;
	}
	if (dataBuff[0] != '\n' || dataBuff[1] != '\r')
	{
		logStream (MESSAGE_ERROR) << "invalid reply" << dataBuff[0] << dataBuff[1] << sendLog;
	}
	davisConn->writePort ("LOOP 1\n", 7);
	return 0;
}

bool Davis::isGoodWeather ()
{
	return SensorWeather::isGoodWeather ();
}

bool Davis::checkCrc ()
{
	unsigned short crc = 0;

static unsigned short crc_table [256] = {
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,  
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,  
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,  
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,  
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,  
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,  
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,  
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,  
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,  
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,  
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,  
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,  
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,  
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,  
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0xe70,  
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,  
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,  
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,  
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,  
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,  
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,  
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,  
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,  
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,  
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,  
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,  
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,  
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,  
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,  
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,  
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,  
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0,  
};

	for (int i = 1; i < lastReceivedChar; i++)
	{
		crc = crc_table [(crc >> 8) ^ (unsigned char) (dataBuff[i])] ^ (crc << 8);
	}

	// value should be 0 at the end..
	return crc == 0x0000;
}

void Davis::processData ()
{
	switch (dataBuff[4])
	{
		case -60:
			barTrend->setValueInteger (1);
			break;
		case -20:
			barTrend->setValueInteger (2);
			break;
		case 0:
			barTrend->setValueInteger (3);
			break;
		case 20:
			barTrend->setValueInteger (4);
			break;
		case 60:
			barTrend->setValueInteger (5);
			break;
		default:
			barTrend->setValueInteger (0);
	}
	float hg2mg = 33.8639;
	barometer->setValueFloat (hg2mg * *((float *) (dataBuff + 8 )));
	insideTemp->setValueFloat (fahrenheitToCelsius (*((int16_t *) (dataBuff + 10)) / 10.0));
	insideHumidity->setValueFloat(*((uint8_t *) (dataBuff + 12)));
	outsideTemp->setValueFloat (fahrenheitToCelsius (*((int16_t *) (dataBuff + 13)) / 10.0));

	windSpeed->setValueFloat(mphToMs (*((int16_t *) (dataBuff + 15))));
	windDirection->setValueInteger(*((uint16_t *) (dataBuff + 17)) * 0.1);
	wind10min->setValueFloat(*((uint8_t *) (dataBuff + 19)) * 0.1);
	wind2min->setValueFloat(*((uint8_t *) (dataBuff + 21)) * 0.1);
	wind10mingust->setValueFloat(*((uint8_t *) (dataBuff + 23)) * 0.1);
	wind10mingustDirection->setValueInteger(*((uint16_t *) (dataBuff + 25)));
	dewPoint->setValueFloat(fahrenheitToCelsius (*((int16_t *) (dataBuff + 31))));
	outsideHumidity->setValueFloat(*((uint8_t *) (dataBuff + 34)));

	float rainClicks2mm = 0.2;
	rainRate->setValueInteger(rainClicks2mm * *((float *) (dataBuff + 42 )));
	uvIndex->setValueInteger(*((uint8_t *) (dataBuff + 44)));	
	stormRain->setValueInteger(*((float *) (dataBuff + 47)));
}

int main (int argc, char **argv)
{
	Davis device (argc, argv);
	return device.run ();
}
