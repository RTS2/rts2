/* 
 * Basic sensor class.
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

#include "sensord.h"

using namespace rts2sensord;

Sensor::Sensor (int argc, char **argv, const char *sn):rts2core::Device (argc, argv, DEVICE_TYPE_SENSOR, sn)
{
	setIdleInfoInterval (60);
}

Sensor::~Sensor (void)
{
}

int SensorWeather::idle ()
{
	// switch weather state back to good..
	if (isGoodWeather () == true && getWeatherState () == false)
	{
		// last chance - run info () again to see if the device does not change the mind about
		// its weather status..
		int ret;
		ret = info ();
		if (ret)
			return Sensor::idle ();
		if (isGoodWeather () == true)
		{
			logStream (MESSAGE_DEBUG) << "switching to GOOD_WEATHER after next_good_weather timeout expires" << sendLog;
			setWeatherState (true, "switching to GOOD_WEATHER after next_good_weather timout expires");
		}
	}
	return Sensor::idle ();
}

int SensorWeather::commandAuthorized (rts2core::Rts2Connection *conn)
{
	if (conn->isCommand ("reset_next"))
	{
		double n = getNow ();
		if (nextGoodWeather->getValueDouble () > n)
		{
			nextGoodWeather->setValueInteger (n);
			sendValueAll (nextGoodWeather);
		}
		return 0;
	}
	return Sensor::commandAuthorized (conn);
}

bool SensorWeather::isGoodWeather ()
{
	if (getNextGoodWeather () >= getNow ())
		return false;
	valueGood (nextGoodWeather);
	return true;
}

SensorWeather::SensorWeather (int argc, char **argv, int _timeout, const char *sn):Sensor (argc, argv, sn)
{
	createValue (nextGoodWeather, "next_good_weather", "date and time of next good weather");
	setWeatherTimeout (_timeout, "initial bad weather state");
}

void SensorWeather::setWeatherTimeout (time_t wait_time, const char *msg)
{
	setWeatherState (false, msg);

	time_t next;
	time (&next);
	next += wait_time;
	if (next > nextGoodWeather->getValueInteger ())
		nextGoodWeather->setValueInteger (next);
	valueError (nextGoodWeather);
}
