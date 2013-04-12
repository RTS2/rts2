/* 
 * Driver for Zelio dome controll.
 * Copyright (C) 2008-2010 Petr Kubanek <petr@kubanek.net>
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
#include "connection/modbus.h"

#define EVENT_DEADBUT    RTS2_LOCAL_EVENT + 1350

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
#define ZS_3RELAYS       0x0080
#define ZS_HUMIDITY      0x0100
#define ZS_FRAM          0x0200
#define ZS_SIMPLE        0x0400
#define ZS_COMPRESSOR    0x0800
#define ZS_WEATHER       0x1000
#define ZS_OPENING_IGNR  0x2000
#define ZS_EMERGENCY_B   0x4000
#define ZS_DEADMAN       0x8000

// dead man timeout
#define ZI_DEADMAN_MASK  0x007f
// emergency button reset
#define ZI_EMMERGENCY_R  0x2000
// bit for Q9 - remote switch
#define ZI_Q9            0x4000

#define ZI_FRAM_Q8       0x0100
#define ZI_FRAM_Q9       0x0200
#define ZI_FRAM_QA       0x0400

// bit mask for rain ignore
#define ZI_IGNORE_RAIN   0x8000

#define OPT_BATTERY      OPT_LOCAL + 501
#define OPT_Q8_NAME      OPT_LOCAL + 502
#define OPT_Q9_NAME      OPT_LOCAL + 503
#define OPT_QA_NAME      OPT_LOCAL + 504
#define OPT_NO_POWER     OPT_LOCAL + 505

namespace rts2dome
{

/**
 * Driver for Bootes IR dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Zelio:public Dome
{
	public:
		Zelio (int argc, char **argv);
		virtual ~Zelio (void);

		virtual int info ();

		virtual void postEvent (rts2core::Event * event);

	protected:
		virtual int processOption (int in_opt);

		virtual int init ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual bool isGoodWeather ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

	private:
		HostString *host;
		int16_t deadManNum;

		enum { ZELIO_UNKNOW, ZELIO_BOOTES3_WOUTPLUGS, ZELIO_BOOTES3, ZELIO_COMPRESSOR_WOUTPLUGS, ZELIO_COMPRESSOR, ZELIO_SIMPLE, ZELIO_FRAM } zelioModel;

		// if model have hardware rain signal
		bool haveRainSignal;

		// if model have battery indicator
		bool haveBatteryLevel;

		// if model have humidity level indicator
		bool haveHumidityOutput;

		rts2core::ValueString *zelioModelString;

		rts2core::ValueInteger *deadTimeout;

		rts2core::ValueBool *rain;
		rts2core::ValueBool *openingIgnoreRain;
		rts2core::ValueBool *ignoreRain;
		rts2core::ValueBool *automode;
		rts2core::ValueBool *ignoreAutomode;
		rts2core::ValueBool *timeoutOccured;
		rts2core::ValueBool *onPower;
		bool createonPower;

		rts2core::ValueBool *weather;
		rts2core::ValueBool *emergencyButton;

		rts2core::ValueBool *swOpenLeft;
		rts2core::ValueBool *swCloseLeft;
		rts2core::ValueBool *swCloseRight;
		rts2core::ValueBool *swOpenRight;

		rts2core::ValueBool *motOpenLeft;
		rts2core::ValueBool *motCloseLeft;
		rts2core::ValueBool *motOpenRight;
		rts2core::ValueBool *motCloseRight;

		rts2core::ValueBool *timeoOpenLeft;
		rts2core::ValueBool *timeoCloseLeft;
		rts2core::ValueBool *timeoOpenRight;
		rts2core::ValueBool *timeoCloseRight;
	
		rts2core::ValueBool *blockOpenLeft;
		rts2core::ValueBool *blockCloseLeft;
		rts2core::ValueBool *blockOpenRight;
		rts2core::ValueBool *blockCloseRight;

		rts2core::ValueBool *emergencyReset;

		rts2core::ValueBool *Q8;
		rts2core::ValueBool *Q9;
		rts2core::ValueBool *QA;

		const char *Q8_name;
		const char *Q9_name;
		const char *QA_name;

		rts2core::ValueFloat *battery;
		rts2core::ValueFloat *batteryMin;

		rts2core::ValueFloat *humidity;

		rts2core::ValueInteger *J1XT1;
		rts2core::ValueInteger *J2XT1;
		rts2core::ValueInteger *J3XT1;
		rts2core::ValueInteger *J4XT1;

		rts2core::ValueInteger *O1XT1;
		rts2core::ValueInteger *O2XT1;
		rts2core::ValueInteger *O3XT1;
		rts2core::ValueInteger *O4XT1;

		rts2core::ConnModbus *zelioConn;

	  	int setBitsInput (uint16_t reg, uint16_t mask, bool value);

		void createZelioValues ();

		float getHumidity (int vout) { return ((0.8 + ((float) vout) * 10.0f / 255.0f ) / 5.0f - 0.16) / 0.0062; }

		void sendSwInfo (uint16_t regs[2]);
};

}

using namespace rts2dome;

int Zelio::setBitsInput (uint16_t reg, uint16_t mask, bool value)
{
	uint16_t oldValue;
	try
	{
		zelioConn->readHoldingRegisters (reg, 1, &oldValue);
		// switch mask..
		oldValue &= ~mask;
		if (value)
			oldValue |= mask;
		zelioConn->writeHoldingRegister (reg, oldValue);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}
	return 0;
}

int Zelio::startOpen ()
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
		if ((zelioModel == ZELIO_BOOTES3_WOUTPLUGS || zelioModel == ZELIO_BOOTES3) && onPower && !(reg & ZS_POWER))
		{
			logStream (MESSAGE_WARNING) << "power failure" << sendLog;
			return -1;
		}
		if (haveRainSignal && !(reg & ZS_RAIN) && !(reg_J1 & ZI_IGNORE_RAIN))
		{
			logStream (MESSAGE_WARNING) << "it is raining and rain is not ignored" << sendLog;
			return -1;
		}

		if (haveBatteryLevel && battery->getValueFloat () < batteryMin->getValueFloat ())
		{
			logStream (MESSAGE_WARNING) << "current battery level (" << battery->getValueFloat () << ") is bellow minimal level (" << batteryMin->getValueFloat () << sendLog;
		}

		zelioConn->writeHoldingRegisterMask (ZREG_J1XT1, ZI_DEADMAN_MASK, deadTimeout->getValueInteger ());
		zelioConn->writeHoldingRegister (ZREG_J2XT1, 0);
		zelioConn->writeHoldingRegister (ZREG_J2XT1, 1);
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		return -1;
	}
	deadManNum = 0;
	addTimer (deadTimeout->getValueInteger () / 5.0, new rts2core::Event (EVENT_DEADBUT, this));
	return 0;
}

bool Zelio::isGoodWeather ()
{
	if (getIgnoreMeteo ())
		return true;
	if (ignoreAutomode->getValueBool () == true && isOpened () == -2)
		return true;
	uint16_t reg;
	uint16_t reg3;
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O4XT1, 1, &reg);
		if (haveBatteryLevel || haveHumidityOutput)
			zelioConn->readHoldingRegisters (ZREG_O3XT1, 1, &reg3);
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
	if (haveBatteryLevel)
	{
		battery->setValueFloat (reg3 * 24.0f / 255.0f);
		sendValueAll (battery);
	}
	if (haveHumidityOutput)
	{
		humidity->setValueFloat (getHumidity (reg3));
		sendValueAll (humidity);
	}
	weather->setValueBool (reg & ZS_WEATHER);
	sendValueAll (weather);
	// now check for rain..
	if (haveRainSignal && !(reg & ZS_RAIN) && weather->getValueBool () == false)
	{
	  	valueError (rain);
		setWeatherTimeout (3600, "raining");
		return false;
	}
	else if (rain)
	{
		valueGood (rain);
	}
	// battery too weak
	if (haveBatteryLevel && battery->getValueFloat () < batteryMin->getValueFloat ())
	{
		valueError (battery);
		setWeatherTimeout (300, "battery level too low");
		return false;
	}
	else if (battery)
	{
		valueGood (battery);
	}
	// not in auto mode..
	automode->setValueBool (reg & ZS_SW_AUTO);
	if (automode->getValueBool () == false)
	{
	  	valueError (automode);
		if (ignoreAutomode->getValueBool () == false)
		{
	  		setWeatherTimeout (30, "not in auto mode");
			return false;
		}
		else if (isOpened () != -2)
		{
			setWeatherTimeout (30, "automode ignored, but dome is not opened");
			return false;
		}
	}
	else
	{
		valueGood (automode);
	}
	sendValueAll (automode);

	return Dome::isGoodWeather ();
}

long Zelio::isOpened ()
{
	uint16_t regs[2];
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O1XT1, 2, regs);
		sendSwInfo (regs);
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
		case ZELIO_BOOTES3_WOUTPLUGS:
		case ZELIO_BOOTES3:
		case ZELIO_COMPRESSOR_WOUTPLUGS:
		case ZELIO_COMPRESSOR:
		case ZELIO_FRAM:
			if ((regs[0] & ZO_EP_OPEN) && (regs[1] & ZO_EP_OPEN))
				return -2;
			break;
		case ZELIO_UNKNOW:
			break;
	}

	return 0;
}

int Zelio::endOpen ()
{
	return 0;
}

int Zelio::startClose ()
{
	try
	{
		uint16_t reg;
		zelioConn->writeHoldingRegisterMask (ZREG_J1XT1, ZI_DEADMAN_MASK, 0);
		// update automode status..
		zelioConn->readHoldingRegisters (ZREG_O4XT1, 1, &reg);
		automode->setValueBool (reg & ZS_SW_AUTO);
		// reset ignore rain value
		if (ignoreRain->getValueBool ())
		{
	  		setBitsInput (ZREG_J1XT1, ZI_IGNORE_RAIN, false);
			ignoreRain->setValueBool (false);
		}
		if (automode->getValueBool () == false)
		{
			return 0;
		}
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
	 	return -1;
	}
	// 20 minutes timeout..
	setWeatherTimeout (1200, "closed, timeout for opening (to allow dissipate motor heat)");
	return 0;
}

long Zelio::isClosed ()
{
	uint16_t regs[2];
	try
	{
		zelioConn->readHoldingRegisters (ZREG_O1XT1, 2, regs);
		sendSwInfo (regs);
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
		case ZELIO_BOOTES3_WOUTPLUGS:
		case ZELIO_BOOTES3:
		case ZELIO_COMPRESSOR_WOUTPLUGS:
		case ZELIO_COMPRESSOR:
		case ZELIO_FRAM:
			if ((regs[0] & ZO_EP_CLOSE) && (regs[1] & ZO_EP_CLOSE))
				return -2;
			break;
		case ZELIO_UNKNOW:
			break;
	}

	return 0;
}

int Zelio::endClose ()
{
	return 0;
}

int Zelio::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'z':
			host = new HostString (optarg, "502");
			break;
		case OPT_NO_POWER:
			createonPower = false;
			break;
		case OPT_BATTERY:
			batteryMin->setValueCharArr (optarg);
			break;
		case OPT_Q8_NAME:
			Q8_name = optarg;
			break;
		case OPT_Q9_NAME:
			Q9_name = optarg;
			break;
		case OPT_QA_NAME:
			QA_name = optarg;
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}

void Zelio::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_DEADBUT:
			if (isGoodWeather () && ((getState () & DOME_DOME_MASK) == DOME_OPENED || (getState () & DOME_DOME_MASK) == DOME_OPENING))
			{
			  	try
				{
					zelioConn->writeHoldingRegister (ZREG_J2XT1, deadManNum);
				}
				catch (rts2core::ConnError err)
				{
					logStream (MESSAGE_ERROR) << err << sendLog;
				}
				deadManNum = (deadManNum + 1) % 2;
				addTimer (deadTimeout->getValueInteger () / 5.0, event);
				return;
			}
	}
	Dome::postEvent (event);
}

Zelio::Zelio (int argc, char **argv):Dome (argc, argv)
{
	zelioModel = ZELIO_UNKNOW;
	haveRainSignal = true;
	haveBatteryLevel = false;
	haveHumidityOutput = false;

	createValue (zelioModelString, "zelio_model", "String with Zelio model", false);

	createValue (deadTimeout, "dead_timeout", "timeout for dead man button", false, RTS2_VALUE_WRITABLE);
	deadTimeout->setValueInteger (60);

	createValue (automode, "automode", "state of automatic dome mode", false, RTS2_DT_ONOFF);
	createValue (ignoreAutomode, "automode_ignore", "do not switch to off when not in automatic", false, RTS2_VALUE_WRITABLE);
	ignoreAutomode->setValueBool (false);

	createValue (timeoutOccured, "timeout_occured", "on if timeout occured", false);
	createValue (weather, "weather", "true if weather is (for some reason) believed to be fine", false);
	createValue (emergencyButton, "emergency", "state of emergency button", false);

	createValue (emergencyReset, "reset_emergency", "(re)set emergency state -cycle true/false to reset", false, RTS2_VALUE_WRITABLE);
	emergencyReset->setValueBool (false);

	host = NULL;
	deadManNum = 0;

	battery = NULL;
	batteryMin = NULL;

	onPower = NULL;
	createonPower = true;

	humidity = NULL;

	Q8 = NULL;
	Q9 = NULL;
	QA = NULL;

	Q8_name = "Q8_switch";
	Q9_name = "Q9_switch";
	QA_name = "QA_switch";

	addOption ('z', NULL, 1, "Zelio TCP/IP address and port (separated by :)");
	addOption (OPT_NO_POWER, "without-on-power", 0, "do not create onPower value (some Zelios does not support it)");
	addOption (OPT_BATTERY, "min-battery", 1, "minimal battery level [V])");

	addOption (OPT_Q8_NAME, "Q8-name", 1, "name of the Q8 switch");
	addOption (OPT_Q9_NAME, "Q9-name", 1, "name of the Q9 switch");
	addOption (OPT_QA_NAME, "QA-name", 1, "name of the QA switch");
}

Zelio::~Zelio (void)
{
	delete zelioConn;
	delete host;
}

int Zelio::info ()
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

		sendValueAll (rain);
		sendValueAll (ignoreRain);
		sendValueAll (openingIgnoreRain);
	}
	automode->setValueBool (regs[7] & ZS_SW_AUTO);
	sendValueAll (automode);

	timeoutOccured->setValueBool (regs[7] & ZS_TIMEOUT);
	if (timeoutOccured->getValueBool ())
		valueError (timeoutOccured);
	else
		valueGood (timeoutOccured);

	weather->setValueBool (regs[7] & ZS_WEATHER);
	emergencyButton->setValueBool (regs[7] & ZS_EMERGENCY_B);

	sendValueAll (weather);
	sendValueAll (emergencyButton);

	switch (zelioModel)
	{
	 	case ZELIO_BOOTES3_WOUTPLUGS:
		case ZELIO_BOOTES3:
			if (onPower)
			{
				onPower->setValueBool (regs[7] & ZS_POWER);

				sendValueAll (onPower);
			}
		case ZELIO_FRAM:
		case ZELIO_COMPRESSOR_WOUTPLUGS:
		case ZELIO_COMPRESSOR:
		case ZELIO_SIMPLE:
			sendSwInfo (regs + 4);
		case ZELIO_UNKNOW:
			break;
	}

	if (haveBatteryLevel)
	{
		battery->setValueFloat (regs[6] * 24.0f / 255.0f);
		sendValueAll (battery);
	}

	if (haveHumidityOutput)
	{
		humidity->setValueFloat (getHumidity (regs[6]));
		sendValueAll (humidity);
	}

	J1XT1->setValueInteger (regs[0]);
	J2XT1->setValueInteger (regs[1]);
	J3XT1->setValueInteger (regs[2]);
	J4XT1->setValueInteger (regs[3]);

	sendValueAll (J1XT1);
	sendValueAll (J2XT1);
	sendValueAll (J3XT1);
	sendValueAll (J4XT1);

	O1XT1->setValueInteger (regs[4]);
	O2XT1->setValueInteger (regs[5]);
	O3XT1->setValueInteger (regs[6]);
	O4XT1->setValueInteger (regs[7]);

	sendValueAll (O1XT1);
	sendValueAll (O2XT1);
	sendValueAll (O3XT1);
	sendValueAll (O4XT1);

	return Dome::info ();
}

int Zelio::init ()
{
	int ret = Dome::init ();
	if (ret)
		return ret;

	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify zelio hostname (with -z option)." << sendLog;
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
	int model = regs[7] & (ZS_COMPRESSOR | ZS_SIMPLE | ZS_FRAM | ZS_3RELAYS);
	switch (model)
	{
		case 0:
			zelioModel = ZELIO_BOOTES3_WOUTPLUGS;
			zelioModelString->setValueString ("ZELIO_BOOTES3_WITHOUT_PLUGS");
			break;
		case ZS_3RELAYS:
			zelioModel = ZELIO_BOOTES3;
			zelioModelString->setValueString ("ZELIO_BOOTES3");
			break;
		case ZS_COMPRESSOR | ZS_FRAM:
		case ZS_COMPRESSOR | ZS_FRAM | ZS_3RELAYS:
			zelioModel = ZELIO_FRAM;
			haveBatteryLevel = true;
			zelioModelString->setValueString ("ZELIO_FRAM");
			break;
		case ZS_COMPRESSOR:
			zelioModel = ZELIO_COMPRESSOR_WOUTPLUGS;
			zelioModelString->setValueString ("ZELIO_COMPRESSOR_WOUTPLUGS");
			break;
		case ZS_COMPRESSOR | ZS_3RELAYS:
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

	haveHumidityOutput = regs[7] & ZS_HUMIDITY;

	createZelioValues ();

	ret = info ();
	if (ret)
		return ret;

	// switch on dome state
	switch (zelioModel)
	{
		case ZELIO_BOOTES3_WOUTPLUGS:
		case ZELIO_BOOTES3:
		case ZELIO_COMPRESSOR_WOUTPLUGS:
		case ZELIO_COMPRESSOR:
		case ZELIO_FRAM:
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
	addTimer (1, new rts2core::Event (EVENT_DEADBUT, this));
	return 0;
}

void Zelio::createZelioValues ()
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
	
		case ZELIO_BOOTES3_WOUTPLUGS:
		case ZELIO_BOOTES3:
			if (createonPower)
				createValue (onPower, "on_power", "true if power is connected", false);
		case ZELIO_FRAM:
		case ZELIO_COMPRESSOR_WOUTPLUGS:
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
			createValue (blockCloseRight, "block_close_right", "right close block", false);

			break;
		case ZELIO_UNKNOW:
			break;
	}

	if (haveBatteryLevel)
	{
		createValue (battery, "battery", "[V] battery level", false);
		createValue (batteryMin, "battery_min", "[V] battery minimal level for good weather", false, RTS2_VALUE_WRITABLE);
		batteryMin->setValueFloat (0);
	}

	if (haveHumidityOutput)
	{
		createValue (humidity, "humidity", "Humidity sensor raw output", false);
	}

	if (zelioModel == ZELIO_FRAM || zelioModel == ZELIO_BOOTES3)
	{
		createValue (Q8, Q8_name, "Q8 switch", false, RTS2_VALUE_WRITABLE);
		createValue (Q9, Q9_name, "Q9 switch", false, RTS2_VALUE_WRITABLE);
		createValue (QA, QA_name, "QA switch", false, RTS2_VALUE_WRITABLE);
	}
	else 
	{
		createValue (Q9, Q9_name, "Q9 switch", false, RTS2_VALUE_WRITABLE);
	}

	// create rain values only if rain sensor is present
	if (haveRainSignal)
	{
		createValue (rain, "rain", "state of rain sensor", false);
		createValue (openingIgnoreRain, "opening_ignore", "ignore rain during opening", false);
		createValue (ignoreRain, "ignore_rain", "whenever rain is ignored (know issue with interference between dome and rain sensor)", false, RTS2_VALUE_WRITABLE);
	}

	createValue (J1XT1, "J1XT1", "first input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);
	createValue (J2XT1, "J2XT1", "second input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);
	createValue (J3XT1, "J3XT1", "third input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);
	createValue (J4XT1, "J4XT1", "fourth input", false, RTS2_DT_HEX | RTS2_VALUE_WRITABLE);

	createValue (O1XT1, "O1XT1", "first output", false, RTS2_DT_HEX);
	createValue (O2XT1, "O2XT1", "second output", false, RTS2_DT_HEX);
	createValue (O3XT1, "O3XT1", "third output", false, RTS2_DT_HEX);
	createValue (O4XT1, "O4XT1", "fourth output", false, RTS2_DT_HEX);
}

int Zelio::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == emergencyReset)
	  	return setBitsInput (ZREG_J1XT1, ZI_EMMERGENCY_R, ((rts2core::ValueBool*) newValue)->getValueBool ()) == 0 ? 0 : -2;
	if (zelioModel == ZELIO_FRAM || zelioModel == ZELIO_BOOTES3)
	{
		if (oldValue == Q8)
		  	return setBitsInput (ZREG_J1XT1, ZI_FRAM_Q8, ((rts2core::ValueBool*) newValue)->getValueBool ()) == 0 ? 0 : -2;
		if (oldValue == Q9)
		  	return setBitsInput (ZREG_J1XT1, ZI_FRAM_Q9, ((rts2core::ValueBool*) newValue)->getValueBool ()) == 0 ? 0 : -2;
		if (oldValue == QA)
		  	return setBitsInput (ZREG_J1XT1, ZI_FRAM_QA, ((rts2core::ValueBool*) newValue)->getValueBool ()) == 0 ? 0 : -2;
	}
	else
	{
		if (oldValue == Q9)
		  	return setBitsInput (ZREG_J1XT1, ZI_Q9, ((rts2core::ValueBool*) newValue)->getValueBool ()) == 0 ? 0 : -2;
	}
	if (oldValue == ignoreRain)
	  	return setBitsInput (ZREG_J1XT1, ZI_IGNORE_RAIN, ((rts2core::ValueBool*) newValue)->getValueBool ()) == 0 ? 0 : -2;
	if (oldValue == deadTimeout)
		return 0;
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

void Zelio::sendSwInfo (uint16_t regs[2])
{
	switch (zelioModel)
	{
	 	case ZELIO_BOOTES3_WOUTPLUGS:
		case ZELIO_BOOTES3:
		case ZELIO_FRAM:
		case ZELIO_COMPRESSOR_WOUTPLUGS:
		case ZELIO_COMPRESSOR:
			swCloseRight->setValueBool (regs[1] & ZO_EP_CLOSE);
			swOpenRight->setValueBool (regs[1] & ZO_EP_OPEN);

			sendValueAll (swCloseRight);
			sendValueAll (swOpenRight);

			motOpenRight->setValueBool (regs[1] & ZO_MOT_OPEN);
			motCloseRight->setValueBool (regs[1] & ZO_MOT_CLOSE);

			sendValueAll (motOpenRight);
			sendValueAll (motCloseRight);

			timeoOpenRight->setValueBool (regs[1] & ZO_TIMEO_OPEN);
			timeoCloseRight->setValueBool (regs[1] & ZO_TIMEO_CLOSE);

			sendValueAll (timeoOpenRight);
			sendValueAll (timeoCloseRight);

			blockOpenRight->setValueBool (regs[1] & ZO_BLOCK_OPEN);
			blockCloseRight->setValueBool (regs[1] & ZO_BLOCK_CLOSE);

			sendValueAll (blockOpenRight);
			sendValueAll (blockCloseRight);

		case ZELIO_SIMPLE:
			swOpenLeft->setValueBool (regs[0] & ZO_EP_OPEN);
			swCloseLeft->setValueBool (regs[0] & ZO_EP_CLOSE);

			sendValueAll (swOpenLeft);
			sendValueAll (swCloseLeft);

			motOpenLeft->setValueBool (regs[0] & ZO_MOT_OPEN);
			motCloseLeft->setValueBool (regs[0] & ZO_MOT_CLOSE);

			sendValueAll (motOpenLeft);
			sendValueAll (motCloseLeft);

			timeoOpenLeft->setValueBool (regs[0] & ZO_TIMEO_OPEN);
			timeoCloseLeft->setValueBool (regs[0] & ZO_TIMEO_CLOSE);

			sendValueAll (timeoOpenLeft);
			sendValueAll (timeoCloseLeft);

			blockOpenLeft->setValueBool (regs[0] & ZO_BLOCK_OPEN);
			blockCloseLeft->setValueBool (regs[0] & ZO_BLOCK_CLOSE);

			sendValueAll (blockOpenLeft);
			sendValueAll (blockCloseLeft);
			break;
		case ZELIO_UNKNOW:
			break;
	}
}

int main (int argc, char **argv)
{
	Zelio device (argc, argv);
	return device.run ();
}
