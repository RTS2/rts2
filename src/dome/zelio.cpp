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
#define ZO_EP_OPEN       0x0004
#define ZO_EP_CLOSE      0x0008
#define ZO_STATE_OPEN    0x0020
#define ZO_STATE_CLOSE   0x0040
#define ZO_TIMEO_CLOSE   0x0080
#define ZO_TIMEO_OPEN    0x0100
#define ZO_MOT_OPEN      0x0200
#define ZO_MOT_CLOSE     0x0400
#define ZO_BLOCK_OPEN    0x0800
#define ZO_BLOCK_CLOSE   0x1000

// bite mask for state register
#define ZS_SW_AUTO       0x0001
#define ZS_SW_OPENCLOSE  0x0002
#define ZS_TIMEOUT       0x0004
#define ZS_POWER         0x0008
#define ZS_RAIN          0x0010
#define ZS_COMP_RUN      0x0020
// does not have rain signal
#define ZS_WITHOUT_RAIN  0x0040
#define ZS_SIMPLE        0x0400
#define ZS_COMPRESSOR    0x0800
#define ZS_WEATHER       0x1000
#define ZS_OPENING_IGNR  0x2000
#define ZS_EMERGENCY_B   0x4000
#define ZS_DEADMAN       0x8000

// emergency button reset
#define ZI_EMMERGENCY_R  0x2000
// bit for Q9 - remote switch
#define ZI_Q9            0x4000
// bit mask for rain ignore
#define ZI_IGNORE_RAIN   0x8000


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

		enum { ZELIO_UNKNOW, ZELIO_BOOTES3, ZELIO_COMPRESSOR, ZELIO_SIMPLE } zelioModel;

		// if model have hardware rain signal
		bool haveRainSignal;

		Rts2ValueString *zelioModelString;

		Rts2ValueInteger *deadTimeout;

		Rts2ValueBool *rain;
		Rts2ValueBool *openingIgnoreRain;
		Rts2ValueBool *ignoreRain;
		Rts2ValueBool *automode;
		Rts2ValueBool *timeoutOccured;
		Rts2ValueBool *onPower;
		Rts2ValueBool *weather;
		Rts2ValueBool *emergencyButton;

		Rts2ValueBool *swOpenLeft;
		Rts2ValueBool *swCloseLeft;
		Rts2ValueBool *swCloseRight;
		Rts2ValueBool *swOpenRight;

		Rts2ValueBool *motOpenLeft;
		Rts2ValueBool *motCloseLeft;
		Rts2ValueBool *motOpenRight;
		Rts2ValueBool *motCloseRight;

		Rts2ValueBool *timeoOpenLeft;
		Rts2ValueBool *timeoCloseLeft;
		Rts2ValueBool *timeoOpenRight;
		Rts2ValueBool *timeoCloseRight;
	
		Rts2ValueBool *blockOpenLeft;
		Rts2ValueBool *blockCloseLeft;
		Rts2ValueBool *blockOpenRight;
		Rts2ValueBool *blockCloseRight;

		Rts2ValueBool *emergencyReset;
		Rts2ValueBool *Q9;

		Rts2ValueInteger *J1XT1;
		Rts2ValueInteger *J2XT1;
		Rts2ValueInteger *J3XT1;
		Rts2ValueInteger *J4XT1;

		Rts2ValueInteger *O1XT1;
		Rts2ValueInteger *O2XT1;
		Rts2ValueInteger *O3XT1;
		Rts2ValueInteger *O4XT1;

		rts2core::ConnModbus *zelioConn;

	  	int setBitsInput (uint16_t reg, uint16_t mask, bool value);

		void createZelioValues ();

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
Zelio::setBitsInput (uint16_t reg, uint16_t mask, bool value)
{
	uint16_t oldValue;
	try
	{
		zelioConn->readHoldingRegisters (ZREG_J1XT1, 1, &oldValue);
		// switch mask..
		oldValue &= ~mask;
		if (value)
			oldValue |= mask;
		zelioConn->writeHoldingRegister (ZREG_J1XT1, oldValue);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}
	return 0;
}

