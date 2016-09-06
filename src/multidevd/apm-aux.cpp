/**
 * Driver for APM auxiliary devices (mirror covers, fans,..)
 * Copyright (C) 2015 Stanislav Vitek
 * Copyright (C) 2016 Petr Kubanek
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

#include "apm-aux.h"
#include "connection/apm.h"

#define COVER_TIME      35
#define BAFFLE_TIME     30

#define COMMAND_TIMER   1550

using namespace rts2sensord;

/**
 * Driver for APM mirror cover
 *
 * @author Standa Vitek <standa@vitkovi.net>
 */

/*
 *  Constructor for APM multidev, taking connection from filter.
 *  Arguments argc and argv has to be set to 0 and NULL respectively.
 *  See apm-multidev.cpp for example.
 */
APMAux::APMAux (const char *name, rts2core::ConnAPM *conn, bool hasFan, bool hasBaffle, bool hasRelays, bool hasTemp): Sensor (0, NULL, name), rts2multidev::APMMultidev (conn)
{
	fan = NULL;
	baffle = NULL;
	relay1 = NULL;
	relay2 = NULL;
	temperature = NULL;

	coverCommand = NULL;
	baffleCommand = NULL;

	commandInProgress = NONE;

	createValue (coverState, "cover", "mirror cover state", true);
	coverState->addSelVal ("CLOSED");
	coverState->addSelVal ("OPENING");
	coverState->addSelVal ("OPENED");
	coverState->addSelVal ("CLOSING");

	createValue (coverCommand, "cmd_cover", "when last cover command is expected to complete", false);

	if (hasBaffle)
	{
		createValue (baffle, "baffle", "baffle cover state", true);
		baffle->addSelVal ("CLOSED");
		baffle->addSelVal ("OPENING");
		baffle->addSelVal ("OPENED");
		baffle->addSelVal ("CLOSING");

		createValue (baffleCommand, "cmd_baffle", "when last baffle command is expected to complete", false);
	}
	if (hasFan)
	{
		createValue (fan, "fan", "fan state", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	}
	if (hasRelays)
	{
	        createValue (relay1, "relay1", "relay 1 state", true);
	        createValue (relay2, "relay2", "relay 2 state", true);
	}
	if (hasTemp)
	{
		createValue (temperature, "temperature", "telescope temperature", true);
	}
}

int APMAux::initHardware ()
{
	return 0;
}

int APMAux::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("open"))
		return open ();
	else if (conn->isCommand ("close"))
		return close ();
	else if (conn->isCommand ("open_cover"))
		return openCover ();
	else if (conn->isCommand ("close_cover"))
		return closeCover ();
	else if (baffle != NULL && conn->isCommand ("open_baffle"))
		return openBaffle ();
	else if (baffle != NULL && conn->isCommand ("close_baffle"))
		return closeBaffle ();
	return Sensor::commandAuthorized (conn);
}

void APMAux::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			{
				switch (new_state & SERVERD_ONOFF_MASK)
				{
					case SERVERD_ON:
						open ();
						break;
					default:
						close();
						break;
				}
			}
			break;
		default:
			close();
			break;
	}
	Sensor::changeMasterState (old_state, new_state);
}

void APMAux::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case COMMAND_TIMER:
			info ();
			break;
	}
	Sensor::postEvent (event);
}

int APMAux::info ()
{
       	sendUDPMessage ("C999");

	switch (commandInProgress)
	{
		case OPENING:
			if (baffle && baffle->getValueInteger () == 2)
			{
				openCover ();
				commandInProgress = NONE;
			}
			break;
		case CLOSING:
			if (coverState && coverState->getValueInteger () == 0)
			{
				if (baffleCommand != NULL)
					closeBaffle ();
				commandInProgress = NONE;
			}
			break;
		case NONE:
			break;
	}

	if (relay1 != NULL || relay2 != NULL)
	{
        	sendUDPMessage ("A999");
	}

	if (temperature != NULL)
	{
		sendUDPMessage ("T000", true);
	}

	return Sensor::info ();
}

int APMAux::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == fan)
	{
		if (((rts2core::ValueBool *) new_value)->getValueBool () == true)
			return sendUDPMessage ("CF01") ? -2 : 0;
		else
			return sendUDPMessage ("CF00") ? -2 : 0;
	}
	else if (old_value == relay1)
	{
		return relayControl (0, ((rts2core::ValueBool *) new_value)->getValueBool ());

	}
	else if (old_value == relay2)
	{
		return relayControl (1, ((rts2core::ValueBool *) new_value)->getValueBool ());

	}
	return Sensor::setValue (old_value, new_value);
}

int APMAux::open ()
{
	if (baffle == NULL || baffle->getValueInteger () == 2)
		return openCover ();
	if (baffleCommand->getValueInteger () > time (NULL))
		return -2;
	commandInProgress = OPENING;
	addTimer (BAFFLE_TIME + 1, new rts2core::Event (COMMAND_TIMER));
	return openBaffle ();
}

