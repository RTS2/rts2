/* 
 * Fram weather sensor.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "fram-weather.h"

#include <strings.h>

using namespace rts2sensord;

int FramWeather::processOption (int _opt)
{
	switch (_opt)
	{
		case 'W':
			maxWindSpeed->setValueFloat (atof (optarg));
			break;
	}
	return SensorWeather::processOption (_opt);
}

int FramWeather::init ()
{
	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;

	weatherConn = new ConnFramWeather (5002, 40, this);
	weatherConn->init ();
	addConnection (weatherConn);

	return 0;
}

FramWeather::FramWeather (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (windSpeed, "windspeed", "current measured windspeed", false);
	createValue (rain, "rain", "true it it is raining", false);
	createValue (watch, "watch", "connection watch status", false);
	watch->addSelVal ("WATCH");
	watch->addSelVal ("UNKNOW");

	createValue (maxWindSpeed, "max_windspeed", "maximal allowed windspeed", false, RTS2_VALUE_WRITABLE);
	maxWindSpeed->setValueFloat (50);

	createValue (connUpdateSep, "update_time", "maximal time in seconds when connection is still considered as live", false, RTS2_VALUE_WRITABLE);
	connUpdateSep->setValueInteger (40);

	createValue (timeoutConn, "timeout_conn", "connection timeout in seconds", false, RTS2_VALUE_WRITABLE);
	timeoutConn->setValueInteger (600);

	createValue (timeoutRain, "timeout_rain", "rain timeout", false, RTS2_VALUE_WRITABLE);
	timeoutRain->setValueInteger (7200);

	createValue (timeoutWindspeed, "timeout_windspeed", "windspeed timeout", false, RTS2_VALUE_WRITABLE);
	timeoutWindspeed->setValueInteger (600);

	addOption ('W', "max_windspeed", 1, "maximal allowed windspeed (in km/h)");
}

int FramWeather::info ()
{
	return (getLastInfoTime () < connUpdateSep->getValueInteger ()) ? 0 : -1;
}

bool FramWeather::isGoodWeather ()
{
	if (getLastInfoTime () > connUpdateSep->getValueInteger ())
	{
		setWeatherTimeout (80, "did not received weather update");
		return false;
	}
	return SensorWeather::isGoodWeather ();	
}

void FramWeather::setWeather (float _windSpeed, bool _rain, const char *_status, struct ln_date *_date)
{
	struct tm _tm;
	windSpeed->setValueFloat (_windSpeed);
	if (_windSpeed >= maxWindSpeed->getValueFloat ())
	{
		setWeatherTimeout (timeoutWindspeed->getValueInteger (), "wind speed above max wind speed");
		valueError (windSpeed);
	}
	else
	{
		valueGood (windSpeed);
	}
	
	rain->setValueBool (_rain);
	if (_rain == true)
	{
		setWeatherTimeout (timeoutRain->getValueInteger (), "raining");
		valueError (rain);
	}
	else
	{
		valueGood (rain);
	}

	if (strcmp (_status, "watch") == 0)
	{
		watch->setValueInteger (0);
		valueGood (watch);
	}
	else
	{
	  	watch->setValueInteger (1);
		setWeatherTimeout (timeoutConn->getValueInteger (), "meteo station not in 'watch' mode");
		valueError (watch);
	}
	// change from date to tm
	memset (&_tm, 0, sizeof(struct tm));
	_tm.tm_year = _date->years - 1900;
	_tm.tm_mon = _date->months - 1;
	_tm.tm_mday = _date->days;
	_tm.tm_hour = _date->hours;
	_tm.tm_min = _date->minutes;
	_tm.tm_sec = (int) _date->seconds;
	setInfoTime (&_tm);
	if (getLastInfoTime () > connUpdateSep->getValueInteger ())
	{
		setWeatherTimeout (timeoutConn->getValueInteger (), "last weather updated received too late");
		valueError (connUpdateSep);
	}
	else
	{
		valueGood (connUpdateSep);
	}
	infoAll ();
}

int main (int argc, char **argv)
{
	FramWeather device = FramWeather (argc, argv);
	return device.run ();
}
