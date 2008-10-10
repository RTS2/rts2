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

#include "ford.h"

using namespace rts2dome;

// check time in usec (1000000 ms = 1s)
#define FRAM_CHECK_TIMEOUT 1000

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
		Rts2ConnSerial *wdcConn;
		char *wdc_file;

		Rts2ValueInteger *sw_state;
		Rts2ValueDouble *wdcTimeOut;
		Rts2ValueDouble *wdcTemperature;

		int reclosing_num;

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

		bool sendContFailMail;

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

	protected:
		virtual int processOption (int in_opt);

		virtual int init ();
		virtual int idle ();

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

		int sendFramMail (const char *subject);
};

}


int
Fram::zjisti_stav_portu_rep ()
{
	int ret;
	int timeout = 0;
	do
	{
		usleep (USEC_SEC / 1000);
		ret = zjisti_stav_portu ();
		if (ret == 0)
			break;
		timeout++;
		flushPort ();
		usleep (USEC_SEC / 2);
	}
	while (timeout < 10);
	return ret;
}


const char *
Fram::isOnString (int c_port)
{
	return (getPortState (c_port)) ? "off" : "on ";
}


void
Fram::setMotorTimeout (time_t timeout)
{
	time_t now;
	time (&now);
	timeoutEnd = now + timeout;
}


int
Fram::checkMotorTimeout ()
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


int
Fram::openWDC ()
{
	int ret;
	wdcConn = new Rts2ConnSerial (wdc_file, this, BS9600, C8, NONE, 100);
	ret = wdcConn->init ();
	if (ret)
		return ret;

	return setWDCTimeOut (1, wdcTimeOut->getValueDouble ());
}


void
Fram::closeWDC ()
{
	setWDCTimeOut (1, 120.0);
	delete wdcConn;
}


int
Fram::resetWDC ()
{
	wdcConn->writePort ("~**\r", 4);
	wdcConn->flushPortO ();
	return 0;
}


int
Fram::getWDCTimeOut ()
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


int
Fram::setWDCTimeOut (int on, double timeout)
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


int
Fram::getWDCTemp (int id)
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


int
Fram::openLeftMove ()
{
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "opening left door" << sendLog;
	ZAP (VENTIL_OTEVIRANI_LEVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}


int
Fram::openRightMove ()
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


int
Fram::closeRightMove ()
{
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "closing right door" << sendLog;
	ZAP (VENTIL_ZAVIRANI_PRAVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}


int
Fram::closeLeftMove ()
{
	VYP (VENTIL_AKTIVACNI);
	VYP (VENTIL_ZAVIRANI_PRAVY);
	ZAP (KOMPRESOR);
	sleep (1);
	logStream (MESSAGE_DEBUG) << "closing left door" << sendLog;
	ZAP (VENTIL_ZAVIRANI_LEVY);
	ZAP (VENTIL_AKTIVACNI);
	return 0;
}


int
Fram::openLeft ()
{
	movingState = MOVE_OPEN_LEFT;
	openLeftMove ();
	movingState = MOVE_OPEN_LEFT_WAIT;
	setMotorTimeout (FRAM_TIME_OPEN_LEFT);
	return 0;
}


int
Fram::openRight ()
{
	// otevri pravou strechu
	movingState = MOVE_OPEN_RIGHT;
	openRightMove ();
	movingState = MOVE_OPEN_RIGHT_WAIT;
	setMotorTimeout (FRAM_TIME_OPEN_RIGHT);
	return 0;
}


int
Fram::closeRight ()
{
	closeRightMove ();
	movingState = MOVE_CLOSE_RIGHT_WAIT;
	setMotorTimeout (FRAM_TIME_CLOSE_RIGHT);
	return 0;
}


int
Fram::closeLeft ()
{
	closeLeftMove ();
	movingState = MOVE_CLOSE_LEFT_WAIT;
	setMotorTimeout (FRAM_TIME_CLOSE_LEFT);
	return 0;
}


int
Fram::stopMove ()
{
	switchOffPins (VENTIL_AKTIVACNI, KOMPRESOR);
	movingState = MOVE_NONE;
	return 0;
}


int
Fram::startOpen ()
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


long
Fram::isOpened ()
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
		case MOVE_OPEN_RIGHT:
			openRight ();
			break;
		case MOVE_OPEN_RIGHT_WAIT:
			if (!(isOn (KONCAK_OTEVRENI_PRAVY) || checkMotorTimeout ()))
				break;
		case MOVE_NONE:
			// if we are not opened..
			if (!(isOn (KONCAK_OTEVRENI_PRAVY) || isOn (KONCAK_OTEVRENI_LEVY)))
				return 0;
		default:
			return -2;
	}
	if (flag)
		infoAll ();
	return FRAM_CHECK_TIMEOUT;
}


