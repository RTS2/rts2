/* 
 * Dome driver for Bootes1 station.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#define ROOF_TIMEOUT  60		 // in seconds

#define WATCHER_METEO_TIMEOUT 80

#define WATCHER_BAD_WEATHER_TIMEOUT 3600
#define WATCHER_BAD_WINDSPEED_TIMEOUT 360
#define WATCHER_CONN_TIMEOUT    360

#define BASE 0xde00

typedef enum
{
	DOMESWITCH,
	PORT_2,
	PORT_3,
	PORT_4,
	PORT_5,
	PORT_6,
	PORT_7,
	PORT_8,
	OPEN_END_1,
	CLOSE_END_1,
	CLOSE_END_2,
	OPEN_END_2,
} outputs;

class Rts2DevDomeBootes1:public Rts2DomeFord
{
	private:
		time_t timeOpenClose;
		bool domeFailed;

		Rts2ConnBufWeather *weatherConn;

		bool isMoving ();

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
	domeModel = "DUBLIN_DOME";

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
		// close dome - don't thrust centrald to be running and closing
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
	if (isMoving () || (getState () & DOME_OPENED))
		return 0;

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
		// stop motor
		closeDome ();
		return -2;
	}
	return (isMoving () ? USEC_SEC : -2);
}


int
Rts2DevDomeBootes1::endOpen ()
{
	timeOpenClose = 0;
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
	if (isMoving () || (getState () & DOME_CLOSED))
		return 0;

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
		openDome ();
		return -2;
	}
	return (isMoving ()? USEC_SEC : -2);
}


int
Rts2DevDomeBootes1::endClose ()
{
	timeOpenClose = 0;
	return Rts2DomeFord::endClose ();
}


int
main (int argc, char **argv)
{
	Rts2DevDomeBootes1 device = Rts2DevDomeBootes1 (argc, argv);
	return device.run ();
}
