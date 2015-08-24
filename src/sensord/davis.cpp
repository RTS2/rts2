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
#include "davisusb.h"

#define OPT_UDP			OPT_LOCAL + 1001
#define OPT_AVG_WINDSPEED	OPT_LOCAL + 1002
#define OPT_PEEK_WINDSPEED	OPT_LOCAL + 1003
#define OPT_USB			OPT_LOCAL + 1004
#define CONN_METHOD		OPT_LOCAL + 1005


using namespace rts2sensord;

int Davis::processOption (int _opt)
{
	switch (_opt)
	{
		case 'b':
			if (cloud_bad == NULL)
				createValue (cloud_bad, "cloud_bad", "bad cloud trigger", false, RTS2_VALUE_WRITABLE);
			cloud_bad->setValueCharArr (optarg);
			break;
		case OPT_UDP:
			udpPort->setValueInteger (atoi (optarg));
			break;
		case OPT_USB:
			device_file = optarg;
			break;
		case CONN_METHOD:
			conn_method = atoi (optarg);
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

int Davis::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	if (conn_method == 0)
	{
		weatherConn = new DavisUdp (udpPort->getValueInteger (), connTimeout->getValueInteger (), BART_CONN_TIMEOUT, BART_BAD_WEATHER_TIMEOUT, this);
		weatherConn->init ();
		addConnection (weatherConn);
	}
	else if (conn_method == 1)
	{
		logStream (MESSAGE_DEBUG) << "init: " << device_file << sendLog;
		usbConn = new DavisUsb (device_file, connTimeout->getValueInteger (), BART_CONN_TIMEOUT, BART_BAD_WEATHER_TIMEOUT, this);
		usbConn->init ();
		addConnection (usbConn);
	}
	else
	{
		logStream (MESSAGE_ERROR) << "unknown connection method" << sendLog;
		exit (1);
	}

	return 0;
}

int Davis::idle ()
{
	if (getLastInfoTime () > connTimeout->getValueInteger ())
	{
		if (isGoodWeather ())
		{
			logStream (MESSAGE_ERROR) << "Weather station did not send any data for " << connTimeout->getValueInteger () << " seconds, switching to bad weather" << sendLog;
		}
		std::ostringstream os;
		os << "cannot retrieve information from Davis sensor within last " << connTimeout->getValueInteger () << " seconds";
		setWeatherTimeout (300, os.str ().c_str ());
	}
	return SensorWeather::idle ();
}

Davis::Davis (int argc, char **argv):SensorWeather (argc, argv, 180)
{
	device_file = NULL;
	
	conn_method = 0;

  	createValue (connTimeout, "conn_timeout", "connection timeout", false, RTS2_VALUE_WRITABLE);
	connTimeout->setValueInteger (360);

	createValue (temperature, "DOME_TMP", "temperature in degrees C", true);
	createValue (humidity, "DOME_HUM", "(outside) humidity", true);
	createValue (rain, "RAIN", "whenever is raining", true);
	rain->setValueInteger (true);

	createValue (avgWindSpeed, "AVGWIND", "average windspeed", true);
	createValue (avgWindSpeedStat, "AVGWINDS", "average windspeed statistic", false);
	createValue (peekWindSpeed, "PEEKWIND", "peek windspeed", true);
	createValue (windDir, "WINDDIR", "wind direction", true, RTS2_DT_DEGREES);

	createValue (rainRate, "rain_rate", "rain rate from bucket sensor", false);

	createValue (pressure, "BAR_PRESS", "barometric pressure", true);
        
	createValue (maxWindSpeed, "max_windspeed", "maximal average windspeed", false, RTS2_VALUE_WRITABLE);
	createValue (maxPeekWindSpeed, "max_peek_windspeed", "maximal peek windspeed", false, RTS2_VALUE_WRITABLE);

	maxWindSpeed->setValueFloat (NAN);
	maxPeekWindSpeed->setValueFloat (NAN);

	createValue (maxHumidity, "max_humidity", "maximal allowed humidity", false, RTS2_VALUE_WRITABLE);
	maxHumidity->setValueFloat (NAN);

	weatherConn = NULL;

	wetness = NULL;

	cloud = NULL;
	cloudTop = NULL;
	cloudBottom = NULL;
	cloud_bad = NULL;

	createValue (udpPort, "udp-port", "port for UDP connection from meteopoll", false);
	udpPort->setValueInteger (1500);

	addOption (OPT_PEEK_WINDSPEED, "max-peek-windspeed", 1, "maximal allowed peek windspeed (in m/sec)");
	addOption (OPT_AVG_WINDSPEED, "max-avg-windspeed", 1, "maximal allowed average windspeed (in m/sec");
	addOption (OPT_UDP, "udp-port", 1, "UDP port for incoming weather connections");
	addOption (OPT_USB, "usb-port", 1, "USB port where Davis Vantage Pro is connected");
	addOption (CONN_METHOD, "conn-method", 1, "udp (0 = default) / usb (1) connection");
}

int Davis::info ()
{
	// do not call infotime update..
	return 0;
}

int Davis::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == connTimeout)
	{
		weatherConn->setConnTimeout (new_value->getValueInteger ());
		return 0;
	}
	return SensorWeather::setValue (old_value, new_value);
}

void Davis::setWetness (double _wetness)
{
      if (wetness == NULL)
      {
	      createValue (wetness, "WETNESS", "wetness index");
	      updateMetaInformations (wetness);
      }
      wetness->setValueDouble (_wetness);
}

void Davis::setCloud (double _cloud, double _top, double _bottom)
{
      if (cloud == NULL)
      {
	      createValue (cloud, "CLOUD_S", "cloud sensor value", true);
	      updateMetaInformations (cloud);
	      createValue (cloudTop, "CLOUD_T", "cloud sensor top temperature", true);
	      updateMetaInformations (cloudTop);
	      createValue (cloudBottom, "CLOUD_B", "cloud sensor bottom temperature", true);
	      updateMetaInformations (cloudBottom);
      }

      cloud->setValueDouble (_cloud);
      cloudTop->setValueDouble (_top);
      cloudBottom->setValueDouble (_bottom);
      if (cloud_bad != NULL && cloud->getValueFloat () <= cloud_bad->getValueFloat ())
      {
	      setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT, "cloud sensor reports cloudy");
	      valueError (cloud);
      }
      else
      {
              valueGood (cloud);
      }
}

int main (int argc, char **argv)
{
	Davis device = Davis (argc, argv);
	return device.run ();
}
