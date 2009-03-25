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

#define DEF_WEATHER_TIMEOUT         600

#define OPT_UDP                     OPT_LOCAL + 1001
#define OPT_AVG_WINDSPEED           OPT_LOCAL + 1002
#define OPT_PEEK_WINDSPEED          OPT_LOCAL + 1003

using namespace rts2sensor;

int
Davis::processOption (int _opt)
{
	switch (_opt)
	{
		case 'b':
			if (cloud_bad == NULL)
				createValue (cloud_bad, "cloud_bad", "bad cloud trigger", false);
			cloud_bad->setValueCharArr (optarg);
			break;
		case OPT_UDP:
			udpPort->setValueInteger (atoi (optarg));
			break;
		case OPT_PEEK_WINDSPEED:
			maxPeekWindSpeed->setValueCharArr (optarg);
			break;
		case OPT_AVG_WINDSPEED:
			maxWindSpeed->setValueCharArr (optarg);
			break;
		default:
			return SensorWeather::processOption (_opt);
	}
	return 0;
}

int
Davis::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	weatherConn = new DavisUdp (udpPort->getValueInteger (), BART_WEATHER_TIMEOUT,
		BART_CONN_TIMEOUT, BART_BAD_WEATHER_TIMEOUT, this);
	weatherConn->init ();
	addConnection (weatherConn);

	return 0;
}


int
Davis::setValue (Rts2Value *old_value, Rts2Value *new_value)
{
	if (old_value == maxWindSpeed || old_value == maxPeekWindSpeed)
		return 0;
	return SensorWeather::setValue (old_value, new_value);
}


int
Davis::idle ()
{
	if (getLastInfoTime () > 120)
		setWeatherTimeout (300);
	return SensorWeather::idle ();
}


Davis::Davis (int argc, char **argv)
:SensorWeather (argc, argv, 180)
{
	createValue (temperature, "DOME_TMP", "temperature in degrees C", true);
	createValue (humidity, "DOME_HUM", "(outside) humidity", true);
	createValue (rain, "RAIN", "whenever is raining", true);
	rain->setValueInteger (true);

	createValue (avgWindSpeed, "AVGWIND", "average windspeed", true);
	createValue (peekWindSpeed, "PEEKWIND", "peek windspeed", true);

	createValue (rainRate, "rain_rate", "rain rate from bucket sensor", false);
        
	createValue (maxWindSpeed, "max_windspeed", "maximal average windspeed", false);
	createValue (maxPeekWindSpeed, "max_peek_windspeed", "maximal peek windspeed", false);

	maxWindSpeed->setValueFloat (nan ("f"));
	maxPeekWindSpeed->setValueFloat (nan ("f"));

	weatherConn = NULL;

	cloud = NULL;
	cloud_bad = NULL;

	createValue (udpPort, "udp-port", "port for UDP connection from meteopoll", false);
	udpPort->setValueInteger (1500);

	addOption (OPT_PEEK_WINDSPEED, "max-peek-windspeed", 1, "maximal allowed peek windspeed (in m/sec)");
	addOption (OPT_AVG_WINDSPEED, "max-avg-windspeed", 1, "maximal allowed average windspeed (in m/sec");
	addOption (OPT_UDP, "udp-port", 1, "UDP port for incoming weather connections");
}


int
Davis::info ()
{
	// do not call infotime update..
	return 0;
}


void
Davis::setCloud (double in_cloud)
{
      if (cloud == NULL)
      {
	      createValue (cloud, "CLOUD_S", "cloud sensor value");
	      sendMetaInfo (cloud);
      }
      cloud->setValueDouble (in_cloud);
      if (cloud_bad != NULL && cloud->getValueFloat () <= cloud_bad->getValueFloat ())
      {
	      setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT);	
      }
}


int
main (int argc, char **argv)
{
	Davis device = Davis (argc, argv);
	return device.run ();
}
