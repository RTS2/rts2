/* 
 * Sensor for SAAO weather stations.
 * Copyright (C) 2017 Petr Kubanek <petr@kubanek.net>
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
#include "xmlrpc++/XmlRpc.h"

namespace rts2sensord
{

/**
 * Class for SAAO weather sensor.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class SAAO:public SensorWeather
{
	public:
		SAAO (int argc, char **argv);
	
	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual bool isGoodWeather ();

		int willConnect (rts2core::NetworkAddress * in_addr);

	private:
		void checkWeatherTimeout (float timeout, const char *msg);

		rts2core::ValueTime *lastGood;
		rts2core::ValueInteger *ignoreTime;
		rts2core::ValueFloat *temperature;
		rts2core::ValueFloat *humidity;
		rts2core::ValueFloat *windDirection;
		rts2core::ValueFloat *windSpeed;
		rts2core::ValueFloat *pressure;

		rts2core::ValueFloat *humidityLimit;
		rts2core::ValueFloat *windLimit;

		rts2core::ValueBool *okToOpen;

		const char *url;
};

}

using namespace rts2sensord;

SAAO::SAAO (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (lastGood, "good_till", "observing till at least", false);
	createValue (ignoreTime, "ignore_time", "[s] time when bad weather will be ignored", false, RTS2_VALUE_AUTOSAVE | RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL);
	ignoreTime->setValueInteger (1200);

	lastGood->setValueDouble (getNow () + ignoreTime->getValueInteger ());

	createValue (temperature, "TEMPERATURE", "[C] ambient temperature", true);
	createValue (humidity, "HUMIDITY", "[%] relative humidity", true, RTS2_DT_PERCENTS);
	createValue (windDirection, "WIND_DIR", "wind direction", true, RTS2_DT_DEGREES);
	createValue (windSpeed, "WINDSPEED", "[km/h] wind speed", true);
	createValue (pressure, "PRESSURE", "[mbar] atmospheric pressure", true);

	createValue (humidityLimit, "humidity_limit", "[%] relative humidity limit", false, RTS2_VALUE_WRITABLE);
	humidityLimit->setValueFloat (90);

	createValue (windLimit, "wind_limit", "[km/h] windspeed limit", false, RTS2_VALUE_WRITABLE);
	windLimit->setValueFloat (60);

	createValue (okToOpen, "OKTOOPEN", "OK to open", false);
	okToOpen->setValueBool (false);

	addOption ('u', NULL, 1, "URL to parse data");

	url = NULL;
}

int SAAO::processOption (int opt)
{
	switch (opt)
	{
		case 'u':
			url = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int SAAO::initHardware ()
{
	if (url == NULL)
	{
		logStream (MESSAGE_ERROR) << "please specify URL to retrieve with -u" << sendLog;
		return -1;
	}
	return 0;
}

int SAAO::info ()
{
	int ret;
	char *reply = NULL;
	int reply_length = 0;

	const char *_uri;

	const char *lcgot = "http://196.21.94.19/simple.html";

	XmlRpc::XmlRpcClient httpClient (lcgot, &_uri);

	ret = httpClient.executeGet (_uri, reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "cannot get " << lcgot << sendLog;
		return -1;
	}

	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "received:" << reply << sendLog;

//2017-04-18 21:52:36, CPT, OKTOOPEN, , T, 12.9, WS, 4.4, WD, 79, H, 44.3, DP, 1, PY, 0, BP, 647, WET, FALSE

	struct tm wsdate;

	char *data = strptime (reply, "%Y-%m-%d %H:%M:%S", &wsdate);

	int parsed = 0;

	rts2core::Connection *telConn = getOpenConnection (DEVICE_TYPE_MOUNT);

	while (*data)
	{
		char *endptr;
		if (*data == ',')
		{
			data++;
			while (*data && isspace (*data))
				data++;
			if (strncmp (data, "WS,", 3) == 0)
			{
				data += 4;
				windSpeed->setValueFloat (strtod (data, &endptr) * 3.6);
				if (windSpeed->getValueFloat () > windLimit->getValueFloat ())
				{
					checkWeatherTimeout (120, "wind over limit");
					valueError (windSpeed);
				}
				else
				{
					valueGood (windSpeed);
					parsed |= 0x01;
				}
				data = endptr;
			}
			else if (strncmp (data, "WD,", 3) == 0)
			{
				data += 4;
				windDirection->setValueFloat (strtod (data, &endptr));
				data = endptr;
			}
			else if (strncmp (data, "T,", 2) == 0)
			{
				data += 3;
				temperature->setValueFloat (strtod (data, &endptr));
				if (telConn)
					telConn->queCommand (new rts2core::CommandChangeValue (telConn->getOtherDevClient (), "AMBTEMP", '=', temperature->getValueFloat ()));
				data = endptr;
			}
			else if (strncmp (data, "H,", 2) == 0)
			{
				data += 3;
				humidity->setValueFloat (strtod (data, &endptr));

				if (humidity->getValueFloat () > humidityLimit->getValueFloat ())
				{
					checkWeatherTimeout (120, "humidity above limit");
					valueError (humidity);
				}
				else
				{
					valueGood (humidity);
					parsed |= 0x02;
				}
				if (telConn)
					telConn->queCommand (new rts2core::CommandChangeValue (telConn->getOtherDevClient (), "AMBHUMIDITY", '=', humidity->getValueFloat ()));
				data = endptr;
			}
			else if (strncmp (data, "BP,", 3) == 0)
			{
				data += 4;
				// pressure is in mmHg
				pressure->setValueFloat (1.333224 * strtod (data, &endptr));
				data = endptr;
			}
			else if (strncmp (data, "OKTOOPEN", 8) == 0)
			{
				data += 9;
				okToOpen->setValueBool (true);
				valueGood (okToOpen);
				parsed |= 0x04;
			}
		}
		else
		{
			data++;
		}
	}

	if (parsed == 0x07)
	{
		lastGood->setValueDouble (getNow () + ignoreTime->getValueInteger ());
	}
	else
	{
		okToOpen->setValueBool (false);
		checkWeatherTimeout (120, "not OK to open");
		valueError (okToOpen);
	}

	free (reply);
	setInfoTime (&wsdate);
	return 0;
}

void SAAO::checkWeatherTimeout (float timeout, const char *msg)
{
	time_t now;
	time (&now);
	if (lastGood->getValueDouble () > getNow ())
		return;
	setWeatherTimeout (timeout, msg);
}

bool SAAO::isGoodWeather ()
{
	time_t now;
	time (&now);
	if (lastGood->getValueDouble () > now)
		return true;

	if (getLastInfoTime () > 120)
	{
		checkWeatherTimeout (80, "unable to receive data");
		return false;
	}
	return SensorWeather::isGoodWeather ();
}

int SAAO::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_MOUNT)
		return 1;
	return rts2core::Device::willConnect (in_addr);
}

int main (int argc, char **argv)
{
	SAAO device (argc, argv);
	return device.run ();
}
