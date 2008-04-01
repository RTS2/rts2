/* 
 * Dome driver for Bootes1 station.
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

#include "ford.h"

#include "rts2connbufweather.h"

#define ROOF_TIMEOUT  120		 // in seconds

#define WATCHER_METEO_TIMEOUT 80

#define WATCHER_BAD_WEATHER_TIMEOUT 3600
#define WATCHER_BAD_WINDSPEED_TIMEOUT 360
#define WATCHER_CONN_TIMEOUT    360

typedef enum
{
	PORT_1,
	PORT_2,
	PORT_3,
	PORT_4,
	DOMESWITCH,
	PORT_6,
	PORT_7,
	PORT_8,
	// A
	OPEN_END_1,
	CLOSE_END_1,
	CLOSE_END_2,
	OPEN_END_2,
	// 0xy0,
	PORT_13,
	RAIN_SENSOR
} outputs;

class Rts2DevDomeBootes1:public Rts2DomeFord
{
	private:
		time_t timeOpenClose;
		bool domeFailed;

		Rts2ConnBufWeather *weatherConn;

		bool isMoving ();

		int lastWeatherCheckState;

		Rts2ValueTime *ignoreRainSensorTime;

	protected:
		virtual int isGoodWeather ();

	public:
		Rts2DevDomeBootes1 (int argc, char **argv);
		virtual ~ Rts2DevDomeBootes1 (void);
		virtual int init ();
		virtual int idle ();

		virtual int ready ();
		virtual int baseInfo ();
		virtual int info ();

		virtual int openDome ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int closeDome ();
		virtual long isClosed ();
		virtual int endClose ();
};

Rts2DevDomeBootes1::Rts2DevDomeBootes1 (int in_argc, char **in_argv):
Rts2DomeFord (in_argc, in_argv)
{
	createValue (ignoreRainSensorTime, "ignore_rain_time", "time when rain sensor will be ignored", false);
	ignoreRainSensorTime->setValueDouble (nan ("f"));

	domeModel = "BOOTES1";

	lastWeatherCheckState = -1;

	weatherConn = NULL;

	timeOpenClose = 0;
	domeFailed = false;
}


Rts2DevDomeBootes1::~Rts2DevDomeBootes1 (void)
{
}


int
Rts2DevDomeBootes1::isGoodWeather ()
{
	int ret = zjisti_stav_portu ();
	if (ret)
	  	return getIgnoreMeteo () == true ? 1 : 0;

	// rain sensor, ignore if dome was recently opened
	if (getPortState (RAIN_SENSOR))
	{
		int weatherCheckState = (getPortState (OPEN_END_2) << 3)
			| (getPortState (CLOSE_END_2) << 2)
			| (getPortState (CLOSE_END_1) << 1)
			| (getPortState (OPEN_END_1));
		if (lastWeatherCheckState == -1)
			lastWeatherCheckState = weatherCheckState;

		// all switches must be off and previous reading must show at least one was on, know prerequsity for failed rain sensor
		if (weatherCheckState != lastWeatherCheckState || (getState () & DOME_OPENING) || (getState () & DOME_CLOSING))
		{
			logStream (MESSAGE_WARNING) << "ignoring faulty rain sensor" << sendLog;
			ignoreRainSensorTime->setValueDouble (getNow () + 120);
		}
		else
		{
			// if we are not in ignoreRainSensorTime period..
			if (isnan (ignoreRainSensorTime->getValueDouble ()) || getNow () > ignoreRainSensorTime->getValueDouble ())
			{
				lastWeatherCheckState = weatherCheckState;
				setWeatherTimeout (WATCHER_BAD_WEATHER_TIMEOUT);
	  			return getIgnoreMeteo () == true ? 1 : 0;
			}
		}
		lastWeatherCheckState = weatherCheckState;
	}
	else
	{
		lastWeatherCheckState = (getPortState (OPEN_END_2) << 3)
			| (getPortState (CLOSE_END_2) << 2)
			| (getPortState (CLOSE_END_1) << 1)
			| (getPortState (OPEN_END_1));
	}

	if (getIgnoreMeteo () == true)
		return 1;

	if (weatherConn)
		return weatherConn->isGoodWeather ();
	return 0;
}


int
Rts2DevDomeBootes1::init ()
{
	int ret;
	ret = Rts2DomeFord::init ();
	if (ret)
		return ret;

	weatherConn =
		new Rts2ConnBufWeather (5005, WATCHER_METEO_TIMEOUT,
		WATCHER_CONN_TIMEOUT,
		WATCHER_BAD_WEATHER_TIMEOUT,
		WATCHER_BAD_WINDSPEED_TIMEOUT, this);
	weatherConn->init ();
	addConnection (weatherConn);

	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	// set dome state..
	if (getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2))
	{
		setState (DOME_CLOSED, "Init state is closed");
	} else if (getPortState (OPEN_END_1) && getPortState (OPEN_END_2))
	{
		setState (DOME_OPENED, "Init state is opened");
	} else
	{
		// not opened, not closed..
		return -1;
	}
	return 0;
}


int
Rts2DevDomeBootes1::idle ()
{
	// check for weather..
	if (isGoodWeather ())
	{
		if (((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
			&& ((getState () & DOME_DOME_MASK) == DOME_CLOSED))
		{
			// after centrald reply, that he switched the state, dome will
			// open
			domeWeatherGood ();
		}
	}
	else
	{
		int ret;
		// close dome - don't trust centrald to be running and closing
		// it for us
		ret = closeDomeWeather ();
		if (ret == -1)
		{
			setTimeout (10 * USEC_SEC);
		}
	}
	return Rts2DomeFord::idle ();
}


int
Rts2DevDomeBootes1::ready ()
{
	return 0;
}


int
Rts2DevDomeBootes1::baseInfo ()
{
	return 0;
}


int
Rts2DevDomeBootes1::info ()
{
	// switches are both off either when we move enclosure or when dome failed
	if (domeFailed || timeOpenClose > 0)
		sw_state->setValueInteger (0);

	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	sw_state->setValueInteger ((getPortState (CLOSE_END_1) << 3)
		| (getPortState (CLOSE_END_2) << 2)
		| (getPortState (OPEN_END_1) << 1)
		| getPortState (OPEN_END_2));

	if (weatherConn)
	{
		setRain (weatherConn->getRain ());
		setWindSpeed (weatherConn->getWindspeed ());
	}

	return Rts2DomeFord::info ();
}


bool
Rts2DevDomeBootes1::isMoving ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (getPortState (CLOSE_END_1) == getPortState (CLOSE_END_2)
		&& getPortState (OPEN_END_1) == getPortState (OPEN_END_2)
		&& (getPortState (CLOSE_END_1) != getPortState (OPEN_END_1)))
		return false;
	return true;
}


int
Rts2DevDomeBootes1::openDome ()
{
	if (!isGoodWeather ())
		return -1;
	if (getState () & DOME_OPENING)
	{
		if (isMoving ())
			return 0;
		return -1;
	}
	if ((getState () & DOME_OPENED)
		&& (getPortState (OPEN_END_1) || getPortState (OPEN_END_2)))
	{
		return 0;
	}
	if (getState () & DOME_CLOSING)
		return -1;

	ZAP(DOMESWITCH);
	sleep (1);
	VYP(DOMESWITCH);
	sleep (1);

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return Rts2DomeFord::openDome ();
}


long
Rts2DevDomeBootes1::isOpened ()
{
	time_t now;
	time (&now);
	// timeout
	if (now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBootes1::isOpened timeout" <<
			sendLog;
		domeFailed = true;
		sw_state->setValueInteger (0);
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome opened with errror");
		openDome ();
		return -2;
	}
	if (isMoving ())
		return USEC_SEC;
	if (getPortState (OPEN_END_1) || getPortState (OPEN_END_2))
		return -2;
	if (getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2))
		return USEC_SEC;
	logStream (MESSAGE_ERROR) << "isOpened reached unknow state" << sendLog;
	return USEC_SEC;
}


int
Rts2DevDomeBootes1::endOpen ()
{
	if (!domeFailed)
	{
		sw_state->setValueInteger (1);
	}
	return Rts2DomeFord::endOpen ();
}


int
Rts2DevDomeBootes1::closeDome ()
{
	// we cannot close dome when we are still moving
	if (getState () & DOME_CLOSING)
	{
		if (isMoving ())
			return 0;
		return -1;
	}
	if ((getState () & DOME_CLOSED)
		&& (getPortState (CLOSE_END_1) || getPortState (CLOSE_END_2)))
	{
		return 0;
	}
	if (getState () & DOME_OPENING)
		return -1;

	ZAP(DOMESWITCH);
	sleep (1);
	VYP(DOMESWITCH);
	sleep (1);

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return Rts2DomeFord::closeDome ();
}


long
Rts2DevDomeBootes1::isClosed ()
{
	time_t now;
	time (&now);
	if (now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBootes1::isClosed dome timeout"
			<< sendLog;
		domeFailed = true;
		sw_state->setValueInteger (0);
		// cycle again..
		maskState (DOME_DOME_MASK, DOME_OPENED, "failed closing");
		closeDome ();
		return USEC_SEC;
	}
	if (isMoving ())
		return USEC_SEC;
	// at least one switch must be closed
	if (getPortState (CLOSE_END_1) || getPortState (CLOSE_END_2))
		return -2;
	if (getPortState (OPEN_END_1) && getPortState (OPEN_END_2))
		return USEC_SEC;
	logStream (MESSAGE_ERROR) << "isClosed reached unknow state" << sendLog;
	return USEC_SEC;
}


int
Rts2DevDomeBootes1::endClose ()
{
	return Rts2DomeFord::endClose ();
}


int
main (int argc, char **argv)
{
	Rts2DevDomeBootes1 device = Rts2DevDomeBootes1 (argc, argv);
	return device.run ();
}
