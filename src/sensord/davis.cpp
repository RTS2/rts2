/* 
 * Davis weather sensor.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "davis.h"
#include "davisudp.h"

// we should get packets every minute; 5 min timeout, as data from meteo station
// aren't as precise as we want and we observe dropouts quite often
#define BART_WEATHER_TIMEOUT        360

// how long after weather was bad can weather be good again; in
// seconds
#define BART_BAD_WEATHER_TIMEOUT    360
#define BART_BAD_WINDSPEED_TIMEOUT  360
#define BART_CONN_TIMEOUT           360

#define DEF_WEATHER_TIMEOUT 600

#define OPT_UDP             OPT_LOCAL + 1001

using namespace rts2sensor;

int
Davis::processOption (int _opt)
{
	switch (_opt)
	{
		case 'b':
			cloud_bad = atof (optarg);
			break;
		case OPT_UDP:
			udpPort->setValueInteger (atoi (optarg));
			break;
		default:
			return Rts2DevSensor::processOption (_opt);
	}
	return 0;
}

int
Davis::init ()
{
	int ret;
	ret = Rts2DevSensor::init ();
	if (ret)
		return ret;

	weatherConn = new DavisUdp (udpPort->getValueInteger (), BART_WEATHER_TIMEOUT,
		BART_CONN_TIMEOUT,
		BART_BAD_WEATHER_TIMEOUT,
		BART_BAD_WINDSPEED_TIMEOUT, this);
	weatherConn->init ();
	addConnection (weatherConn);

	return 0;
}


Davis::Davis (int argc, char **argv)
:Rts2DevSensor (argc, argv)
{
	createValue (temperature, "DOME_TMP", "temperature in degrees C");
	createValue (humidity, "DOME_HUM", "(outside) humidity");
	createValue (rain, "RAIN", "whenever is raining");
	rain->setValueInteger (1);
	createValue (windspeed, "WINDSPED", "windspeed");
	// as soon as we get update from meteo, we will solve it. We have rain now, so dome will remain closed at start

	maxWindSpeed = 50;
	maxPeekWindspeed = 50;

	weatherConn = NULL;
	cloud_bad = 0;

	createValue (cloud, "CLOUD_S", "cloud sensor value");

	createValue (nextOpen, "next_open", "time when we can next time open dome",
		false);

	time_t nextGoodWeather;
	time (&nextGoodWeather);
	nextGoodWeather += DEF_WEATHER_TIMEOUT;

	nextOpen->setValueTime (nextGoodWeather);

	createValue (udpPort, "udp-port", "port for UDP connection from meteopoll", false);
	udpPort->setValueInteger (1500);

	addOption ('W', "max_windspeed", 1, "maximal allowed windspeed (in km/h)");
	addOption ('P', "max_peek_windspeed", 1,
		"maximal allowed windspeed (in km/h");
	addOption (OPT_UDP, "udp-port", 1, "UDP port for incoming weather connections");
}

void
Davis::setWeatherTimeout (time_t wait_time)
{
	time_t next;
	time (&next);
	next += wait_time;
	if (next > nextOpen->getValueInteger ())
		nextOpen->setValueTime (next);
	setWeatherState (false);
}


int
main (int argc, char **argv)
{
	Davis device = Davis (argc, argv);
	return device.run ();
}
