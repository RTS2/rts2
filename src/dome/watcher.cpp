// WARNING - this code is not working on Watcher, as its dome control was repleced
// with ZELIO PLC in ~2011. Probably will not work, leave as reference for possible
// similar implementations

/* 
 * Driver for Watcher dome board.
 * Copyright (C) 2005-2008 John French
 * Copyright (C) 2006-2008 Petr Kubanek <petr@kubanek.net>
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
#include <sys/io.h>

#include "dome.h"

#define ROOF_TIMEOUT  360		 // in seconds

#define OPEN  	  2
#define CLOSE     4

#define OPENING   2
#define CLOSING   0

#define WATCHER_DOME_OPEN                 1
#define WATCHER_DOME_CLOSED               0
#define WATCHER_DOME_UNKNOWN             -1

#define BASE                         0xde00

typedef enum
{ TYPE_OPENED, TYPE_CLOSED, TYPE_STUCK }
smsType_t;

using namespace rts2dome;

namespace rts2dome
{

class Watcher:public Dome
{
	private:
		int dome_state;
		time_t timeOpenClose;
		bool domeFailed;
		char *smsExec;
		double cloud_bad;

		bool isMoving ();

		void openDomeReal ();
		void closeDomeReal ();

		const char *isOnString (int mask);

		rts2core::ValueInteger *sw_state;

	protected:
		virtual int processOption (int in_opt);

	public:
		Watcher (int argc, char **argv);
		virtual ~ Watcher (void);
		virtual int init ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
};

}

Watcher::Watcher (int argc, char **argv):Dome (argc, argv)
{
	createValue (sw_state, "sw_state", "dome state", false);
	smsExec = NULL;
	
	timeOpenClose = 0;
	domeFailed = false;
}

Watcher::~Watcher (void)
{
	outb (0, BASE);
	// SWITCH OFF INTERFACE
	outb (0, BASE + 1);
}

int Watcher::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 's':
			smsExec = optarg;
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}

int Watcher::init ()
{
	int ret, i;
	ret = Dome::init ();
	if (ret)
		return ret;

	dome_state = WATCHER_DOME_UNKNOWN;

	ioperm (BASE, 4, 1);

	// SET CONTROL WORD
	outb (137, BASE + 3);

	// INITIALIZE ALL PORTS TO 0

	for (i = 0; i <= 2; i++)
	{
		outb (0, BASE + i);
	}

	// SWITCH ON INTERFACE
	outb (1, BASE + 1);

	if (!isMoving ())
	{
		// close roof - security measurement
		startClose ();

		maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome after init");
	}

	return 0;
}

int Watcher::info ()
{
	// switches are both off either when we move enclosure or when dome failed
	if (domeFailed || timeOpenClose > 0)
		sw_state->setValueInteger (0);

	return Dome::info ();
}

bool Watcher::isMoving ()
{
	int result;
	int moving = 0;
	int count;
	for (count = 0; count < 100; count++)
	{
		result = (inb (BASE + 2));
		// we think it's moving
		if (result & 2)
			moving++;
		usleep (USEC_SEC / 100);
	}
	// motor is moving at least once
	if (moving > 0)
		return true;
	// dome is regarded as not failed after move of motor stop nominal way
	domeFailed = false;
	return false;
}

void Watcher::openDomeReal ()
{
	outb (OPEN, BASE);

	sleep (1);
	outb (0, BASE);

	// wait for motor to decide to move
	sleep (5);
}

int Watcher::startOpen ()
{
	if (isMoving () || dome_state == WATCHER_DOME_OPEN)
		return 0;

	openDomeReal ();

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return 0;
}


long
Watcher::isOpened ()
{
	time_t now;
	time (&now);
	// timeout
	if (timeOpenClose > 0 && now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Watcher::isOpened timeout" <<
			sendLog;
		domeFailed = true;
		sw_state->setValueInteger (0);
		// stop motor
		closeDomeReal ();
		return -2;
	}
	if (isMoving ())
	{
		return USEC_SEC;
	}
	return (getState () & DOME_DOME_MASK) == DOME_CLOSED ? 0 : -2;
}

int Watcher::endOpen ()
{
	timeOpenClose = 0;
	dome_state = WATCHER_DOME_OPEN;
	if (!domeFailed)
	{
		sw_state->setValueInteger (1);
	}
	return 0;
}

void Watcher::closeDomeReal ()
{
	outb (CLOSE, BASE);

	sleep (1);
	outb (0, BASE);

	// give controller time to react
	sleep (5);
}

int Watcher::startClose ()
{
	// we cannot close dome when we are still moving
	if (isMoving () || dome_state == WATCHER_DOME_CLOSED)
		return 0;

	closeDomeReal ();

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return 0;
}

long Watcher::isClosed ()
{
	time_t now;
	time (&now);
	if (timeOpenClose > 0 && now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Watcher::isClosed dome timeout"
			<< sendLog;
		domeFailed = true;
		sw_state->setValueInteger (0);
		openDomeReal ();
		return -2;
	}
	if (isMoving ())
	{
		return USEC_SEC;
	}
	return (getState () & DOME_DOME_MASK) == DOME_OPENED ? 0 : -2;
}

int Watcher::endClose ()
{
	timeOpenClose = 0;
	dome_state = WATCHER_DOME_CLOSED;
	if (!domeFailed)
	{
		sw_state->setValueInteger (4);
	}
	return 0;
}

const char *Watcher::isOnString (int mask)
{
	return (sw_state->getValueInteger () & mask) ? "on" : "off";
}

int main (int argc, char **argv)
{
	Watcher device (argc, argv);
	return device.run ();
}
