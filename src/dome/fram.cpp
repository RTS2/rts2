/*
 * Driver for dome board for FRAM telescope, Pierre-Auger observatory, Argentina
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

#include "domeford.h"

using namespace rts2dome;

// check time in usec (1000000 ms = 1s)
#define FRAM_CHECK_TIMEOUT    1000

// following times are in seconds!
#define FRAM_TIME_OPEN_RIGHT    24
#define FRAM_TIME_OPEN_LEFT     26
#define FRAM_TIME_CLOSE_RIGHT   28
#define FRAM_TIME_CLOSE_LEFT    32

#define FRAM_TIME_RECLOSE_RIGHT 5
#define FRAM_TIME_RECLOSE_LEFT  5

// try to close dome again after n second
#define FRAM_TIME_CLOSING_RETRY 200

// if closing fails for n times, do not close again; send warning
// e-mail
#define FRAM_MAX_CLOSING_RETRY  3

// how many times to try to reclose dome
#define FRAM_RECLOSING_MAX      3

#define OPT_WDC_DEV		OPT_LOCAL + 130
#define OPT_WDC_TIMEOUT		OPT_LOCAL + 131
#define OPT_EXTRA_SWITCH	OPT_LOCAL + 132


typedef enum
{
	SWITCH_BATBACK,
	SPINAC_2,
	SPINAC_3,
	SPINAC_4,
	PLUG_4,
	PLUG_3,
	PLUG_PHOTOMETER,
	PLUG_1
} extraOuts;

typedef enum
{
	VENTIL_OTEVIRANI_LEVY,      // PORT_B, 1
	VENTIL_OTEVIRANI_PRAVY,     // PORT_B, 2
	VENTIL_ZAVIRANI_PRAVY,      // PORT_B, 4
	VENTIL_ZAVIRANI_LEVY,       // PORT_B, 8
	KONCAK_OTEVRENI_LEVY,       // PORT_B, 16
	KONCAK_OTEVRENI_PRAVY,      // PORT_B, 32
	KONCAK_ZAVRENI_PRAVY,       // PORT_B, 64
	KONCAK_ZAVRENI_LEVY,        // PORT_B, 128
	KOMPRESOR,                  // PORT_A, 1
	ZASUVKA_PRAVA,              // PORT_A, 2
	ZASUVKA_LEVA,               // PORT_A, 4
	PORT_A_8,
	PORT_A_16,
	VENTIL_AKTIVACNI            // PORT_A, 32
}
vystupy;

namespace rts2dome
{

class Fram:public Ford
{
	private:
		rts2core::ConnSerial *wdcConn;
		char *wdc_file;

		rts2core::FordConn *extraSwitch;

		char *extraSwitchFile;

		rts2core::ValueDouble *wdcTimeOut;
		rts2core::ValueDouble *wdcTemperature;

		rts2core::ValueBool *swOpenLeft;
		rts2core::ValueBool *swCloseLeft;
		rts2core::ValueBool *swCloseRight;
		rts2core::ValueBool *swOpenRight;

		rts2core::ValueBool *valveOpenLeft;
		rts2core::ValueBool *valveCloseLeft;
		rts2core::ValueBool *valveOpenRight;
		rts2core::ValueBool *valveCloseRight;

		rts2core::ValueInteger *reclosing_num;

		rts2core::ValueBool *switchBatBack;
		rts2core::ValueBool *plug1;
		rts2core::ValueBool *plug_photometer;
		rts2core::ValueBool *plug3;
		rts2core::ValueBool *plug4;

		int zjisti_stav_portu_rep ();

		const char *isOnString (int c_port);

		/**
		 *
		 * Handles move commands without changing our state.
		 *
		 */
		int openLeftMove ();
		int openRightMove ();

		int closeRightMove ();
		int closeLeftMove ();

		/**
		 *
		 * Add state handling for basic operations..
		 *
		 */
		int openLeft ();
		int openRight ();

		int closeRight ();
		int closeLeft ();

		int stopMove ();

		time_t timeoutEnd;

		void setMotorTimeout (time_t timeout);

		/**
		 * Check if timeout was exceeded.
		 *
		 * @return 0 when timeout wasn't reached, 1 when timeout was
		 * exceeded.
		 */

		int checkMotorTimeout ();

		time_t lastClosing;
		int closingNum;
		int lastClosingNum;

		int openWDC ();
		void closeWDC ();

		int resetWDC ();
		int getWDCTimeOut ();
		int setWDCTimeOut (int on, double timeout);
		int getWDCTemp (int id);

		enum
		{
			MOVE_NONE, MOVE_OPEN_LEFT, MOVE_OPEN_LEFT_WAIT, MOVE_OPEN_RIGHT,
			MOVE_OPEN_RIGHT_WAIT,
			MOVE_CLOSE_RIGHT, MOVE_CLOSE_RIGHT_WAIT, MOVE_CLOSE_LEFT,
			MOVE_CLOSE_LEFT_WAIT,
			MOVE_RECLOSE_RIGHT_WAIT, MOVE_RECLOSE_LEFT_WAIT
		} movingState;

		int setValueSwitch (int sw, bool new_state);

	protected:
		virtual int processOption (int in_opt);

		virtual int init ();
		virtual int idle ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

	public:
		Fram (int argc, char **argv);
		virtual ~Fram (void);
		virtual int info ();
};

}

