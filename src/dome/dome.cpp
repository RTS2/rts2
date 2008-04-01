/* 
 * Dome driver skeleton.
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

#include "dome.h"

Rts2DevDome::Rts2DevDome (int in_argc, char **in_argv, int in_device_type):
Rts2Device (in_argc, in_argv, in_device_type, "DOME")
{
	createValue (sw_state, "dome", "dome status mask", false);
	createValue (temperature, "DOME_TMP", "temperature in degrees C");
	createValue (humidity, "DOME_HUM", "(outside) humidity");
	createValue (rain, "RAIN", "whenever is raining");
	rain->setValueInteger (1);
	createValue (windspeed, "WINDSPED", "windspeed");
	// as soon as we get update from meteo, we will solve it. We have rain now, so dome will remain closed at start

	maxWindSpeed = 50;
	maxPeekWindspeed = 50;
	weatherCanOpenDome = false;
	createValue (ignoreMeteo, "ignoreMeteo", "if meteo situation is ignored",
		false);
	ignoreMeteo->setValueBool (false);

	createValue (cloud, "CLOUD_S", "cloud sensor value");

	addOption ('W', "max_windspeed", 1, "maximal allowed windspeed (in km/h)");
	addOption ('P', "max_peek_windspeed", 1,
		"maximal allowed windspeed (in km/h");
	addOption ('O', "weather_can_open", 0,
		"specified that option if weather signal is allowed to open dome");
	addOption ('I', "ignore_meteo", 0, "whenever to ignore meteo station");

	createValue (observingPossible, "observingPossible",
		"if observation is possible", false);

	time (&nextGoodWeather);

	nextGoodWeather += DEF_WEATHER_TIMEOUT;

	createValue (nextOpen, "next_open", "time when we can next time open dome",
		false);
	nextOpen->setValueTime (getNextOpen ());
}


int
Rts2DevDome::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'W':
			maxWindSpeed = atoi (optarg);
			break;
		case 'P':
			maxPeekWindspeed = atoi (optarg);
			break;
		case 'O':
			weatherCanOpenDome = true;
			break;
		case 'I':
			ignoreMeteo->setValueBool (true);
			break;
		default:
			return Rts2Device::processOption (in_opt);
	}
	return 0;
}


void
Rts2DevDome::domeWeatherGood ()
{
	if (weatherCanOpenDome && !getIgnoreMeteo ())
	{
		setMasterOn ();
	}
}


int
Rts2DevDome::isGoodWeather ()
{
	if (getIgnoreMeteo () == true)
		return 1;
	return 0;
}


int
Rts2DevDome::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == ignoreMeteo)
		return 0;
	return Rts2Device::setValue (old_value, new_value);
}


int
Rts2DevDome::init ()
{
	return Rts2Device::init ();
}


int
Rts2DevDome::initValues ()
{
	addConstValue ("dome_model", domeModel);
	return Rts2Device::initValues ();
}


int
Rts2DevDome::checkOpening ()
{
	if ((getState () & DOME_DOME_MASK) == DOME_OPENING)
	{
		long ret;
		ret = isOpened ();
		if (ret >= 0)
		{
			setTimeout (ret);
			return 0;
		}
		if (ret == -1)
		{
			endOpen ();
			infoAll ();
			maskState (DOME_DOME_MASK, DOME_OPENED,
				"opening finished with error");
		}
		if (ret == -2)
		{
			ret = endOpen ();
			infoAll ();
			if (ret)
			{
				maskState (DOME_DOME_MASK, DOME_OPENED,
					"dome opened with error");
			}
			else
			{
				maskState (DOME_DOME_MASK, DOME_OPENED, "dome opened");
			}
		}
	}
	else if ((getState () & DOME_DOME_MASK) == DOME_CLOSING)
	{
		long ret;
		ret = isClosed ();
		if (ret >= 0)
		{
			setTimeout (ret);
			return 0;
		}
		if (ret == -1)
		{
			endClose ();
			infoAll ();
			maskState (DOME_DOME_MASK, DOME_CLOSED,
				"closing finished with error");
		}
		if (ret == -2)
		{
			ret = endClose ();
			infoAll ();
			if (ret)
			{
				maskState (DOME_DOME_MASK, DOME_CLOSED,
					"dome closed with error");
			}
			else
			{
				maskState (DOME_DOME_MASK, DOME_CLOSED, "dome closed");
			}
		}
	}
	// if we are back in idle state..beware of copula state (bit non-structural, but I
	// cannot find better solution)
	if ((getState () & DOME_COP_MASK_MOVE) == DOME_COP_NOT_MOVE)
		setTimeout (10 * USEC_SEC);
	return 0;
}


int
Rts2DevDome::idle ()
{
	checkOpening ();
	return Rts2Device::idle ();
}


int
Rts2DevDome::info ()
{
	nextOpen->setValueTime (getNextOpen ());
	return Rts2Device::info ();
}


int
Rts2DevDome::closeDomeWeather ()
{
	int ret;
	if (getIgnoreMeteo () == false)
	{
		ret = closeDome ();
		setMasterStandby ();
		return ret;
	}
	return 0;
}


int
Rts2DevDome::observing ()
{
	observingPossible->setValueInteger (1);
	if ((getState () & DOME_DOME_MASK) != DOME_OPENED)
		return openDome ();
	return 0;
}


int
Rts2DevDome::standby ()
{
	ignoreMeteo->setValueBool (false);
	if ((getState () & DOME_DOME_MASK) != DOME_CLOSED)
		return closeDome ();
	return 0;
}


int
Rts2DevDome::off ()
{
	ignoreMeteo->setValueBool (false);
	if ((getState () & DOME_DOME_MASK) != DOME_CLOSED)
		return closeDome ();
	return 0;
}


int
Rts2DevDome::setMasterStandby ()
{
	if ((getMasterState () != SERVERD_OFF && getMasterState () != SERVERD_UNKNOW)
		&& ((getMasterState () & SERVERD_STANDBY_MASK) != SERVERD_STANDBY))
	{
		return sendMaster ("standby");
	}
	return 0;
}


int
Rts2DevDome::setMasterOn ()
{
	if ((getMasterState () != SERVERD_OFF)
		&& ((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY))
	{
		return sendMaster ("on");
	}
	return 0;
}


int
Rts2DevDome::changeMasterState (int new_state)
{
	observingPossible->setValueInteger (0);
	if ((new_state & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	{
		switch (new_state & SERVERD_STATUS_MASK)
		{
			case SERVERD_EVENING:
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				standby ();
				break;
			default:
				off ();
		}
	}
	else
	{
		switch (new_state & SERVERD_STATUS_MASK)
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				observing ();
				break;
			case SERVERD_EVENING:
			case SERVERD_MORNING:
				standby ();
				break;
			default:
				off ();
		}
	}
	return Rts2Device::changeMasterState (new_state);
}


void
Rts2DevDome::setWeatherTimeout (time_t wait_time)
{
	time_t next;
	time (&next);
	next += wait_time;
	if (next > nextGoodWeather)
		nextGoodWeather = next;
}


int
Rts2DevDome::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("open"))
	{
		return (openDome () == 0 ? 0 : -2);
	}
	else if (conn->isCommand ("close"))
	{
		return (closeDome () == 0 ? 0 : -2);
	}
	else if (conn->isCommand ("ignore"))
	{
		char *ignore;
		bool newIgnore = false;
		if (conn->paramNextString (&ignore) || !conn->paramEnd ())
			return -2;
		if (!strcasecmp (ignore, "on"))
		{
			newIgnore = true;
		}
		return setIgnoreMeteo (newIgnore);
	}
	return Rts2Device::commandAuthorized (conn);
}
