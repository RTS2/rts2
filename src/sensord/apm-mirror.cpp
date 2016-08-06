/**
 * Driver for APM mirror cover
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

#include "apm-mirror.h"
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
APMMirror::APMMirror (const char *name, rts2core::ConnAPM *conn): Sensor (0, NULL, name), rts2multidev::APMMultidev (conn)
{
	createValue (mirror, "mirror", "Mirror cover state", true);
}

/*int APMMirror::initHardware ()
{
	sendUDPMessage ("C999");

	return 0;
}*/

int APMMirror::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("open"))
	{
		return open();
	}
	
	if (conn->isCommand ("close"))
	{
		return close();
	}

	return 0;
}

void APMMirror::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
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

int APMMirror::info ()
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

	sendValueAll(mirror);

	return Sensor::info ();
}

int APMMirror::open ()
{
	sendUDPMessage ("C001");

	if (mirror_state == MIRROR_CLOSED)
		mirror_state = MIRROR_OPENING;

	info ();

	return 0;
}

int APMMirror::close ()
{
	sendUDPMessage ("C000");

	if (mirror_state == MIRROR_OPENED)
		mirror_state = MIRROR_CLOSING;

	info ();

	return 0;
}	

int APMMirror::sendUDPMessage (const char * _message)
{
        char *response = (char *)malloc(20);

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = apmConn->sendReceive (_message, response, 20);

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
        }

        return 0;
}
