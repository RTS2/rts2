/**
 * Driver for APM relays
 * Copyright (C) 2015 Stanislav Vitek
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

#include "apm-relay.h"
#include "connection/apm.h"

using namespace rts2sensord;

/**
 * Driver for APM relays
 *
 * @author Standa Vitek <standa@vitkovi.net>
 */

/**
 *  Constructor for APM multidev, taking connection from APMMultidev.
 */
APMRelay::APMRelay (const char *name, rts2core::ConnAPM *conn): Sensor (0, NULL, name), rts2multidev::APMMultidev (conn)
{
        createValue(relay1, "relay1", "Relay 1 state", true);
        createValue(relay2, "relay2", "Relay 2 state", true);
}

int APMRelay::commandAuthorized (rts2core::Connection * conn)
{
        if (conn->isCommand ("on"))
        {
                int n;
                if (conn->paramNextInteger (&n) || !conn->paramEnd ())
                        return -2;
                return relayOn (n);
        }

        if (conn->isCommand ("off"))
        {
                int n;
                if (conn->paramNextInteger (&n) || !conn->paramEnd ())
                        return -2;
                return relayOff (n);
        }

        return 0;
}

int APMRelay::info ()
{
        sendUDPMessage ("A999");

        switch (relay1_state)
        {
                case RELAY1_ON:
                        relay1->setValueString("on");
                        break;
                case RELAY1_OFF:
                        relay1->setValueString("off");
                        break;
        }

        switch (relay2_state)
        {
                case RELAY2_ON:
                        relay2->setValueString("on");
                        break;
                case RELAY2_OFF:
                        relay2->setValueString("off");
                        break;
        }

        sendValueAll(relay1);
        sendValueAll(relay2);

        return Sensor::info();
}

int APMRelay::relayOn (int n)
{
        char *cmd = (char *)malloc(4*sizeof (char));

        sprintf (cmd, "A0%d1", n);

        sendUDPMessage (cmd);

        info ();

        return 0;
}

int APMRelay::relayOff (int n)
{
        char *cmd = (char *)malloc(4*sizeof (char));

        sprintf (cmd, "A0%d0", n);

        sendUDPMessage (cmd);

        info ();

        return 0;
}

int APMRelay::sendUDPMessage (const char * _message)
{
        char *response = (char *)malloc(20*sizeof(char));

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = apmConn->sendReceive (_message, response, 20);
	response[n] = '\0';

        logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

	if (n > 0)
        {
                if (response[0] == 'A' && response[1] == '9')
                {
                        if (response[3] == '3')
                        {
                                relay1_state = RELAY1_ON;
                                relay2_state = RELAY2_ON;
                        }

                        if (response[3] == '2')
                        {
                                relay1_state = RELAY1_OFF;
                                relay2_state = RELAY2_ON;
                        }

                        if (response[3] == '1')
                        {
                                relay1_state = RELAY1_ON;
                                relay2_state = RELAY2_OFF;
                        }

			if (response[3] == '0')
                        {
                                relay1_state = RELAY1_OFF;
                                relay2_state = RELAY2_OFF;
                        }
                }
        }

        return 0;
}