int
Zelio::startOpen ()
{
	// check auto state..
	uint16_t reg;
	uint16_t reg_J1;
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O4XT1, 1, &reg);
		zelioConn->readHoldingRegisters (ZREG_J1XT1, 1, &reg_J1);
		if (!(reg & ZS_SW_AUTO))
		{
			logStream (MESSAGE_WARNING) << "dome not in auto mode" << sendLog;
			return -1;
		}
		if (reg & ZS_EMERGENCY_B)
		{
			logStream (MESSAGE_WARNING) << "emergency button pusshed" << sendLog;
			return -1;
		}
		if (reg & ZS_TIMEOUT)
		{
			logStream (MESSAGE_WARNING) << "timeout occured" << sendLog;
			return -1;
		}
		if (zelioModel == ZELIO_BOOTES3 && !(reg & ZS_POWER))
		{
			logStream (MESSAGE_WARNING) << "power failure" << sendLog;
			return -1;
		}
		if (haveRainSignal && !(reg & ZS_RAIN) && !(reg_J1 & ZI_IGNORE_RAIN))
		{
			logStream (MESSAGE_WARNING) << "it is raining and rain is not ignored" << sendLog;
			return -1;
		}

		zelioConn->writeHoldingRegister (ZREG_J1XT1, deadTimeout->getValueInteger ());
		zelioConn->writeHoldingRegister (ZREG_J2XT1, 0);
		zelioConn->writeHoldingRegister (ZREG_J2XT1, 1);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}
	deadManNum = 0;
	return 0;
}


bool
Zelio::isGoodWeather ()
{
	if (getIgnoreMeteo ())
		return true;
	uint16_t reg;
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O4XT1, 1, &reg);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return false;
	}
	if (haveRainSignal)
	{
		rain->setValueBool (!(reg & ZS_RAIN));
		sendValueAll (rain);
		openingIgnoreRain->setValueBool (reg & ZS_OPENING_IGNR);
		sendValueAll (openingIgnoreRain);
	}
	weather->setValueBool (reg & ZS_WEATHER);
	sendValueAll (weather);
	// now check for rain..
	if (haveRainSignal && !(reg & ZS_RAIN) && weather->getValueBool () == false)
	{
		setWeatherTimeout (3600);
		return false;
	}
	// not in auto mode..
	if (!(reg & ZS_SW_AUTO))
	{
	  	setWeatherTimeout (30);
		return false;
	}
	return Dome::isGoodWeather ();
}


long
Zelio::isOpened ()
{
	uint16_t regs[2];
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O1XT1, 2, regs);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}
	// check states of end switches..
	switch (zelioModel)
	{
		case ZELIO_SIMPLE:
			if ((regs[0] & ZO_EP_OPEN))
				return -2;
			break;
		case ZELIO_BOOTES3:
		case ZELIO_COMPRESSOR:
			if ((regs[0] & ZO_EP_OPEN) && (regs[1] & ZO_EP_OPEN))
				return -2;
			break;
		case ZELIO_UNKNOW:
			break;
	}

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
	try
	{
		zelioConn->writeHoldingRegister (ZREG_J1XT1, 0);
		if (automode->getValueBool () == false)
		{
		 	maskState (DOME_DOME_MASK, DOME_CLOSED, "dome is not controlled- it is in manual mode");
			return 0;
		}
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
	 	return -1;
	}
	// 20 minutes timeout..
	setWeatherTimeout (1200);
	return 0;
}