int
Fram::endOpen ()
{
	int ret;
	stopMove ();
	VYP (VENTIL_OTEVIRANI_PRAVY);
	VYP (VENTIL_OTEVIRANI_LEVY);
	ret = zjisti_stav_portu_rep ();	 //kdyz se to vynecha, neposle to posledni prikaz nebo znak
	if (!ret && isOn (KONCAK_OTEVRENI_PRAVY) && isOn (KONCAK_OTEVRENI_LEVY) &&
		!isOn (KONCAK_ZAVRENI_PRAVY) && !isOn (KONCAK_ZAVRENI_LEVY))
	{
		sendFramMail ("FRAM dome opened");
	}
	else
	{
		sendFramMail ("WARNING FRAM dome opened with wrong end swithes status");
	}
	return 0;
}


int
Fram::startClose ()
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
		if (!sendContFailMail)
		{
			sendFramMail ("ERROR FRAM DOME CANNOT BE CLOSED DUE TO ROOF CONTROLLER FAILURE!!");
			sendContFailMail = true;
		}
		return -1;
	}
	sendContFailMail = false;
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
		sendFramMail ("FRAM WARNING - !!max closing retry reached!!");
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
	reclosing_num = 0;
	return 0;
}


long
Fram::isClosed ()
{
	int flag = 0;				 // send infoAll at end
	logStream (MESSAGE_DEBUG) << "isClosed " << (int) movingState << sendLog;
	switch (movingState)
	{
		case MOVE_CLOSE_RIGHT_WAIT:
			if (checkMotorTimeout ())
			{
				if (reclosing_num >= FRAM_RECLOSING_MAX)
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
		case MOVE_CLOSE_LEFT:
			closeLeft ();
			break;
		case MOVE_CLOSE_LEFT_WAIT:
			if (checkMotorTimeout ())
			{
				if (reclosing_num >= FRAM_RECLOSING_MAX)
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
			reclosing_num++;
			VYP (VENTIL_AKTIVACNI);
			VYP (VENTIL_OTEVIRANI_PRAVY);
			closeRight ();
			break;
		case MOVE_RECLOSE_LEFT_WAIT:
			if (!(isOn (KONCAK_OTEVRENI_LEVY) || checkMotorTimeout ()))
				break;
			reclosing_num++;
			VYP (VENTIL_AKTIVACNI);
			VYP (VENTIL_OTEVIRANI_LEVY);
			closeLeft ();
			break;
		case MOVE_NONE:
			if (!(isOn (KONCAK_ZAVRENI_PRAVY) || isOn (KONCAK_ZAVRENI_LEVY)))
				return 0;
		default:
			return -2;
	}
	if (flag)
		infoAll ();
	return FRAM_CHECK_TIMEOUT;
}


int
Fram::endClose ()
{
	int ret;
	stopMove ();
	VYP (VENTIL_ZAVIRANI_LEVY);
	VYP (VENTIL_ZAVIRANI_PRAVY);
	ret = zjisti_stav_portu_rep ();	 //kdyz se to vynecha, neposle to posledni prikaz nebo znak
	time (&lastClosing);
	if (closingNum != lastClosingNum)
	{
		if (!ret && !isOn (KONCAK_OTEVRENI_PRAVY)
			&& !isOn (KONCAK_OTEVRENI_LEVY)
			&& isOn (KONCAK_ZAVRENI_PRAVY)
			&& isOn (KONCAK_ZAVRENI_LEVY))
		{
			sendFramMail ("FRAM dome closed");
		}
		else
		{
			sendFramMail ("WARNING FRAM dome closed with wrong end switches state");
		}
		lastClosingNum = closingNum;
	}
	return 0;
}


int
Fram::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'w':
			wdc_file = optarg;
			break;
		case 't':
			wdcTimeOut->setValueDouble (atof (optarg));
			break;
		default:
			return Ford::processOption (in_opt);
	}
	return 0;
}


