/* 
 * Driver for IR (OpenTPL) dome.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/connmodbus.h"

// Zelio registers

#define ZREG_J1XT1       16
#define ZREG_J2XT1       17
#define ZREG_J3XT1       18
#define ZREG_J4XT1       19

#define ZREG_O1XT1       20
#define ZREG_O2XT1       21
#define ZREG_O3XT1       22
#define ZREG_O4XT1       23

// bite mask for O1 and O2 registers
#define ZO_SW_AUTO       0x0001
#define ZO_SW_OPENCLOSE  0x0002
#define ZO_EP_OPEN       0x0004
#define ZO_EP_CLOSE      0x0008
#define ZO_RAIN          0x0010
#define ZO_STATE_OPEN    0x0020
#define ZO_STATE_CLOSE   0x0040
#define ZO_TIMEO_CLOSE   0x0080
#define ZO_TIMEO_OPEN    0x0100
#define ZO_MOT_OPEN      0x0200
#define ZO_MOT_CLOSE     0x0400
#define ZO_BLOCK_OPEN    0x0800
#define ZO_BLOCK_CLOSE   0x1000
#define ZO_IGNORE_RAIN   0x2000
#define ZO_EMERGENCY     0x4000
#define ZO_DEADMAN       0x8000

// bit mask for rain ignore
#define ZI_IGNORE_RAIN  0x8000

namespace rts2dome
{

/**
 * Driver for Bootes IR dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Zelio:public Dome
{
	private:
		HostString *host;
		int16_t deadManNum;
		time_t nextDeadCheck;

		Rts2ValueInteger *deadTimeout;

		Rts2ValueBool *rain;
		Rts2ValueBool *ignoreRain;
		Rts2ValueBool *automode;
		Rts2ValueBool *emergencyButton;

		Rts2ValueBool *swOpenLeft;
		Rts2ValueBool *swCloseLeft;
		Rts2ValueBool *swCloseRight;
		Rts2ValueBool *swOpenRight;

		Rts2ValueBool *motOpenLeft;
		Rts2ValueBool *motCloseLeft;
		Rts2ValueBool *motOpenRight;
		Rts2ValueBool *motCloseRight;

		Rts2ValueInteger *J1XT1;
		Rts2ValueInteger *J2XT1;
		Rts2ValueInteger *J3XT1;
		Rts2ValueInteger *J4XT1;

		Rts2ValueInteger *O1XT1;
		Rts2ValueInteger *O2XT1;
		Rts2ValueInteger *O3XT1;
		Rts2ValueInteger *O4XT1;

		rts2core::ConnModbus *zelioConn;

	protected:
		virtual int processOption (int in_opt);
		virtual int idle ();

		virtual int init ();

		virtual int setValue (Rts2Value *oldValue, Rts2Value *newValue);

		virtual bool isGoodWeather ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

	public:
		Zelio (int argc, char **argv);
		virtual ~Zelio (void);

		virtual int info ();
};

}

using namespace rts2dome;

int
Zelio::startOpen ()
{
	int ret;
	ret = zelioConn->writeHoldingRegister (ZREG_J1XT1, deadTimeout->getValueInteger ());
	if (ret)
		return ret;
	ret = zelioConn->writeHoldingRegister (ZREG_J2XT1, 0);
	if (ret)
		return ret;
	ret = zelioConn->writeHoldingRegister (ZREG_J2XT1, 1);
	if (ret)
		return ret;
	deadManNum = 0;
	return ret;
}


bool
Zelio::isGoodWeather ()
{
	if (getIgnoreMeteo ())
		return true;
	int ret;
	uint16_t reg;
	ret = zelioConn->readHoldingRegisters (ZREG_O1XT1, 1, &reg);
	if (ret)
		return false;
	ignoreRain->setValueBool (reg & ZO_IGNORE_RAIN);	
	// now check for rain..
	if (!(reg & ZO_RAIN))
	{
		rain->setValueBool (true);
		if (ignoreRain->getValueBool () == false)
			return false;
	}
	if (reg & ZO_EMERGENCY)
	{
		emergencyButton->setValueBool (true);
		return false;
	}
	return Dome::isGoodWeather ();
}


long
Zelio::isOpened ()
{
	int ret;
	uint16_t regs[2];
	ret = zelioConn->readHoldingRegisters (ZREG_O1XT1, 2, regs);
	if (ret)
		return -1;
	// check states of end switches..
	if ((regs[0] & ZO_EP_OPEN) && (regs[1] & ZO_EP_OPEN))
		return -2;
	return 0;
}


int
Zelio::endOpen ()
{
	return 0;
}


int
Zelio::startClose ()
{
	int ret;
	ret = zelioConn->writeHoldingRegister (ZREG_J1XT1, 0);
	sleep (2);
	return ret;
}


long
Zelio::isClosed ()
{
	int ret;
	uint16_t regs[2];
	ret = zelioConn->readHoldingRegisters (ZREG_O1XT1, 2, regs);
	if (ret)
		return -1;
	// check states of end switches..
	if ((regs[0] & ZO_EP_CLOSE) && (regs[1] & ZO_EP_CLOSE))
		return -2;
	return 0;
}


int
Zelio::endClose ()
{
	return 0;
}


int
Zelio::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'z':
			host = new HostString (optarg, "502");
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}


int
Zelio::idle ()
{
	if (isGoodWeather ())
	{
		if ((getState () & DOME_DOME_MASK) == DOME_OPENED || (getState () & DOME_DOME_MASK) == DOME_OPENING)
		{
			time_t now = time (NULL);
			if (now > nextDeadCheck)
			{
				zelioConn->writeHoldingRegister (ZREG_J2XT1, deadManNum);
				deadManNum = (++deadManNum) % 2;
				nextDeadCheck = now + deadTimeout->getValueInteger () / 5;
			}
		}
	}
	return Dome::idle ();
}


Zelio::Zelio (int argc, char **argv)
:Dome (argc, argv)
{
	createValue (deadTimeout, "dead_timeout", "timeout for dead man button", false);
	deadTimeout->setValueInteger (60);

	createValue (rain, "rain", "state of rain sensor", false);
	createValue (ignoreRain, "ignore_rain", "whenever rain is ignored (know issue with interference between dome and rain sensor)", false);
	createValue (automode, "automode", "state of automatic dome mode", false);
	createValue (emergencyButton, "emmergency", "state of emergency button", false);

	createValue (swOpenLeft, "sw_open_left", "state of left open switch", false);
	createValue (swCloseLeft, "sw_close_left", "state of left close switch", false);
	createValue (swCloseRight, "sw_close_right", "state of right close switch", false);
	createValue (swOpenRight, "sw_open_right", "state of right open switch", false);

	createValue (motOpenLeft, "motor_open_left", "state of left opening motor", false);
	createValue (motCloseLeft, "motor_close_left", "state of left closing motor", false);
	createValue (motOpenRight, "motor_open_right", "state of right opening motor", false);
	createValue (motCloseRight, "motor_close_right", "state of right closing motor", false);

	createValue (J1XT1, "J1XT1", "first input", false, RTS2_DT_HEX);
	createValue (J2XT1, "J2XT1", "second input", false, RTS2_DT_HEX);
	createValue (J3XT1, "J3XT1", "third input", false, RTS2_DT_HEX);
	createValue (J4XT1, "J4XT1", "fourth input", false, RTS2_DT_HEX);

	createValue (O1XT1, "O1XT1", "first output", false, RTS2_DT_HEX);
	createValue (O2XT1, "O2XT1", "second output", false, RTS2_DT_HEX);
	createValue (O3XT1, "O3XT1", "third output", false, RTS2_DT_HEX);
	createValue (O4XT1, "O4XT1", "fourth output", false, RTS2_DT_HEX);

	host = NULL;
	deadManNum = 0;
	nextDeadCheck = 0;

	addOption ('z', NULL, 1, "Zelio TCP/IP address and port (separated by :)");
}


Zelio::~Zelio (void)
{
	delete zelioConn;
	delete host;
}


int
Zelio::info ()
{
	uint16_t regs[8];
	int ret;
	ret = zelioConn->readHoldingRegisters (16, 8, regs);
	if (ret)
		return -1;

	rain->setValueBool (!(regs[4] & ZO_RAIN));
	ignoreRain->setValueBool (regs[4] & ZO_IGNORE_RAIN);
	automode->setValueBool (regs[4] & ZO_SW_AUTO);
	emergencyButton->setValueBool (regs[4] & ZO_EMERGENCY);

	swOpenLeft->setValueBool (regs[4] & ZO_EP_OPEN);
	swCloseLeft->setValueBool (regs[4] & ZO_EP_CLOSE);
	swCloseRight->setValueBool (regs[5] & ZO_EP_CLOSE);
	swOpenRight->setValueBool (regs[5] & ZO_EP_OPEN);

	motOpenLeft->setValueBool (regs[4] & ZO_MOT_OPEN);
	motCloseLeft->setValueBool (regs[4] & ZO_MOT_CLOSE);
	motOpenRight->setValueBool (regs[5] & ZO_MOT_OPEN);
	motCloseRight->setValueBool (regs[5] & ZO_MOT_CLOSE);

	J1XT1->setValueInteger (regs[0]);
	J2XT1->setValueInteger (regs[1]);
	J3XT1->setValueInteger (regs[2]);
	J4XT1->setValueInteger (regs[3]);

	O1XT1->setValueInteger (regs[4]);
	O2XT1->setValueInteger (regs[5]);
	O3XT1->setValueInteger (regs[6]);
	O4XT1->setValueInteger (regs[7]);

	zelioConn->readHoldingRegisters (32, 4, regs);
	return Dome::info ();
}


int
Zelio::init ()
{
	int ret = Dome::init ();
	if (ret)
		return ret;

	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify zeliho hostname (with -z option)." << sendLog;
		return -1;
	}
	zelioConn = new rts2core::ConnModbus (this, host->getHostname (), host->getPort ());
	ret = zelioConn->init ();
	if (ret)
		return ret;
	ret = info ();
	if (ret)
		return ret;
	// switch on dome state
	if (swOpenLeft->getValueBool () == true && swOpenRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_OPENED, "initial dome state is opened");
	}
	else if (swCloseLeft->getValueBool () == true && swCloseRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_CLOSED, "initial dome state is closed");
	}
	else if (motOpenLeft->getValueBool () == true || motOpenRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_OPENING, "initial dome state is opening");
	}
	else if (motCloseLeft->getValueBool () == true || motCloseRight->getValueBool () == true)
	{
		maskState (DOME_DOME_MASK, DOME_CLOSING, "initial dome state is closing");
	}
	return 0;
}


int
Zelio::setValue (Rts2Value *oldValue, Rts2Value *newValue)
{
	if (oldValue == J1XT1)
		return zelioConn->writeHoldingRegister (ZREG_J1XT1, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == J2XT1)
		return zelioConn->writeHoldingRegister (ZREG_J2XT1, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == J3XT1)
		return zelioConn->writeHoldingRegister (ZREG_J3XT1, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == J4XT1)
		return zelioConn->writeHoldingRegister (ZREG_J4XT1, newValue->getValueInteger ()) == 0 ? 0 : -2;
	return Dome::setValue (oldValue, newValue);
}


int
main (int argc, char **argv)
{
	Zelio device = Zelio (argc, argv);
	return device.run ();
}
