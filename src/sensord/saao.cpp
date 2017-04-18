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
class SAAO:public Sensor
{
	public:
		SAAO (int argc, char **argv);
	
	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

	private:
		rts2core::ValueFloat *temperature;
		rts2core::ValueFloat *humidity;
		rts2core::ValueFloat *windDirection;
		rts2core::ValueFloat *windSpeed;

		const char *url;
};

}

using namespace rts2sensord;

SAAO::SAAO (int argc, char **argv):Sensor (argc, argv)
{
	createValue (temperature, "TEMPERATURE", "[C] ambient temperature", true);
	createValue (humidity, "HUMIDITY", "[%] relative humidity", true);
	createValue (windDirection, "WIND_DIR", "wind direction", true, RTS2_DT_DEGREES);
	createValue (windSpeed, "WINDSPEED", "[km/h] wind speed", true);

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
				windSpeed->setValueFloat (strtod (data, &endptr));
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
				data = endptr;
			}
			else if (strncmp (data, "H,", 2) == 0)
			{
				data += 3;
				humidity->setValueFloat (strtod (data, &endptr));
				data = endptr;
			}
		}
		else
		{
			data++;
		}
	}

	free (reply);
	setInfoTime (&wsdate);
	return 0;
}

int main (int argc, char **argv)
{
	SAAO device (argc, argv);
	return device.run ();
}