int
Fram::init ()
{
	int ret = Ford::init ();
	if (ret)
		return ret;

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
	else if (isOn (KONCAK_ZAVRENI_PRAVY) && isOn (KONCAK_ZAVRENI_PRAVY))
	{
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome is closed");
	}
	else
	{
		maskState (DOME_DOME_MASK, DOME_OPENED, "dome is opened");
		domeCloseStart ();
	}
		
	sendFramMail ("FRAM DOME restart");

	return 0;
}


int
Fram::idle ()
{
	// resetWDC
	if (wdc_file)
		resetWDC ();
	return Ford::idle ();
}


Fram::Fram (int argc, char **argv)
:Ford (argc, argv)
{
	createValue (sw_state, "sw_state", "state of the switches", false, RTS2_DT_HEX);
	createValue (wdcTimeOut, "watchdog_timeout", "timeout of the watchdog card (in seconds)", false);
	wdcTimeOut->setValueDouble (30.0);

	createValue (wdcTemperature, "watchdog_temp1", "first temperature of the watchedog card", false);

	wdc_file = NULL;
	wdcConn = NULL;

	movingState = MOVE_NONE;

	lastClosing = 0;
	closingNum = 0;
	lastClosingNum = -1;

	sendContFailMail = false;

	addOption ('w', "wdc_file", 1, "/dev file with watch-dog card");
	addOption ('t', "wdc_timeout", 1, "WDC timeout (default to 30 seconds");
}


Fram::~Fram (void)
{
	stopMove ();
	if (wdc_file)
		closeWDC ();
	stopMove ();
}


int
Fram::info ()
{
	int ret;
	ret = zjisti_stav_portu_rep ();
	if (ret)
		return -1;
	sw_state->setValueInteger (getPortState (KONCAK_OTEVRENI_PRAVY));
	sw_state->setValueInteger (sw_state->getValueInteger () | (getPortState (KONCAK_OTEVRENI_LEVY) << 1));
	sw_state->setValueInteger (sw_state->getValueInteger () | (getPortState (KONCAK_ZAVRENI_PRAVY) << 2));
	sw_state->setValueInteger (sw_state->getValueInteger () | (getPortState (KONCAK_ZAVRENI_LEVY) << 3));
	if (wdcConn > 0)
	{
	  	wdcTemperature->setValueDouble (getWDCTemp (2));
	}
	return Ford::info ();
}


int
Fram::sendFramMail (const char *subject)
{
	char *openText;
	int ret;
	ret = zjisti_stav_portu_rep ();
	asprintf (&openText, "%s.\n"
		"End switch status:\n"
		"CLOSE SWITCH RIGHT:%s CLOSE SWITCH LEFT:%s\n"
		" OPEN SWITCH RIGHT:%s  OPEN SWITCH LEFT:%s\n"
		"Weather::isGoodWeather %i\n"
		"port state: %i\n"
		"closingNum: %i lastClosing: %s",
		subject,
		isOnString (KONCAK_ZAVRENI_PRAVY),
		isOnString (KONCAK_ZAVRENI_LEVY),
		isOnString (KONCAK_OTEVRENI_PRAVY),
		isOnString (KONCAK_OTEVRENI_LEVY),
		isGoodWeather (),
		ret,
		closingNum,
		ctime (&lastClosing));
	ret = sendMail (subject, openText);
	free (openText);
	return ret;
}


int
main (int argc, char **argv)
{
	Fram device = Fram (argc, argv);
	return device.run ();
}