int Fram::zjisti_stav_portu_rep ()
{
	int ret;
	int timeout = 0;
	do
	{
		usleep (USEC_SEC / 1000);
		ret = zjisti_stav_portu ();
		// if first B port state are set, then it is a know error on the board
		if (ret == 0)
			break;
		timeout++;
		flushPort ();
		usleep (USEC_SEC / 2);
	}
	while (timeout < 10);
	return ret;
}

const char * Fram::isOnString (int c_port)
{
	return (getPortState (c_port)) ? "off" : "on ";
}

void Fram::setMotorTimeout (time_t timeout)
{
	time_t now;
	time (&now);
	timeoutEnd = now + timeout;
}

int Fram::checkMotorTimeout ()
{
	time_t now;
	time (&now);
	if (now >= timeoutEnd)
		//    logStream (MESSAGE_DEBUG) << "timeout reached: " << now -
		//      timeoutEnd << " state: " << movingState << sendLog;
		logStream (MESSAGE_DEBUG) << "timeout reached: " << now -
			timeoutEnd << " state: " << int (movingState) << sendLog;
	return (now >= timeoutEnd);
}

int Fram::openWDC ()
{
	int ret;
	wdcConn = new rts2core::ConnSerial (wdc_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 100);
	ret = wdcConn->init ();
	if (ret)
		return ret;

	return setWDCTimeOut (1, wdcTimeOut->getValueDouble ());
}

void Fram::closeWDC ()
{
	setWDCTimeOut (1, 120.0);
	delete wdcConn;
}

int Fram::resetWDC ()
{
	wdcConn->writePort ("~**\r", 4);
	wdcConn->flushPortO ();
	return 0;
}

int Fram::getWDCTimeOut ()
{
	int i, r, t;
	char q;
	char reply[128];

	wdcConn->writePort ("~012\r", 5); /*,i,q */;
	wdcConn->flushPortO ();

	q = i = t = 0;
	do
	{
		r = wdcConn->readPort (q);
		if (r == 1)
			reply[i++] = q;
		else
		{
			t++;
			if (t > 10)
				break;
		}
	}
	while (q > 20);

	reply[i] = 0;

	if (reply[0] == '!')
		logStream (MESSAGE_ERROR) << "Fram::getWDCTimeOut reply " << reply + 1 << sendLog;

	return 0;

}

int Fram::setWDCTimeOut (int on, double timeout)
{
	int i, r, t, timeo;
	char q;
	char reply[128];

	timeo = (int) (timeout / 0.03);
	if (timeo > 0xffff)
		timeo = 0xffff;
	if (timeo < 0)
		timeo = 0;
	if (on)
		i = '1';
	else
		i = '0';

	t = sprintf (reply, "~013%c%04X\r", i, timeo);
	wdcConn->writePort (reply, t);
	wdcConn->flushPortO ();

	q = i = t = 0;
	do
	{
		r = wdcConn->readPort (q);
		if (r == 1)
			reply[i++] = q;
		else
		{
			t++;
			if (t > 10)
				break;
		}
	}
	while (q > 20);

	logStream (MESSAGE_DEBUG) << "Fram::setWDCTimeOut on: " << on << " timeout: " << timeout <<
		" q: " << q << sendLog;

	reply[i] = 0;

	if (reply[0] == '!')
		logStream (MESSAGE_DEBUG) << "Fram::setWDCTimeOut reply: " <<
			reply + 1 << sendLog;

	return 0;
}