int APMAux::close ()
{
	if (baffle == NULL || baffle->getValueInteger () == 0 || coverState->getValueInteger () == 2)
	{
		commandInProgress = CLOSING;
		addTimer (COVER_TIME + 1, new rts2core::Event (COMMAND_TIMER));
		return closeCover ();
	}
	if (baffleCommand->getValueInteger () > time (NULL))
		return -2;
	return closeBaffle ();
}

int APMAux::openCover ()
{
	sendUDPMessage ("C001");
	if (coverState->getValueInteger () == 0)
	{
		coverState->setValueInteger (1);
		sendValueAll (coverState);
	}

	coverCommand->setValueTime (time (NULL) + COVER_TIME);
	sendValueAll (coverCommand);

	return 0;
}

int APMAux::closeCover ()
{
	sendUDPMessage ("C000");
	if (coverState->getValueInteger () == 2)
	{
		coverState->setValueInteger (3);
		sendValueAll (coverState);
	}

	coverCommand->setValueTime (time (NULL) + COVER_TIME);
	sendValueAll (coverCommand);

	return 0;
}

int APMAux::openBaffle ()
{
	if (sendUDPMessage ("B001"))
		return -1;

	if (baffle->getValueInteger () == 0)
	{
		baffle->setValueInteger (1);
		sendValueAll (baffle);
	}

	baffleCommand->setValueTime (time (NULL) + BAFFLE_TIME);
	sendValueAll (baffleCommand);

	return 0;
}

int APMAux::closeBaffle ()
{
	if (sendUDPMessage ("B000"))
		return -1;

	if (baffle != NULL && baffle->getValueInteger () == 2)
	{
		baffle->setValueInteger (3);
		sendValueAll (baffle);
		baffleCommand->setValueTime (time (NULL) + BAFFLE_TIME);
		sendValueAll (baffleCommand);
	}

	return 0;
}

int APMAux::relayControl (int n, bool on)
{
        char cmd[5];
        snprintf (cmd, 5, "A0%d%c", n, on ? '1': '0');
        sendUDPMessage (cmd);

        info ();

        return 0;
}

int APMAux::sendUDPMessage (const char * _message, bool expectSecond)
{
	char *response = (char *)malloc(20);

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

	int n = apmConn->sendReceive (_message, response, 20);

	if (n <= 0)
	{
		logStream (MESSAGE_ERROR) << "no response" << sendLog;
		usleep (200);
		return -1;
	}

	response[n] = '\0';
	logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

	// temperature commands needs second read
	if (expectSecond)
	{
		n = apmConn->receiveMessage (response, 20, 10);
		if (n <= 0)
		{
			logStream (MESSAGE_ERROR) << "no second response" << sendLog;
			usleep (200);
			return -1;
		}
		response[n] = '\0';
		logStream (MESSAGE_DEBUG) << "response 2: " << response << sendLog;
	}

	if (baffleCommand != NULL && !isnan (baffleCommand->getValueDouble ()) && baffleCommand->getValueInteger () < time (NULL))
	{
		if (baffle->getValueInteger () == 1)
			baffle->setValueInteger (2);
		if (baffle->getValueInteger () == 3)
			baffle->setValueInteger (0);
		baffleCommand->setValueDouble (NAN);
		sendValueAll (baffle);
		sendValueAll (baffleCommand);
	}

	if (response[0] == 'C' && response[1] == '9')
	{
		switch (response[3])
		{
			case '0':
				if (!isnan (coverCommand->getValueDouble ()))
				{
					if (coverCommand->getValueInteger () > time (NULL))
						break;
					coverCommand->setValueDouble (NAN);
					sendValueAll (coverCommand);
				}
				coverState->setValueInteger (0);
				sendValueAll (coverState);
				break;
			case '1':
				if (!isnan (coverCommand->getValueDouble ()))
				{
					if (coverCommand->getValueInteger () > time (NULL))
						break;
					coverCommand->setValueDouble (NAN);
					sendValueAll (coverCommand);
				}
				coverState->setValueInteger (2);
				sendValueAll (coverState);
				break;
			default:
				logStream (MESSAGE_ERROR) << "invalid cover state:" << response[3] << sendLog;
				return -1;
		}
	}

	if (response[0] == 'A' && response[1] == '9')
	{
		if (response[3] < '0' || response[3] > '3')
		{
			logStream (MESSAGE_ERROR) << "invalid relay state " << response[3] << sendLog;
			usleep (200);
			return -1;
		}
		int rv = response[3] - '0';
		relay1->setValueBool (rv & 0x01);
		relay2->setValueBool (rv & 0x02);
	}

	if (response[0] == 'T')
	{
		temperature->setValueFloat (atof (response + 1));
	}

	usleep (200);

        return 0;
}
