/* 
 * Dome driver for Bootes 1A observatory.
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

#define ROOF_TIMEOUT  120		 // in seconds

typedef enum
{
	// B
	PORT_1,
	PORT_2,
	PORT_3,
	ROOF_SWITCH,
	PORT_5,
	PORT_6,
	PORT_7,
	PORT_8,
	// A
	PORT_10,  // floats around 0/1 independently on input
	OPEN_END_2,
	OPEN_END_1,
	PORT_13,  // seems to be always zero
	CLOSE_END_1,
	CLOSE_END_2
	//
} outputs;

using namespace rts2dome;

namespace rts2dome
{

class Bootes1A:public Ford
{
	private:
		Rts2ValueInteger *sw_state;

		time_t timeOpenClose;
		time_t timeOpen;
		time_t timeClose;
		bool domeFailed;

		bool isMoving ();

		int logPorts ();

		int swDome (); // send pulse to the dome in a safe way to avoid problems

	protected:
		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

	public:
		Bootes1A (int argc, char **argv);
		virtual ~ Bootes1A (void);

		virtual int init ();

		virtual int info ();
};

}

Bootes1A::Bootes1A (int argc, char **argv):Ford (argc, argv)
{
	createValue (sw_state, "sw_state", "end switches state", RTS2_DT_HEX);
	timeOpenClose = 0;
	domeFailed = false;
}


Bootes1A::~Bootes1A (void)
{
}


int
Bootes1A::logPorts ()
{
	logStream (MESSAGE_DEBUG) << "dome1a init 3" 
		<< " C1:" << getPortState (CLOSE_END_1) 
		<< " C2:" << getPortState (CLOSE_END_2) 
		<< " O1:" << getPortState (OPEN_END_1) 
		<< " O2:" << getPortState (OPEN_END_2) 
		<< sendLog;
	return 0;
}

int
Bootes1A::init ()
{
	int ret;
	logStream (MESSAGE_DEBUG) << "dome1a init 1" << sendLog;
	ret = Ford::init ();
	if (ret)
		return ret;
	logStream (MESSAGE_DEBUG) << "dome1a init 2" << sendLog;

/*
	weatherConn =
		new Rts2ConnBufWeather (5005, WATCHER_METEO_TIMEOUT,
		WATCHER_CONN_TIMEOUT,
		WATCHER_BAD_WEATHER_TIMEOUT,
		WATCHER_BAD_WINDSPEED_TIMEOUT, this);
	weatherConn->init ();
	addConnection (weatherConn);
*/
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	// set dome state..
	if (getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2))
	{
		logStream (MESSAGE_DEBUG) << "dome1a: found dome closed on startup" << sendLog;
		setState (DOME_CLOSED, "Init state is closed");
		return 0;
	} 
	
	if (getPortState (OPEN_END_1) && getPortState (OPEN_END_2))
	{
		logStream (MESSAGE_DEBUG) << "dome1a: found dome open on startup" << sendLog;
		setState (DOME_OPENED, "Init state is opened");
		return 0;
	} 

	// not opened, not closed..
	logPorts();
	logStream (MESSAGE_DEBUG) << "dome1a: found dome unclean - refusing to start up" << sendLog;
	return -1;
}


int
Bootes1A::info ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	sw_state->setValueInteger ((getPortState (CLOSE_END_1) << 3)
		| (getPortState (CLOSE_END_2) << 2)
		| (getPortState (OPEN_END_1) << 1)
		| getPortState (OPEN_END_2));

	return Ford::info ();
}


bool
Bootes1A::isMoving ()
{
	int ret;
	ret = zjisti_stav_portu ();

	// the roof may have nine ( [open, closed, running = 3], [two halves=2]
	// 3^2=9) distinct port states. Of these, only full open and full close
	// mean no activity of the housing.

	return  
		!(
			(
			( getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2) ) 
			||
			( getPortState (OPEN_END_1)  && getPortState (OPEN_END_2) )
			)	 
		);
}

// Florentinos new FEATURE! - the dome must not be touched if not in
// the completely open/closed state - Therefore we MUST check the
// endswitches and WAIT until we may touch it. This presents a
// potential security weakness - if the outer end switches fail, we
// have a problem of not closing, knowing that we would break the
// system!
//	
// This should be and stay the only code to touch the output to the dome
int
Bootes1A::swDome ()
{
	if (! isMoving() )
	{
		ZAP (ROOF_SWITCH);
		usleep (750000); 
		VYP (ROOF_SWITCH);
		return 0;
	}
	return -1;
}

int
Bootes1A::startOpen ()
{
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

	if (swDome())
		return -1;

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return 0;
}


long
Bootes1A::isOpened ()
{
	time_t now;
	time (&now);
	// timeout
	if (timeOpenClose > 0 && now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBootes1::isOpened timeout" <<
			sendLog;
		domeFailed = true;
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome opened with errror");
		time (&timeOpenClose);
		timeOpenClose += ROOF_TIMEOUT;

		domeOpenStart ();
		return USEC_SEC;
	}

	// Vzhledem k logice isMoving se strecha bud hejbe, nebo je otevrena, bo je zavrena, 
	// takze jsou jen 3 moznosti:
	// Je jeste zavrena, nebo se hejbe, pak pockame:
	if (isMoving () || (getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2)) )
		return USEC_SEC;
	// je otevrena, pak rekneme, ze je otevrena.
	if (getPortState (OPEN_END_1) && getPortState (OPEN_END_2))
		return -2;

	logPorts();

	return USEC_SEC;
}


int
Bootes1A::endOpen ()
{
	timeOpenClose = 0;
	return 0;
}


int
Bootes1A::startClose ()
{

	// we cannot close dome when we are still moving
	if (getState () & DOME_CLOSING)
	{
		if (isMoving ())
			return 0;
		return -1;
	}
	if (getState () & DOME_OPENING)
		return -1;

	if (swDome())
		return -1;

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return 0;
}


long
Bootes1A::isClosed ()
{
	time_t now;
	time (&now);
	if (timeOpenClose > 0 && now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevDomeBootes1::isClosed dome timeout" << sendLog;
		domeFailed = true;
		// cycle again..
		maskState (DOME_DOME_MASK, DOME_OPENED, "failed closing");
		time (&timeOpenClose);
		timeOpenClose += ROOF_TIMEOUT;
		domeCloseStart ();
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
Bootes1A::endClose ()
{
	timeOpenClose = 0;
	return 0;
}


int
main (int argc, char **argv)
{
	Bootes1A device = Bootes1A (argc, argv);
	return device.run ();
}
