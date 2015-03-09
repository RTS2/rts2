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

#include "sensord.h"

#include "connection/apm.h"

#define	RELAY1_OFF	1
#define RELAY1_ON	2
#define	RELAY2_OFF	3
#define RELAY2_ON	4
#define RELAYS_OFF	5
#define RELAYS_ON	6

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
		APMRelay (int argc, char **argv);
		virtual int initHardware ();
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual int info ();
	protected:
		virtual int processOption (int in_opt);
	private:
		int relays_state;
		HostString *host;
		rts2core::ConnAPM *connRelay;
		rts2core::ValueString *relay1, *relay2;
		int relayOn (int n);
		int relayOff (int n);
		int sendUDPMessage (const char * _message);
};

}

using namespace rts2sensord;

int APMRelay::processOption (int opt)
{
	switch (opt)
	{
		case 'e':
			host = new HostString (optarg, "1000");
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int APMRelay::initHardware ()
{
	if (host == NULL)	        
	{
		logStream (MESSAGE_ERROR) << "You must specify dome hostname (with -e option)." << sendLog;
		return -1;	
	}

	connRelay = new rts2core::ConnAPM (this, host->getHostname(), host->getPort());
	int ret = connRelay->init();
	if (ret)
		return ret;

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
		case RELAYS_ON:
			relay1->setValueString("on");
			relay2->setValueString("on");
		case RELAY1_ON:
			relay1->setValueString("on");
			break;
		case RELAY1_OFF:
			relay1->setValueString("off");
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

	return info();
}

int APMRelay::relayOff (int n)
{
	char *cmd = (char *)malloc(4*sizeof (char));

	sprintf (cmd, "A0%d0", n);

	sendUDPMessage (cmd);
	
	return info();
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
			if (response[2] == '3')
			{
				relays_state = (response[3] == '0') ? RELAYS_OFF : RELAYS_ON;
			}
		
			if (response[2] == '2')
			{
				relays_state = (response[3] == '0') ? RELAY2_OFF : RELAY2_ON;
			}

			if (response[3] == '1')
			{
				relays_state = (response[3] == '0') ? RELAY1_OFF : RELAY1_ON;
			}
		}
	}

	return 0;
}

APMRelay::APMRelay (int argc, char **argv): Sensor (argc, argv)
{
	host = NULL;

        addOption ('e', NULL, 1, "APM relay IP and port (separated by :)");
	createValue(relay1, "relay1", "Relay 1 state", true);
	createValue(relay2, "relay2", "Relay 2 state", true);
}

int main (int argc, char **argv)
{
	APMRelay device (argc, argv);
	return device.run ();
}