long
Zelio::isClosed ()
{
	uint16_t regs[2];
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O1XT1, 2, regs);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}
	// check states of end switches..
	switch (zelioModel)
	{
		case ZELIO_SIMPLE:
			if ((regs[0] & ZO_EP_CLOSE))
				return -2;
			break;
		case ZELIO_BOOTES3:
		case ZELIO_COMPRESSOR:
			if ((regs[0] & ZO_EP_CLOSE) && (regs[1] & ZO_EP_CLOSE))
				return -2;
			break;
		case ZELIO_UNKNOW:
			break;
	}

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
			  	try
				{
					zelioConn->writeHoldingRegister (ZREG_J2XT1, deadManNum);
				}
				catch (rts2core::ConnError err)
				{
					logStream (MESSAGE_ERROR) << err << sendLog;
				}
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
	zelioModel = ZELIO_UNKNOW;
	haveRainSignal = true;

	createValue (zelioModelString, "zelio_model", "String with Zelio model", false);

	createValue (deadTimeout, "dead_timeout", "timeout for dead man button", false);
	deadTimeout->setValueInteger (60);

	createValue (automode, "automode", "state of automatic dome mode", false);
	createValue (timeoutOccured, "timeout_occured", "on if timeout occured", false);
	createValue (weather, "weather", "true if weather is (for some reason) believed to be fine", false);
	createValue (emergencyButton, "emergency", "state of emergency button", false);

	createValue (emergencyReset, "(re)set emergency state -cycle true/false to reset", false);
	emergencyReset->setValueBool (false);

	createValue (Q9, "Q9_switch", "Q9 switch reset - apogee", false);
	Q9->setValueBool (false);

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
	try
	{
		zelioConn->readHoldingRegisters (16, 8, regs);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;	
		return -1;
	}

	if (haveRainSignal)
	{
		rain->setValueBool (!(regs[7] & ZS_RAIN));
		ignoreRain->setValueBool (regs[0] & ZI_IGNORE_RAIN);
		openingIgnoreRain->setValueBool (regs[7] & ZS_OPENING_IGNR);
	}
	automode->setValueBool (regs[7] & ZS_SW_AUTO);
	timeoutOccured->setValueBool (regs[7] & ZS_TIMEOUT);
	weather->setValueBool (regs[7] & ZS_WEATHER);
	emergencyButton->setValueBool (regs[7] & ZS_EMERGENCY_B);

	switch (zelioModel)
	{
		case ZELIO_BOOTES3:
			onPower->setValueBool (regs[7] & ZS_POWER);
		case ZELIO_COMPRESSOR:
			swCloseRight->setValueBool (regs[5] & ZO_EP_CLOSE);
			swOpenRight->setValueBool (regs[5] & ZO_EP_OPEN);

			motOpenRight->setValueBool (regs[5] & ZO_MOT_OPEN);
			motCloseRight->setValueBool (regs[5] & ZO_MOT_CLOSE);

			timeoOpenRight->setValueBool (regs[5] & ZO_TIMEO_OPEN);
			timeoCloseRight->setValueBool (regs[5] & ZO_TIMEO_CLOSE);

			blockOpenRight->setValueBool (regs[5] & ZO_BLOCK_OPEN);
			blockCloseRight->setValueBool (regs[5] & ZO_BLOCK_CLOSE);

		case ZELIO_SIMPLE:
			swOpenLeft->setValueBool (regs[4] & ZO_EP_OPEN);
			swCloseLeft->setValueBool (regs[4] & ZO_EP_CLOSE);

			motOpenLeft->setValueBool (regs[4] & ZO_MOT_OPEN);
			motCloseLeft->setValueBool (regs[4] & ZO_MOT_CLOSE);

			timeoOpenLeft->setValueBool (regs[4] & ZO_TIMEO_OPEN);
			timeoCloseLeft->setValueBool (regs[4] & ZO_TIMEO_CLOSE);

			blockOpenLeft->setValueBool (regs[4] & ZO_BLOCK_OPEN);
			blockCloseLeft->setValueBool (regs[4] & ZO_BLOCK_CLOSE);
			break;
		case ZELIO_UNKNOW:
			break;
	}

	J1XT1->setValueInteger (regs[0]);
	J2XT1->setValueInteger (regs[1]);
	J3XT1->setValueInteger (regs[2]);
	J4XT1->setValueInteger (regs[3]);

	O1XT1->setValueInteger (regs[4]);
	O2XT1->setValueInteger (regs[5]);
	O3XT1->setValueInteger (regs[6]);
	O4XT1->setValueInteger (regs[7]);

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
	
	uint16_t regs[8];

	try
	{
		zelioConn->init ();
		zelioConn->readHoldingRegisters (16, 8, regs);
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	// O4XT1
	int model = regs[7] & (ZS_COMPRESSOR | ZS_SIMPLE);
	switch (model)
	{
		case 0:
			zelioModel = ZELIO_BOOTES3;
			zelioModelString->setValueString ("ZELIO_BOOTES3");
			break;
		case ZS_COMPRESSOR:
			zelioModel = ZELIO_COMPRESSOR;
			zelioModelString->setValueString ("ZELIO_COMPRESSOR");
			break;
		case ZS_SIMPLE:
			zelioModel = ZELIO_SIMPLE;
			zelioModelString->setValueString ("ZELIO_SIMPLE");
			break;
		default:
			logStream (MESSAGE_ERROR) << "cannot retrieve dome model (" << model << ")" << sendLog;
			return -1;
	}
	// have this model rain signal?
	haveRainSignal = !(regs[7] & ZS_WITHOUT_RAIN);

	createZelioValues ();

	ret = info ();
	if (ret)
		return ret;

	// switch on dome state
	switch (zelioModel)
	{
		case ZELIO_BOOTES3:
		case ZELIO_COMPRESSOR:
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
			break;
		case ZELIO_SIMPLE:
			if (swOpenLeft->getValueBool () == true)
			{
				maskState (DOME_DOME_MASK, DOME_OPENED, "initial dome state is opened");
			}
			else if (swCloseLeft->getValueBool () == true)
			{
				maskState (DOME_DOME_MASK, DOME_CLOSED, "initial dome state is closed");
			}
			else if (motOpenLeft->getValueBool () == true)
			{
				maskState (DOME_DOME_MASK, DOME_OPENING, "initial dome state is opening");
			}
			  else if (motCloseLeft->getValueBool () == true)
			{
				maskState (DOME_DOME_MASK, DOME_CLOSING, "initial dome state is closing");
			}
			break;
		case ZELIO_UNKNOW:
			return -1;
	}
	setIdleInfoInterval (20);
	return 0;
}


