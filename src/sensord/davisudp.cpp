/* 
 * Infrastructure for Davis UDP connection.
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
#include "utilsfunc.h"

#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace rts2sensord;

#define FRAM_CONN_TIMEOUT    60

WeatherBuf::WeatherBuf ()
{

}

WeatherBuf::~WeatherBuf ()
{
	values.clear ();
}

int WeatherBuf::parse (char *buf)
{
	char *name;
	char *value;
	char *endval;
	float fval;
	bool last = false;
	while (*buf)
	{
		// eat blanks
		while (*buf && isblank (*buf))
			buf++;
		name = buf;
		while (*buf && *buf != '=')
			buf++;
		if (!*buf)
			break;
		*buf = '\0';
		buf++;
		value = buf;
		while (*buf && *buf != ',')
			buf++;
		if (!*buf)
			last = true;
		*buf = '\0';
		fval = strtod (value, &endval);
		if (*endval)
		{
			if (!strcmp (value, "no"))
			{
				fval = 0;
			}
			else if (!strcmp (value, "yes"))
			{
				fval = 1;
			}
			else
			{
				break;
			}
		}
		values.push_back (WeatherVal (name, fval));
		if (!last)
			buf++;
	}
	if (*buf)
		return -1;
	return 0;
}

void WeatherBuf::getValue (const char *name, float &val, int &status)
{
	if (status)
		return;
	for (std::vector < WeatherVal >::iterator iter = values.begin ();
		iter != values.end (); iter++)
	{
		if ((*iter).isValue (name))
		{
			val = (*iter).value;
			return;
		}
	}
	status = -1;
}

void DavisUdp::setWeatherTimeout (time_t wait_time, const char *msg)
{
	master->setWeatherTimeout (wait_time, msg);
}

DavisUdp::DavisUdp (int _weather_port, int _weather_timeout, int _conn_timeout, int _bad_weather_timeout, Davis * _master):rts2core::ConnNoSend (_master)
{
	weather_port = _weather_port;
	weather_timeout = _weather_timeout;
	conn_timeout = _conn_timeout;
	bad_weather_timeout = _bad_weather_timeout;
	master = _master;
}

int DavisUdp::init ()
{
	struct sockaddr_in bind_addr;
	int ret;

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons (weather_port);
	bind_addr.sin_addr.s_addr = htonl (INADDR_ANY);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) << "DavisUdp::init socket: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "DavisUdp::init fcntl: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	ret = bind (sock, (struct sockaddr *) &bind_addr, sizeof (bind_addr));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "DavisUdp::init bind: " <<
			strerror (errno) << sendLog;
	}
	return ret;
}

int DavisUdp::receive (rts2core::Block *block)
{
	#define BUF_SIZE 1000
	int ret, ret_c;
	char Wbuf[BUF_SIZE];
	int data_size = 0;
	float rtIsRaining;
	float rtRainRate = 0;
	float rtWetness;
	float rtCloudTop;
	float rtCloudBottom;
	float rtOutsideHum;
	float rtOutsideTemp;
	float rtBaroCurr;
	float rtWindDir;
	double cloud = NAN;
	if (sock >= 0 && block->isForRead (sock))
	{
		struct sockaddr_in from;
		socklen_t size = sizeof (from);
		data_size =
			recvfrom (sock, Wbuf, BUF_SIZE - 1, 0, (struct sockaddr *) &from,
			&size);
		if (data_size < 0)
		{
			logStream (MESSAGE_DEBUG) << "error in receiving weather data: " <<
				sendLog;
			return 1;
		}
		Wbuf[data_size] = 0;
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "readed: " << data_size << " " << Wbuf <<
			" from  " << inet_ntoa (from.sin_addr) << " " << ntohs (from.
			sin_port) <<
			sendLog;
		#endif
		// parse weather info
		//rtExtraTemp2=3.3, rtWindSpeed=0.0, rtInsideHum=22.0, rtWindDir=207.0, rtExtraTemp1=3.9, rtRainRate=0.0, rtOutsideHum=52.0, rtWindAvgSpeed=0.4, rtInsideTemp=23.4, rtExtraHum1=51.0, rtBaroCurr=1000.0, rtExtraHum2=51.0, rtOutsideTemp=0.5/
		WeatherBuf *weather = new WeatherBuf ();
		ret = weather->parse (Wbuf);
		ret_c = ret;

		weather->getValue ("rtIsRaining", rtIsRaining, ret);
		if (ret)
		{
			ret = 0;
			weather->getValue ("rtRainRate", rtRainRate, ret);
			if (!ret)
				rtIsRaining = (rtRainRate > 0);
		}
		weather->getValue ("rtRainRate", rtRainRate, ret);
		weather->getValue ("rtWindSpeed", peekwindspeed, ret);
		weather->getValue ("rtWindAvgSpeed", avgWindSpeed, ret);
		weather->getValue ("rtWindDir", rtWindDir, ret);
		weather->getValue ("rtOutsideHum", rtOutsideHum, ret);
		weather->getValue ("rtOutsideTemp", rtOutsideTemp, ret);
		weather->getValue ("rtBaroCurr", rtBaroCurr, ret);
		if (ret)
		{
			rain = 1;
			setWeatherTimeout (conn_timeout, "cannot parse rain, wind, humidity, temperature or barometric pressure data - check the logs");
			return data_size;
		}
		// get information about cloud cover
		ret_c = 0;
		weather->getValue ("rtCloudTop", rtCloudTop, ret_c);
		weather->getValue ("rtCloudBottom", rtCloudBottom, ret_c);
		if (ret_c == 0)
			cloud = rtCloudBottom - rtCloudTop;
		ret_c = 0;

		weather->getValue ("rtWetness", rtWetness, ret_c);
		if (ret_c == 0)
			master->setWetness (rtWetness);
		else
			ret_c = 0;

		if (rtRainRate != 0)
		{
			rain = 1;
		}
		else if (rtIsRaining > 0)
		{
			// try to get more information about nature of rain and cloud cover
			weather->getValue ("rtWetness", rtWetness, ret_c);
			if (ret_c == 0)
			{
				float dew;
				float vapor;
				vapor =
					(rtOutsideHum / 100) * 0.611 * exp (17.27 * rtOutsideTemp /
					(rtOutsideTemp + 237.3));
				dew =
					ceil (10 *
					((116.9 + 237.3 * log (vapor)) / (16.78 -
					log (vapor)))) / 10;
				#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) <<
					"DavisUdp::parse rtCloudBottom " << rtCloudBottom <<
					" rtCloudTop " << rtCloudTop << " rtRainRate " << rtRainRate
					<< " rtWetness " << rtWetness << " vapor " << vapor << " dew "
					<< dew << sendLog;
				#endif
				if ((rtCloudBottom - rtCloudTop) > 2.5 && rtRainRate == 0
					&& rtWetness < 15.0 && fabs (rtOutsideTemp - dew) < 3.0)
					rain = 0;
				else if ((rtCloudBottom - rtCloudTop) > 3.0 && rtRainRate == 0
					&& rtWetness < 7.0 && fabs (rtOutsideTemp - dew) < 6.0)
					rain = 0;
				else
					rain = 1;
			}
			else
			{
				rain = 1;
			}
		}
		else
		{
			rain = 0;
		}
		master->setTemperature (rtOutsideTemp);
		master->setRainRate (rtRainRate);
		master->setRain (rain);
		master->setHumidity (rtOutsideHum);
		master->setAvgWindSpeed (avgWindSpeed);
		master->setPeekWindSpeed (peekwindspeed);
		master->setWindDir (rtWindDir);
		master->setBaroCurr (rtBaroCurr);
		master->updateInfoTime ();
		if (!std::isnan (cloud))
		{
			master->setCloud (cloud, rtCloudTop, rtCloudBottom);
		}
		delete weather;

		time (&lastWeatherStatus);
		if (rain != 0)
		{
			time (&lastBadWeather);
			setWeatherTimeout (bad_weather_timeout, "raining");
		}
		// ack message
		sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from, sizeof (from));
		master->infoAll ();
	}
	return data_size;
	#undef BUF_SIZE
}
