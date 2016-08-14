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

#include "sensord.h"
#include "connection/serial.h"

namespace rts2sensord
{

/**
 * Servo drive controller.
 * http://www.servo-drive.com/pdf_catalog/manual_r272.pdf
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ServoDrive:public Sensor
{
	public:
		ServoDrive (int in_argc, char **in_argv);
		virtual ~ ServoDrive (void);

		virtual int initHardware ();
		virtual int info ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
		virtual int processOption (int in_opt);

	private:
		rts2core::ConnSerial *servoDev;
		const char *dev;

		rts2core::ValueLong *target;
		rts2core::ValueLong *minTarget;
		rts2core::ValueLong *maxTarget;

		int sendCommand (const char *cmd);

		int beginLoading ();
		int executeProgramme ();

		int home (char d);
};

};

using namespace rts2sensord;

ServoDrive::ServoDrive (int argc, char **argv):Sensor (argc, argv)
{
	servoDev = NULL;
	dev = "/dev/ttyS0";

	addOption ('f', NULL, 1, "/dev/ttySx entry (defaults to /dev/ttyS0");

	createValue (target, "TARGET", "target position", false, RTS2_VALUE_WRITABLE);
	createValue (minTarget , "MIN", "minimal axis value",  false, RTS2_VALUE_WRITABLE);
	createValue (maxTarget, "MAX", "maximal target value", false, RTS2_VALUE_WRITABLE);
	minTarget->setValueLong (INT_MIN);
	maxTarget->setValueLong (INT_MAX);
}

ServoDrive::~ServoDrive (void)
{
	delete servoDev;
}

int ServoDrive::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == target)
	{
		if (new_value->getValueLong () < minTarget->getValueLong () || new_value->getValueLong () > maxTarget->getValueLong ())
			return -2;
		servoDev->flushPortIO ();
		beginLoading ();
		long diff = new_value->getValueLong () - old_value->getValueLong ();
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
		return 0;
	}
	return Sensor::setValue (old_value, new_value);
}

int ServoDrive::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			dev = optarg;
			break;
		default:
			return Sensor::processOption (in_opt);
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
	return Sensor::info ();
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
			return home ('H');
		}
	}
	if (conn->isCommand ("r"))
	{
		if (!conn->paramEnd ())
			return -2;
		return sendCommand ("ST1*") == 0 ? 0 : -2;
	}
	return Sensor::commandAuthorized (conn);
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
		d = 'H';
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
	return 0;
}

int main (int argc, char **argv)
{
	ServoDrive device (argc, argv);
	return device.run ();
}
