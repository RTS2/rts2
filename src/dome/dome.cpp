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

using namespace rts2dome;

#define OPT_WEATHER_OPENS     OPT_LOCAL + 200


int
Dome::domeOpenStart ()
{
	if (isOpened () == -2)
		return 0;

	if (isGoodWeather () == false)
		return -1;

	if (startOpen ())
	{
	  	logStream (MESSAGE_ERROR) << "opening of the dome failed" << sendLog;
		return -1;
	}
	maskState (DOME_DOME_MASK, DOME_OPENING, "opening dome");
	logStream (MESSAGE_INFO) << "starting to open the dome" << sendLog;
	return 0;
}


int
Dome::domeOpenEnd ()
{
	if (endOpen ())
	{
		logStream (MESSAGE_ERROR) << "end open operation failed" << sendLog;
	}
	maskState (DOME_DOME_MASK, DOME_OPENED, "dome opened");
	return 0;
};


int
Dome::domeCloseStart ()
{
	if (isClosed () == -2)
		return 0;

	if (startClose ())
	{
		logStream (MESSAGE_ERROR) << "cannot start closing of the dome" << sendLog;
		return -1;
	}
	maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome");
	logStream (MESSAGE_INFO) << "closing dome" << sendLog;
	return 0;
};


int
Dome::domeCloseEnd ()
{
 	if (endClose ())
	{
		logStream (MESSAGE_ERROR) << "cannot end closing of the dome" << sendLog;
	}
	maskState (DOME_DOME_MASK, DOME_CLOSED, "dome closed");
	return 0;
};


Dome::Dome (int in_argc, char **in_argv, int in_device_type):
Rts2Device (in_argc, in_argv, in_device_type, "DOME")
{
	createValue (weatherOpensDome, "weather_open", "if weather information is enought to open dome", false);
	weatherOpensDome->setValueBool (false);

	createValue (ignoreTimeout, "ignore_time", "date and time for which meteo information will be ignored", false);
	ignoreTimeout->setValueDouble (0);

	createValue (nextGoodWeather, "next_open", "date and time when dome can be opened again");
	nextGoodWeather->setValueDouble (getNow () + DEF_WEATHER_TIMEOUT);

	addOption (OPT_WEATHER_OPENS, "weather_can_open", 0, "specified that option if weather signal is allowed to open dome");
	addOption ('I', NULL, 0, "whenever to ignore meteo station. Ignore time will be set to 610 seconds.");
}


Dome::~Dome ()
{
}


int
Dome::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_WEATHER_OPENS:
			weatherOpensDome->setValueBool (true);
			break;
		case 'I':
			ignoreTimeout->setValueDouble (getNow () + DEF_WEATHER_TIMEOUT + 10);
			break;
		default:
			return Rts2Device::processOption (in_opt);
	}
	return 0;
}


void
Dome::domeWeatherGood ()
{
}


bool
Dome::isGoodWeather ()
{
	if (getIgnoreMeteo () == true)
		return true;
	if (Rts2Device::isGoodWeather () == false)
	  	return false;
	if (getNextOpen () > getNow ())
		return false;
	return true;
}


int
Dome::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == ignoreTimeout || old_value == weatherOpensDome)
		return 0;
	return Rts2Device::setValue (old_value, new_value);
}


int
Dome::checkOpening ()
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
Dome::idle ()
{
	checkOpening ();
	if (isGoodWeather ())
	{
		if (weatherOpensDome->getValueBool () == true
			&& (getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
		{
			setMasterOn ();
		}
	}
	else 
	{
		int ret;
		ret = closeDomeWeather ();
		if (ret == -1)
		{
			setTimeout (10 * USEC_SEC);
		}
	}
	// update our own weather state..
	if (allCentraldRunning () && getNextOpen () < getNow ())
	{
		setWeatherState (true);
	}
	else
	{
		setWeatherState (false);
	}

	return Rts2Device::idle ();
}


int
Dome::closeDomeWeather ()
{
	int ret;
	if (getIgnoreMeteo () == false)
	{
		ret = domeCloseStart ();
		setMasterStandby ();
		return ret;
	}
	return 0;
}


int
Dome::observing ()
{
	return domeOpenStart ();
}


int
Dome::standby ()
{
	ignoreTimeout->setValueDouble (getNow () - 1);
	return domeCloseStart ();
}


int
Dome::off ()
{
	ignoreTimeout->setValueDouble (getNow () - 1);
	return domeCloseStart ();
}


int
Dome::setMasterStandby ()
{
	if (getMasterState () != SERVERD_SOFT_OFF && getMasterState () != SERVERD_HARD_OFF
	  	&& getMasterState () != SERVERD_UNKNOW
		&& (getMasterState () & SERVERD_STANDBY_MASK) != SERVERD_STANDBY)
	{
		return sendMasters ("standby");
	}
	return 0;
}


int
Dome::setMasterOn ()
{
	if (getMasterState () != SERVERD_SOFT_OFF && getMasterState () != SERVERD_HARD_OFF
		&& (getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	{
		return sendMasters ("on");
	}
	return 0;
}


int
Dome::changeMasterState (int new_state)
{
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
Dome::setIgnoreTimeout (time_t _ignore_time)
{
	time_t now;
	time (&now);
	now += _ignore_time;
}


bool
Dome::getIgnoreMeteo ()
{
	return ignoreTimeout->getValueDouble () > getNow ();
}


void
Dome::setWeatherTimeout (time_t wait_time)
{
	time_t next;
	time (&next);
	next += wait_time;
	if (next > nextGoodWeather->getValueInteger ())
		nextGoodWeather->setValueInteger (next);
}


int
Dome::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("open"))
	{
		return (domeOpenStart () == 0 ? 0 : -2);
	}
	else if (conn->isCommand ("close"))
	{
		return (domeCloseStart () == 0 ? 0 : -2);
	}
	return Rts2Device::commandAuthorized (conn);
}
