/**
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

#include "multidev.h"
#include "sensord.h"
#include "apm-filter.h"
#include "connection/apm.h"

#define	RELAYS_OFF	1
#define RELAY1_ON	2
#define	RELAY2_ON	3
#define RELAYS_ON	4

#define MIRROR_OPENED	1
#define MIRROR_CLOSED	2

namespace rts2sensord
{

/**
 * APM relays driver.
 *     
 * @author Stanislav Vitek <standa@vitkovi.net>
 */
class APMRelay : public Sensor
{
	public:
		APMRelay (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter);
		virtual int initHardware ();
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual int info ();
	protected:
		virtual int processOption (int in_opt);
	private:
		int relays_state;
		rts2core::ConnAPM *connRelay;
		rts2filterd::APMFilter *filter;
		rts2core::ValueString *relay1, *relay2;
		int relayOn (int n);
		int relayOff (int n);
		int sendUDPMessage (const char * _message);
};

/**
 * APM mirror cover driver.
 *
 * @author Stanislav Vitek <standa@vitkovi.net>
 */
class APMMirror : public Sensor
{
	public:
		APMMirror (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter);
		virtual int initHardware ();
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual int info ();
	protected:
		virtual int processOption (int in_opt);
	private:
		int mirror_state;
                rts2core::ConnAPM *connMirror;
		rts2filterd::APMFilter *filter;
                rts2core::ValueString *mirror;
                int open ();
                int close ();
                int sendUDPMessage (const char * _message);
};

}

using namespace rts2sensord;

int APMRelay::initHardware ()
{
	connRelay = filter->connFilter;

	sendUDPMessage ("A999");

	return 0;
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

	switch (relays_state)
	{
		case RELAYS_OFF:
			relay1->setValueString("off");
			relay2->setValueString("off");
			break;
		case RELAY1_ON:
			relay1->setValueString("on");
			relay2->setValueString("off");
			break;
		case RELAY2_ON:
			relay1->setValueString("off");
			relay2->setValueString("on");
			break;
		case RELAYS_ON:
			relay1->setValueString("on");
			relay2->setValueString("on");
			break;
	}

	sendValueAll(relay1);
	sendValueAll(relay2);

	return Sensor::info();
}

int APMRelay::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'e':
			return 0;
		default:
			return Sensor::processOption (in_opt);
	}
}

int APMRelay::relayOn (int n)
{
	char *cmd = (char *)malloc(4*sizeof (char));

	sprintf (cmd, "A00%d", n);

	sendUDPMessage (cmd);
	sendUDPMessage ("A999");

	return 0;
}

int APMRelay::relayOff (int n)
{
	// current version of firmware doens't allow separate switch off
	sendUDPMessage ("A000");
	sendUDPMessage ("A999");
	return 0;
}

int APMRelay::sendUDPMessage (const char * _message)
{
	char *response = (char *)malloc(20*sizeof(char));

	logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

	int n = connRelay->send (_message, response, 20);

	logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

	if (n > 0)
	{
		if (response[0] == 'A' && response[1] == '9')
		{
			switch (response[3])
			{
				case '0':
					relays_state = RELAYS_OFF;
					break;
				case '1':
					relays_state = RELAY1_ON;
					break;
				case '2':
					relays_state = RELAY2_ON;
					break;
				case '3':
					relays_state = RELAYS_OFF;
					break;
			}
		}
	}

	return 0;
}

APMRelay::APMRelay (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter): Sensor (argc, argv, sn)
{
	addOption ('e', NULL, 1, "IP and port (separated by :)");

	createValue(relay1, "relay1", "Relay 1 state", true);
	createValue(relay2, "relay2", "Relay 2 state", true);

	filter = in_filter;
}

/*------------------------------------------------------------------------------------------*/

int APMMirror::initHardware ()
{
	connMirror = filter->connFilter;

	return info();
}

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
	}

	sendValueAll(mirror);

	return Sensor::info ();
}

int APMMirror::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'e':
			return 0;
		default:
			return Sensor::processOption (in_opt);
	}
}

int APMMirror::open ()
{
	sendUDPMessage ("C001");

	return info ();
}

int APMMirror::close ()
{
	sendUDPMessage ("C000");

	return info ();
}	

int APMMirror::sendUDPMessage (const char * _message)
{
        char *response = (char *)malloc(20*sizeof(char));

        logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

        int n = connMirror->send (_message, response, 20);

        logStream (MESSAGE_DEBUG) << "response: " << response << sendLog;

        if (n > 0)
        {
                if (response[0] == 'C' && response[1] == '9')
                {
                        switch (response[3])
                        {
                                case '0':
                                        mirror_state = MIRROR_OPENED;
                                        break;
                                case '1':
                                        mirror_state = MIRROR_CLOSED;
                                        break;
                        }
                }
        }

        return 0;
}

APMMirror::APMMirror (int argc, char **argv, const char *sn, rts2filterd::APMFilter *in_filter): Sensor (argc, argv, sn)
{
	addOption ('e', NULL, 1, "IP and port (separated by :)");

	createValue (mirror, "mirror", "Mirror cover state", true);

	filter = in_filter;
}

int main (int argc, char **argv)
{
	rts2core::MultiDev md;
	rts2filterd::APMFilter *filter = new rts2filterd::APMFilter (argc, argv);
	md.push_back (filter);
	md.push_back (new APMRelay (argc, argv, "M1", filter));
	md.push_back (new APMMirror (argc, argv, "R1", filter));
	return md.run ();
}
