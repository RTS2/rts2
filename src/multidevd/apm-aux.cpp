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
APMAux::APMAux (const char *name, rts2core::ConnAPM *conn, bool hasFan, bool hasBaffle, bool hasRelays): Sensor (0, NULL, name), rts2multidev::APMMultidev (conn)
{
	fan = NULL;
	baffle = NULL;
	relay1 = NULL;
	relay2 = NULL;
	if (hasBaffle)
	{
		createValue (baffle, "baffle", "baffle cover state", true);
	}
	createValue (mirror, "mirror", "mirror cover state", true);
	if (hasFan)
	{
		createValue (fan, "fan", "fan state", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	}
	if (hasRelays)
	{
	        createValue(relay1, "relay1", "relay 1 state", true);
	        createValue(relay2, "relay2", "relay 2 state", true);
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

int APMAux::info ()
{
	sendUDPMessage ("C999");

	switch (mirror_state)
        {
                case MIRROR_OPENED:
                        mirror->setValueString("opened");
                        break;
                case MIRROR_CLOSED:
                        mirror->setValueString("closed");
                        break;
		case MIRROR_OPENING:
			mirror->setValueString("opening");
			break;
		case MIRROR_CLOSING:
			mirror->setValueString("closing");
			break;
	}

	if (relay1 != NULL || relay2 != NULL)
	{
        	sendUDPMessage ("A999");
	}

	return Sensor::info ();
}

int APMAux::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == fan)
	{
		if (((rts2core::ValueBool *) new_value)->getValueBool () == true)
			return sendUDPMessage ("CF01");
		else
			return sendUDPMessage ("CF00");
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
	sendUDPMessage ("C001");

	if (mirror_state == MIRROR_CLOSED)
		mirror_state = MIRROR_OPENING;

	info ();

	return 0;
}

int APMAux::close ()
{
	sendUDPMessage ("C000");

	if (mirror_state == MIRROR_OPENED)
		mirror_state = MIRROR_CLOSING;

	info ();

	return 0;
}

int APMAux::openBaffle ()
{
	return sendUDPMessage ("B001");
}

int APMAux::closeBaffle ()
{
	return sendUDPMessage ("B000");
}

int APMAux::relayControl (int n, bool on)
{
        char cmd[5];
        snprintf (cmd, 5, "A0%d%c", n, on ? '1': '0');
        sendUDPMessage (cmd);

        info ();

        return 0;
}

int APMAux::sendUDPMessage (const char * _message)
{
        char *response = (char *)malloc(20);

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = apmConn->sendReceive (_message, response, 20);

	if (n < 0)
	{
		logStream (MESSAGE_ERROR) << "no response" << sendLog;
		return -1;
	}

	response[n] = '\0';

        logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

        if (n > 0)
        {
                if (response[0] == 'C' && response[1] == '9')
                {
                        switch (response[3])
                        {
                                case '0':
					if (mirror_state != MIRROR_OPENING)
                                        	mirror_state = MIRROR_CLOSED;
                                        break;
                                case '1':
					if (mirror_state != MIRROR_CLOSING)
                                        	mirror_state = MIRROR_OPENED;
                                        break;
                        }
                }

                if (response[0] == 'A' && response[1] == '9')
                {
			int rv = response[3] - '0';
                        relay1->setValueBool (rv & 0x01);
                        relay2->setValueBool (rv & 0x02);
                }

        }

        return 0;
}