void
Zelio::createZelioValues ()
{
	switch (zelioModel)
	{
		case ZELIO_SIMPLE:
			createValue (swOpenLeft, "sw_open", "state of open switch", false);
			createValue (swCloseLeft, "sw_close", "state of close switch", false);

			createValue (motOpenLeft, "motor_open", "state of opening motor", false);
			createValue (motCloseLeft, "motor_close", "state of closing motor", false);

			createValue (timeoOpenLeft, "timeo_open", "open timeout", false);
			createValue (timeoCloseLeft, "timeo_close", "close timeout", false);

			createValue (blockOpenLeft, "block_open", "open block", false);
			createValue (blockCloseLeft, "block_close", "close block", false);
			break;
		
		case ZELIO_BOOTES3:
			createValue (onPower, "on_power", "true if power is connected", false);
		case ZELIO_COMPRESSOR:
			createValue (swOpenLeft, "sw_open_left", "state of left open switch", false);
			createValue (swCloseLeft, "sw_close_left", "state of left close switch", false);
			createValue (swCloseRight, "sw_close_right", "state of right close switch", false);
			createValue (swOpenRight, "sw_open_right", "state of right open switch", false);

			createValue (motOpenLeft, "motor_open_left", "state of left opening motor", false);
			createValue (motCloseLeft, "motor_close_left", "state of left closing motor", false);
			createValue (motOpenRight, "motor_open_right", "state of right opening motor", false);
			createValue (motCloseRight, "motor_close_right", "state of right closing motor", false);

			createValue (timeoOpenLeft, "timeo_open_left", "left open timeout", false);
			createValue (timeoCloseLeft, "timeo_close_left", "left close timeout", false);
			createValue (timeoOpenRight, "timeo_open_right", "right open timeout", false);
			createValue (timeoCloseRight, "timeo_close_right", "right close timeout", false);

			createValue (blockOpenLeft, "block_open_left", "left open block", false);
			createValue (blockCloseLeft, "block_close_left", "left close block", false);
			createValue (blockOpenRight, "block_open_right", "right open block", false);
			createValue (blockCloseRight, "block_close_left", "left close block", false);

			break;
		case ZELIO_UNKNOW:
			break;
	}

	// create rain values only if rain sensor is present
	if (haveRainSignal)
	{
		createValue (rain, "rain", "state of rain sensor", false);
		createValue (openingIgnoreRain, "opening_ignore", "ignore rain during opening", false);
		createValue (ignoreRain, "ignore_rain", "whenever rain is ignored (know issue with interference between dome and rain sensor)", false);
	}

	createValue (J1XT1, "J1XT1", "first input", false, RTS2_DT_HEX);
	createValue (J2XT1, "J2XT1", "second input", false, RTS2_DT_HEX);
	createValue (J3XT1, "J3XT1", "third input", false, RTS2_DT_HEX);
	createValue (J4XT1, "J4XT1", "fourth input", false, RTS2_DT_HEX);

	createValue (O1XT1, "O1XT1", "first output", false, RTS2_DT_HEX);
	createValue (O2XT1, "O2XT1", "second output", false, RTS2_DT_HEX);
	createValue (O3XT1, "O3XT1", "third output", false, RTS2_DT_HEX);
	createValue (O4XT1, "O4XT1", "fourth output", false, RTS2_DT_HEX);
}


int
Zelio::setValue (Rts2Value *oldValue, Rts2Value *newValue)
{
	if (oldValue == emergencyReset)
	  	return setBitsInput (ZREG_J1XT1, ZI_EMMERGENCY_R, ((Rts2ValueBool*) newValue)->getValueBool ());
	if (oldValue == Q9)
	  	return setBitsInput (ZREG_J1XT1, ZI_Q9, ((Rts2ValueBool*) newValue)->getValueBool ());
	if (oldValue == ignoreRain)
	  	return setBitsInput (ZREG_J1XT1, ZI_IGNORE_RAIN, ((Rts2ValueBool*) newValue)->getValueBool ());
	try
	{
		if (oldValue == J1XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J1XT1, newValue->getValueInteger ());
			return 0;
		}
		else if (oldValue == J2XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J2XT1, newValue->getValueInteger ());
			return 0;
		}
		else if (oldValue == J3XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J3XT1, newValue->getValueInteger ());
			return 0;
		}
		else if (oldValue == J4XT1)
		{
			zelioConn->writeHoldingRegister (ZREG_J4XT1, newValue->getValueInteger ());
			return 0;
		}
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -2;
	}
	return Dome::setValue (oldValue, newValue);
}


int
main (int argc, char **argv)
{
	Zelio device = Zelio (argc, argv);
	return device.run ();
}