int Fram::getWDCTemp (int id)
{
	int i, r, t;
	char q;
	char reply[128];

	if ((id < 0) || (id > 2))
		return -1;

	t = sprintf (reply, "~017%X\r", id + 5);
	wdcConn->writePort (reply, t);
	wdcConn->flushPortO ();

	q = i = t = 0;
	do
	{
		r = wdcConn->readPort (q);
		if (r == 1)
			reply[i++] = q;
		else
		{
			t++;
			if (t > 10)
				return -1;
		}
	}
	while (q > 20);

	reply[i] = 0;

	if (reply[0] != '!')
		return -1;

	i = atoi (reply + 1);

	return i;
}

int Fram::openLeftMove ()
{
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "opening left door" << sendLog;
	ZAP (VENTIL_OTEVIRANI_LEVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}

int Fram::openRightMove ()
{
	VYP (VENTIL_AKTIVACNI);
	VYP (VENTIL_OTEVIRANI_LEVY);
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "opening right door" << sendLog;
	ZAP (VENTIL_OTEVIRANI_PRAVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}

int Fram::closeRightMove ()
{
	if (extraSwitch)
		extraSwitch->ZAP (SWITCH_BATBACK);
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "closing right door" << sendLog;
	ZAP (VENTIL_ZAVIRANI_PRAVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}

int Fram::closeLeftMove ()
{
	VYP (VENTIL_AKTIVACNI);
	VYP (VENTIL_ZAVIRANI_PRAVY);
	if (extraSwitch)
		extraSwitch->ZAP (SWITCH_BATBACK);
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "closing left door" << sendLog;
	ZAP (VENTIL_ZAVIRANI_LEVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}

int Fram::openLeft ()
{
	movingState = MOVE_OPEN_LEFT;
	openLeftMove ();
	movingState = MOVE_OPEN_LEFT_WAIT;
	setMotorTimeout (FRAM_TIME_OPEN_LEFT);
	return 0;
}

int Fram::openRight ()
{
	// otevri pravou strechu
	movingState = MOVE_OPEN_RIGHT;
	openRightMove ();
	movingState = MOVE_OPEN_RIGHT_WAIT;
	setMotorTimeout (FRAM_TIME_OPEN_RIGHT);
	return 0;
}

int Fram::closeRight ()
{
	closeRightMove ();
	movingState = MOVE_CLOSE_RIGHT_WAIT;
	setMotorTimeout (FRAM_TIME_CLOSE_RIGHT);
	return 0;
}

int Fram::closeLeft ()
{
	closeLeftMove ();
	movingState = MOVE_CLOSE_LEFT_WAIT;
	setMotorTimeout (FRAM_TIME_CLOSE_LEFT);
	return 0;
}

int Fram::stopMove ()
{
	switchOffPins (VENTIL_AKTIVACNI, KOMPRESOR);
	if (extraSwitch)
		extraSwitch->VYP (SWITCH_BATBACK);
	movingState = MOVE_NONE;
	return 0;
}

int Fram::startOpen ()
{
	if (movingState != MOVE_NONE)
		return -1;

	lastClosing = 0;
	closingNum = 0;
	lastClosingNum = -1;

	flushPort ();

	if (isOn (KONCAK_OTEVRENI_LEVY) && isOn (KONCAK_OTEVRENI_LEVY))
	{
		return endOpen ();
	}

	switchOffPins (VENTIL_OTEVIRANI_PRAVY, VENTIL_ZAVIRANI_PRAVY, VENTIL_ZAVIRANI_LEVY);

	if (!isOn (KONCAK_OTEVRENI_LEVY))
	{
		// otevri levou strechu
		openLeft ();
	}
	else						 // like !isOn (KONCAK_OTEVRENI_PRAVY - see && at begining
	{
		openRight ();
	}

	setTimeout (10);
	return 0;
}

long Fram::isOpened ()
{
	int flag = 0;
	logStream (MESSAGE_DEBUG) << "isOpened " << (int) movingState << sendLog;
	switch (movingState)
	{
		case MOVE_OPEN_LEFT_WAIT:
			// check for timeout..
			if (!(isOn (KONCAK_OTEVRENI_LEVY) || checkMotorTimeout ()))
				// go to end return..
				break;
			flag = 1;
			// follow on..
			__attribute__ ((fallthrough));
		case MOVE_OPEN_RIGHT:
			openRight ();
			break;
		case MOVE_OPEN_RIGHT_WAIT:
			if (!(isOn (KONCAK_OTEVRENI_PRAVY) || checkMotorTimeout ()))
				break;
			__attribute__ ((fallthrough));
		default:
			// if we are not opened..
			if (!(isOn (KONCAK_OTEVRENI_PRAVY) || isOn (KONCAK_OTEVRENI_LEVY)))
				return 0;
			return -2;
	}
	if (flag)
		infoAll ();
	return FRAM_CHECK_TIMEOUT;
}

int Fram::endOpen ()
{
	int ret;
	stopMove ();
	VYP (VENTIL_OTEVIRANI_PRAVY);
	VYP (VENTIL_OTEVIRANI_LEVY);
	ret = zjisti_stav_portu_rep ();	 //kdyz se to vynecha, neposle to posledni prikaz nebo znak
	if (!ret && isOn (KONCAK_OTEVRENI_PRAVY) && isOn (KONCAK_OTEVRENI_LEVY) &&
		!isOn (KONCAK_ZAVRENI_PRAVY) && !isOn (KONCAK_ZAVRENI_LEVY))
		return 0;
	// incorrect state -> error
	return -1;
}

int Fram::startClose ()
{
	time_t now;
	if (movingState == MOVE_CLOSE_RIGHT
		|| movingState == MOVE_CLOSE_RIGHT_WAIT
		|| movingState == MOVE_CLOSE_LEFT
		|| movingState == MOVE_CLOSE_LEFT_WAIT)
	{
		// closing already in progress
		return 0;
	}

	flushPort ();

	if (zjisti_stav_portu_rep () == -1)
	{
		logStream (MESSAGE_ERROR) << "Cannot read from port at close!" << sendLog;
		return -1;
	}
	if (movingState != MOVE_NONE)
	{
		stopMove ();
	}
	if (isOn (KONCAK_ZAVRENI_PRAVY) && isOn (KONCAK_ZAVRENI_LEVY))
		return endClose ();
	time (&now);
	if (lastClosing + FRAM_TIME_CLOSING_RETRY > now)
	{
		logStream (MESSAGE_WARNING) <<
			"closeDome ignored - closing timeout not reached" << sendLog;
		return -1;
	}

	switchOffPins (VENTIL_OTEVIRANI_PRAVY, VENTIL_ZAVIRANI_LEVY, VENTIL_OTEVIRANI_LEVY);

	if (closingNum > FRAM_MAX_CLOSING_RETRY)
	{
		logStream (MESSAGE_WARNING) <<
			"max closing retry reached, do not try closing again" << sendLog;
		return -1;
	}

	if (!isOn (KONCAK_ZAVRENI_PRAVY))
	{
		closeRight ();
	}
	else
	{
		closeLeft ();
	}
	closingNum++;
	reclosing_num->setValueInteger (0);
	return 0;
}

long Fram::isClosed ()
{
	int flag = 0;				 // send infoAll at end
	logStream (MESSAGE_DEBUG) << "isClosed " << (int) movingState << sendLog;
	switch (movingState)
	{
		case MOVE_CLOSE_RIGHT_WAIT:
			if (checkMotorTimeout ())
			{
				if (reclosing_num->getValueInteger () >= FRAM_RECLOSING_MAX)
				{
					// reclosing number exceeded, move to next door
					movingState = MOVE_CLOSE_LEFT;
					break;
				}
				setMotorTimeout (FRAM_TIME_RECLOSE_RIGHT);
				VYP (VENTIL_AKTIVACNI);
				VYP (VENTIL_ZAVIRANI_PRAVY);
				movingState = MOVE_RECLOSE_RIGHT_WAIT;
				openRightMove ();
				break;
			}
			if (!(isOn (KONCAK_ZAVRENI_PRAVY)))
				break;
			flag = 1;
			// close dome..
			__attribute__ ((fallthrough));
		case MOVE_CLOSE_LEFT:
			closeLeft ();
			break;
		case MOVE_CLOSE_LEFT_WAIT:
			if (checkMotorTimeout ())
			{
				if (reclosing_num->getValueInteger () >= FRAM_RECLOSING_MAX)
				{
					return -2;
				}
				// reclose left..
				setMotorTimeout (FRAM_TIME_RECLOSE_LEFT);
				VYP (VENTIL_AKTIVACNI);
				VYP (VENTIL_ZAVIRANI_LEVY);
				movingState = MOVE_RECLOSE_LEFT_WAIT;
				openLeftMove ();
				break;
			}
			if (!(isOn (KONCAK_ZAVRENI_LEVY)))
				break;
			return -2;
		case MOVE_RECLOSE_RIGHT_WAIT:
			if (!(isOn (KONCAK_OTEVRENI_PRAVY) || checkMotorTimeout ()))
				break;
			reclosing_num->inc ();
			VYP (VENTIL_AKTIVACNI);
			VYP (VENTIL_OTEVIRANI_PRAVY);
			closeRight ();
			break;
		case MOVE_RECLOSE_LEFT_WAIT:
			if (!(isOn (KONCAK_OTEVRENI_LEVY) || checkMotorTimeout ()))
				break;
			reclosing_num->inc ();
			VYP (VENTIL_AKTIVACNI);
			VYP (VENTIL_OTEVIRANI_LEVY);
			closeLeft ();
			break;
		default:
			if (isOn (KONCAK_ZAVRENI_PRAVY) && isOn (KONCAK_ZAVRENI_LEVY))
				return -2;
			return 0;
	}
	if (flag)
		infoAll ();
	return FRAM_CHECK_TIMEOUT;
}

int Fram::endClose ()
{
	int ret;
	stopMove ();
	VYP (VENTIL_ZAVIRANI_LEVY);
	VYP (VENTIL_ZAVIRANI_PRAVY);
	ret = zjisti_stav_portu_rep ();	 //kdyz se to vynecha, neposle to posledni prikaz nebo znak
	time (&lastClosing);
	if (closingNum != lastClosingNum)
	{
		lastClosingNum = closingNum;
		if (!ret && !isOn (KONCAK_OTEVRENI_PRAVY)
			&& !isOn (KONCAK_OTEVRENI_LEVY)
			&& isOn (KONCAK_ZAVRENI_PRAVY)
			&& isOn (KONCAK_ZAVRENI_LEVY))
			return 0;
		return -1;
	}
	return 0;
}

int Fram::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_WDC_DEV:
			wdc_file = optarg;
			break;
		case OPT_WDC_TIMEOUT:
			wdcTimeOut->setValueDouble (atof (optarg));
			break;
		case OPT_EXTRA_SWITCH:
			extraSwitchFile = optarg;
			break;
		default:
			return Ford::processOption (in_opt);
	}
	return 0;
}

int Fram::init ()
{
	int ret = Ford::init ();
	if (ret)
		return ret;

	if (extraSwitchFile)
	{
		extraSwitch = new rts2core::FordConn (extraSwitchFile, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
		ret = extraSwitch->init ();
		if (ret)
			return ret;

		extraSwitch->VYP (SWITCH_BATBACK);

		createValue (switchBatBack, "bat_backup", "state of batter backup switch", false, RTS2_VALUE_WRITABLE);
		switchBatBack->setValueBool (false);

		createValue (plug1, "plug_1", "1st plug", false, RTS2_VALUE_WRITABLE);
		createValue (plug_photometer, "plug_photometer", "1st plug", false, RTS2_VALUE_WRITABLE);
		extraSwitch->ZAP (PLUG_PHOTOMETER);
		plug_photometer->setValueBool (true);

		createValue (plug3, "plug_3", "3rd plug", false, RTS2_VALUE_WRITABLE);
		createValue (plug4, "plug_4", "4th plug", false, RTS2_VALUE_WRITABLE);
	}
	switchOffPins (VENTIL_AKTIVACNI, KOMPRESOR);

	movingState = MOVE_NONE;

	if (wdc_file)
	{
		ret = openWDC ();
		if (ret)
			return ret;
	}

	ret = zjisti_stav_portu_rep ();
	if (ret)
		return ret;
	// switch state
	if (isOn (KONCAK_OTEVRENI_PRAVY) && isOn (KONCAK_OTEVRENI_LEVY))
	{
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome opened");
	}
	else if (isOn (KONCAK_ZAVRENI_PRAVY) && isOn (KONCAK_ZAVRENI_LEVY))
	{
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome is closed");
	}
	else
	{
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome is opened");
		domeCloseStart ();
	}

	return 0;
}

int Fram::idle ()
{
	// resetWDC
	if (wdc_file)
		resetWDC ();
	return Ford::idle ();
}

int Fram::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == switchBatBack)
	{
		return setValueSwitch (SWITCH_BATBACK, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug_photometer)
	{
		return setValueSwitch (PLUG_PHOTOMETER, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug1)
	{
		return setValueSwitch (PLUG_1, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug3)
	{
		return setValueSwitch (PLUG_3, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	if (oldValue == plug4)
	{
		return setValueSwitch (PLUG_4, ((rts2core::ValueBool *) newValue)->getValueBool ());
	}
	return Ford::setValue (oldValue, newValue);
}

int Fram::setValueSwitch (int sw, bool new_state)
{
	if (new_state == true)
	{
		return extraSwitch->ZAP (sw) == 0 ? 0 : -2;
	}
	return extraSwitch->VYP (sw) == 0 ? 0 : -2;
}

Fram::Fram (int argc, char **argv):Ford (argc, argv)
{
	createValue (swOpenLeft, "sw_open_left", "state of left open switch", false);
	createValue (swCloseLeft, "sw_close_left", "state of left close switch", false);
	createValue (swCloseRight, "sw_close_right", "state of right close switch", false);
	createValue (swOpenRight, "sw_open_right", "state of right open switch", false);

	createValue (valveOpenLeft, "valve_open_left", "state of left opening valve", false);
	createValue (valveCloseLeft, "valve_close_left", "state of left closing valve", false);
	createValue (valveOpenRight, "valve_open_right", "state of right opening valve", false);
	createValue (valveCloseRight, "valve_close_right", "state of right closing valve", false);

	createValue (reclosing_num, "reclosing_num", "number of reclosing attempts", false);

	createValue (wdcTimeOut, "watchdog_timeout", "timeout of the watchdog card (in seconds)", false);
	wdcTimeOut->setValueDouble (30.0);

	createValue (wdcTemperature, "watchdog_temp1", "first temperature of the watchedog card", false);

	wdc_file = NULL;
	wdcConn = NULL;

	extraSwitchFile = NULL;
	extraSwitch = NULL;

	plug1 = NULL;
	plug_photometer = NULL;
	plug3 = NULL;
	plug4 = NULL;

	movingState = MOVE_NONE;

	lastClosing = 0;
	closingNum = 0;
	lastClosingNum = -1;

	addOption (OPT_WDC_DEV, "wdc-dev", 1, "/dev file with watch-dog card");
	addOption (OPT_WDC_TIMEOUT, "wdc-timeout", 1, "WDC timeout (default to 30 seconds)");
	addOption (OPT_EXTRA_SWITCH, "extra-switch", 1, "/dev entery for extra switches, handling baterry etc..");
}

Fram::~Fram (void)
{
	stopMove ();
	if (wdc_file)
		closeWDC ();
	stopMove ();
}

int Fram::info ()
{
	int ret;
	ret = zjisti_stav_portu_rep ();
	if (ret)
		return -1;
	swOpenRight->setValueBool (getPortState (KONCAK_OTEVRENI_PRAVY));
	swOpenLeft->setValueBool (getPortState (KONCAK_OTEVRENI_LEVY));
	swCloseRight->setValueBool(getPortState (KONCAK_ZAVRENI_PRAVY));
	swCloseLeft->setValueBool(getPortState (KONCAK_ZAVRENI_LEVY));
	if (wdcConn)
	{
	  	wdcTemperature->setValueDouble (getWDCTemp (2));
	}
	return Ford::info ();
}

int main (int argc, char **argv)
{
	Fram device = Fram (argc, argv);
	return device.run ();
}
