/**
 * Driver for TEFO focuser by Universal Scientific Technologies.
 * Copyright (C) 2021 Jan Strobl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "focusd.h"

#include "connection/udp.h"

#define MOTOR_STATE_IDLE	0
#define MOTOR_STATE_MOVING	1
#define MOTOR_STATE_CALIBRATING	2

#define LAST_MOVEMENT_RESULT_OK		0
#define LAST_MOVEMENT_RESULT_FAIL	1

#define UDP_RETRY_ATTEMPTS	5
#define UDP_RESPONSE_TIMEOUT	0.5


namespace rts2focusd
{

/**
 * Driver for TEFO focuser by Universal Scientific Technologies.
 * https://github.com/UniversalScientificTechnologies/TelescopeFocuser
 * Relates to the "responsive" version.
 *
 * @author Jan Strobl
 */
class TEFO:public Focusd
{
	public:
		TEFO (int argc, char **argv);
		virtual ~ TEFO (void);

	protected:
		virtual int isFocusing ();
		virtual bool isAtStartPosition ();

		virtual int processOption (int in_opt);
		virtual int initHardware ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
		virtual double tcOffset () { return 0.;};

	private:
		rts2core::ConnUDP *connTEFO;
		HostString *host;
		unsigned short messageID;
		unsigned short motorState;
		unsigned short last_movement_result;
		unsigned short position_actual;
		unsigned short position_set;
		float time_to_moveend;

		int sendCommand (const char *command);
};

};

using namespace rts2focusd;

TEFO::TEFO (int argc, char **argv):Focusd (argc, argv)
{
	host = NULL;

	setFocusExtent(1, 1000);	// It's hardcoded in the daemon's code at the moment

	addOption ('t', "tefo_server", 1, "IP and port (separated by :) of the TEFO daemon");
}

TEFO::~TEFO (void)
{
	delete connTEFO;
	delete host;
}

int TEFO::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 't':
			host = new HostString (optarg, "5000");
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

int TEFO::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify IP:port of the TEFO daemon (-t option)." << sendLog;
		return -1;
	}

	connTEFO = new rts2core::ConnUDP (host->getPort (), this, host->getHostname());
	connTEFO->setDebug (getDebug ());

	int ret = connTEFO->init();
	if (ret)
		return ret;

	return 0;
}

int TEFO::initValues ()
{
	focType = std::string ("TEFO");
	messageID = 0;
	return Focusd::initValues ();
}

int TEFO::sendCommand (const char *command)
{
	int ret = 0;
	int attempt = 0;
	char message[20], response[80];
	unsigned short response_ID;
	char response_command[10], request_status[10], motor_status[12];

	messageID++;
	sprintf (message, "%i %s", messageID, command);

	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "message: '" << message << "'" << sendLog;
	while (ret <= 0 && attempt < UDP_RETRY_ATTEMPTS)
	{
		attempt++;
		ret = connTEFO->sendReceive ((const char *)message, response, 80, 0, UDP_RESPONSE_TIMEOUT);
	}
	if (ret<1)
	{
		logStream (MESSAGE_ERROR) << "Didn't get response from focuser daemon..." << sendLog;
		return -1;
	}

	response[ret] = 0;
	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "response: '" << response << "'" << sendLog;
	// <ID_int16> <command> <request-status_string> <motor-status_string> <last-movement-result_int> <actual-ABS-position_int> <set-ABS-position_int> <time-to-moveend_float>
	ret = sscanf (response, "%hd %s %s %s %hd %hd %hd %f", &response_ID, response_command, request_status, motor_status, &last_movement_result, &position_actual, &position_set, &time_to_moveend);
	if (ret !=8)
	{
		logStream (MESSAGE_ERROR) << "Cannot parse reply from focuser, reply was: '" << response << "', sscanf returned " << ret << sendLog;
		return -1;
	}

	if (response_ID != messageID)
	{
		logStream (MESSAGE_ERROR) << "responseID is not the same as messageID?!?" << sendLog;
		return -1;
	}

	if (strcmp(request_status, "wrong") == 0)
	{
		logStream (MESSAGE_ERROR) << "request_status reported as wrong in daemon's response!" << sendLog;
		return -1;
	}

	if (strcmp(motor_status, "idle") == 0)
		motorState = MOTOR_STATE_IDLE;
	else if (strcmp(motor_status, "moving") == 0)
		motorState = MOTOR_STATE_MOVING;
	else
		motorState = MOTOR_STATE_CALIBRATING;

	return 0;
}

int TEFO::info ()
{
	int ret;

	ret = sendCommand ("S");
	if (ret)
		return -1;

	position->setValueInteger ((int) position_actual);

	return Focusd::info ();
}

int TEFO::setTo (double num)
{
	int ret;
	char command[10];

	ret = info ();
	if (ret)
		return ret;

	if (motorState == MOTOR_STATE_CALIBRATING)
	{
		logStream (MESSAGE_ERROR) << "focuser is not accepting a movement commands at the moment (calibration is in progress)" << sendLog;
		// OK, this situation can be solved by postponing the movement command internally, sending it later within the scope of isFocusing () function
		// however, it should be very unlikely to ever happen at all - so we can "solve" it this way...
		return -1;
	}

	if (motorState == MOTOR_STATE_IDLE && last_movement_result == LAST_MOVEMENT_RESULT_OK && position_actual != position_set)
	{
		// it seems like the TEFO daemon has been restarted, we have to recalibrate!
		sprintf (command, "CM%i", (int) num);
	}
	else
	{
		sprintf (command, "M%i", (int) num);
	}

	ret = sendCommand (command);

	return ret;
}

int TEFO::isFocusing ()
{
	int ret;

	ret = info ();
	if (ret)
		return ret;

	if (motorState == MOTOR_STATE_MOVING)
		return (int) (USEC_SEC * time_to_moveend);
	if (motorState == MOTOR_STATE_CALIBRATING)
		return (int) (USEC_SEC / 10);

	// the only left case is motorState == MOTOR_STATE_IDLE
	if (last_movement_result == LAST_MOVEMENT_RESULT_FAIL || position_actual != position_set)
		return -1;	// error

	return -2;	// OK, we are in place
}

bool TEFO::isAtStartPosition ()
{
	return false;
}

int main (int argc, char **argv)
{
	TEFO device (argc, argv);
	return device.run ();
}
