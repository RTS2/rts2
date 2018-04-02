/*
 * Driver for Servo-drive.com controller.
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
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

#include "Axisd.hpp"
#include "connection/serial.h"

namespace rts2axisd
{

/**
 * Servo drive controller.
 * http://www.servo-drive.com/pdf_catalog/manual_r272.pdf
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ServoDrive:public Axis
{
	public:
		ServoDrive (int in_argc, char **in_argv);
		virtual ~ ServoDrive (void);

		virtual int initHardware ();
		virtual int info ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int moveTo(int tcounts);
		virtual int processOption (int in_opt);

	private:
		rts2core::ConnSerial *servoDev;
		const char *dev;

		rts2core::ValueLong *target;
		rts2core::ValueLong *homeHigh;
		rts2core::ValueLong *minTarget;
		rts2core::ValueLong *maxTarget;
		rts2core::ValueTime *lastCommand;

		int sendCommand (const char *cmd);

		int beginLoading ();
		int executeProgramme ();

		int home (char d);
		int lastDiff;
};

};

using namespace rts2axisd;

ServoDrive::ServoDrive (int argc, char **argv):Axis (argc, argv)
{
	servoDev = NULL;
	dev = "/dev/ttyS0";

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");

	createValue (homeHigh, "home_high", "high home position", false, RTS2_VALUE_WRITABLE);
	homeHigh->setValueLong (3400);

	createValue (lastCommand, "last_cmd", "start last movement", false);

	lastDiff = 0;
}

ServoDrive::~ServoDrive (void)
{
	delete servoDev;
}

int ServoDrive::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Axis::processOption (in_opt);
	}
	return 0;
}

int ServoDrive::initHardware ()
{
	servoDev = new rts2core::ConnSerial (dev, this, rts2core::BS9600, rts2core::C8, rts2core::EVEN, 200);
	int ret = servoDev->init ();
	if (ret)
		return ret;
	servoDev->setDebug (getDebug ());
	servoDev->flushPortIO ();
	
	return 0;
}

int ServoDrive::info ()
{
	if (lastDiff != 0)
	{
		
	}
	return Axis::info ();
}

int ServoDrive::moveTo (int tcounts)
{
	servoDev->flushPortIO ();
	beginLoading ();
	long diff = tcounts - getTarget();
	sendCommand ("BG*");
	sendCommand ("EN*");
	if (diff == 0)
		return 0;
	if (diff > 0)
		sendCommand ("DL*");
	else
		sendCommand ("DR*");
	char cmd[20];
	snprintf (cmd, 20, "MV%ld*", labs (diff));
	sendCommand (cmd);
	sendCommand ("ED*");
	executeProgramme ();
	lastCommand->setNow ();
	lastDiff = diff;
	return 0;
}

int ServoDrive::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("home"))
	{
		char *mv;
		if (!conn->paramEnd ())
		{
			if (conn->paramNextStringNull (&mv) || !conn->paramEnd ())
				return -2;
			return home (mv[0]);
		}
		else
		{
			return home ('L');
		}
	}
	else if (conn->isCommand ("r"))
	{
		if (!conn->paramEnd ())
			return -2;
		return sendCommand ("ST1*") == 0 ? 0 : -2;
	}
	else if (conn->isCommand (COMMAND_STOP))
	{
		if (!conn->paramEnd ())
			return -2;
		return sendCommand ("SP*") == 0 ? 0 : -2;
	}
	return Axis::commandAuthorized (conn);
}

int ServoDrive::sendCommand (const char *cmd)
{
	char rbuf[strlen (cmd) + 5];
	int ret = servoDev->writeRead (cmd, strlen (cmd), rbuf, strlen (cmd) + 4);
	if (ret != 4)
		return -1;
	if (rbuf[0] != 'E' || isdigit (rbuf[1]) == 0 || isdigit (rbuf[2]) == 0 || rbuf[3] != '*')
		return -1;
	int ecode = (rbuf[1] - '0') * 10 + (rbuf[2] - '0');
	if (ecode == 10)
		return 0;
	return -ecode;
}

int ServoDrive::beginLoading ()
{
	return sendCommand ("LD1*");
}

int ServoDrive::executeProgramme ()
{
	return sendCommand ("ST1*");
}

int ServoDrive::home (char d)
{
	beginLoading ();
	sendCommand ("BG*");
	sendCommand ("EN*");
	char md = 'R';
	if (!(d == 'L' || d == 'H'))
		d = 'L';
	if (d == 'H')
		md = 'L';
		
	char dcmd[4] = "DR*";
	dcmd[1] = md;
	sendCommand (dcmd);
	char cmd[4] = "MH*";
	cmd[1] = d;
	sendCommand (cmd);
	sendCommand ("ED*");
	executeProgramme ();
	if (d == 'H')
	{
		target->setValueLong (homeHigh->getValueLong ());
	}
	else
	{
		target->setValueLong (0);
	}
	return 0;
}

int main (int argc, char **argv)
{
	ServoDrive device (argc, argv);
	return device.run ();
}
