/**
 * APM dome driver (ESA TestBed telescope)
 * Copyright (C) 2014 Standa Vitek <standa@vitkovi.net>
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

#include "dome.h"

#include "connection/apm.h"

#define APMDOME_OPENING		1
#define APMDOME_CLOSING		2
#define APMDOME_OPENED		3
#define APMDOME_CLOSED		4
#define APMDOME_STOPPED		5
#define APMDOME_INIT		6

#define EVENT_APMDOME_DELAY_OPEN	RTS2_LOCAL_EVENT + 667
#define EVENT_APMDOME_DELAY_CLOSE	RTS2_LOCAL_EVENT + 668

using namespace rts2dome;

namespace rts2dome
{

/**
 * Class for APM dome driver.
 *
 * @author Standa Vítek <standa@vitkovi.net>
 */
class APMDome:public Dome
{
	public:
		APMDome (int argc, char **argv);
		virtual int initHardware ();
		virtual int commandAuthorized (rts2core::Connection *conn);
		virtual void postEvent (rts2core::Event *event);
                virtual int info ();

                virtual int startOpen ();
                virtual long isOpened ();
                virtual int endOpen ();
                virtual int startClose ();
                virtual long isClosed ();
                virtual int endClose ();

	protected:
		virtual int processOption (int in_opt);
		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		rts2core::ValueInteger * sw_state;
		rts2core::ValueString * domeStatus;
		rts2core::ValueString * sideApos, * sideBpos;
		rts2core::ValueSelection * closeDome;
		rts2core::ConnAPM *connDome;
		int delay;
		int dome_state;

		HostString *host;

		int sock;
                struct sockaddr_in servaddr, clientaddr;

		bool isMoving ();
		int sendUDPMessage (const char * in_message);
};

}

APMDome::APMDome (int argc, char **argv):Dome (argc, argv)
{
	host = NULL;
	delay = 0;

	addOption ('e', NULL, 1, "ESA dome IP and port (separated by :)");
	addOption ('t', NULL, 10, "Delay betwen dome sides during open/close");
	createValue(domeStatus, "dome_status", "current dome status", true);
	createValue(sideApos, "sideA_pos", "last known side A position", true);
	createValue(sideBpos, "sideB_pos", "last known side B position", true);
}

int APMDome::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("stop"))
	{
		sendUDPMessage ("D666");
		return 0;	
	}
	else if (conn->isCommand ("sideA"))
	{
		char *val;
		if (conn->paramNextString(&val) || !conn->paramEnd ())
			return -2;
		
		if (strcmp (val, "open"))
		{
			//
		}
		else if (strcmp (val, "close"))
		{
			//
		}
		return 0;
	}
	else if (conn->isCommand ("sideB"))
	{
		char *val;
		if (conn->paramNextString(&val) || !conn->paramEnd ())
			return -2;

		if (strcmp (val, "open"))
		{
			//
		}
		else if (strcmp (val, "close"));
		{
			//
		}
		return 0;
	}	
	return Dome::commandAuthorized (conn);
}

int APMDome::processOption (int in_opt)
{
	switch (in_opt)
        {
                case 'e':
			host = new HostString (optarg, "1000");
                        break;
		case 't':
			delay = atoi (optarg);
			break;
                default:
                        return Dome::processOption (in_opt);
        }
        return 0;
}

int APMDome::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify dome hostname (with -e option)." << sendLog;
		return -1;
	}

	connDome = new rts2core::ConnAPM (this, host->getHostname(), host->getPort());
	
	int ret = connDome->init();
        
	if (ret)
		return ret;

	if (!isMoving ())
        {
                // initial dome close
		sendUDPMessage ("D777");

                maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome after init");
        }

	return 0;
}

void APMDome::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_APMDOME_DELAY_OPEN:
			sendUDPMessage ("DB100");
			return;
		case EVENT_APMDOME_DELAY_CLOSE:
			sendUDPMessage ("DA000");
			return;
	}
	Dome::postEvent (event);
}

int APMDome::setValue(rts2core::Value *old_value, rts2core::Value *new_value)
{
    
	if (old_value == closeDome)
	{
		if(new_value->getValueInteger() == 1)
		{
			startOpen();	
		}
		else if(new_value->getValueInteger() == 0)
		{
			startClose();	
		}	
	}
	
	return Dome::setValue(old_value, new_value);
}

int APMDome::info ()
{
	sendUDPMessage ("D999");

	switch(dome_state)
	{
		case APMDOME_CLOSING:
			domeStatus->setValueString("Closing");
			break;
		case APMDOME_OPENING:
			domeStatus->setValueString("Openning");
			break;
		case APMDOME_CLOSED:
			domeStatus->setValueString("Closed");
			break;
		case APMDOME_OPENED:
			domeStatus->setValueString("Opened");
			break;
		case APMDOME_STOPPED:
			domeStatus->setValueString("Stopped");
			break;
		case APMDOME_INIT:
			domeStatus->setValueString("Closing (init)");
			break;
		default:
			domeStatus->setValueString("Unknown...");
	}
	
	sendValueAll(domeStatus);

	return Dome::info ();
}

int APMDome::startOpen ()
{
	if (isMoving () || dome_state == APMDOME_OPENED)                
		return 0;

	if (delay == 0)
	{
		sendUDPMessage ("D100");
	}
	else
	{
		sendUDPMessage ("DA100");
		addTimer (delay, new rts2core::Event (EVENT_APMDOME_DELAY_OPEN));
	}
	domeStatus->setValueString("Openning");
	sendValueAll(domeStatus);
	
	return 0;
}

long APMDome::isOpened ()
{
	// return (getState () & DOME_DOME_MASK) == DOME_CLOSED ? 0 : -2;
	if (isMoving () || dome_state == APMDOME_CLOSED)
		return USEC_SEC;

	if (dome_state == APMDOME_OPENED)
		return -2;

	return USEC_SEC;
}
                
int APMDome::endOpen ()
{
	return 0;
}
                
int APMDome::startClose ()
{
	if (isMoving () || dome_state == APMDOME_CLOSED)
		return 0;

	sendUDPMessage ("D000");
	domeStatus->setValueString("Closing");
	sendValueAll(domeStatus);
	return 0;
}
                
long APMDome::isClosed ()
{
	// return (getState () & DOME_DOME_MASK) == DOME_OPENED ? 0 : -2;
	if (isMoving () || dome_state == APMDOME_OPENED)
                return USEC_SEC;

        if (dome_state == APMDOME_CLOSED)
                return -2;

        return USEC_SEC;
}
                
int APMDome::endClose ()
{
	return 0;
}

bool APMDome::isMoving ()
{
	sendUDPMessage ("D999");

	if (dome_state == APMDOME_CLOSING || dome_state == APMDOME_OPENING)
		return true;

	return false;
}

int APMDome::sendUDPMessage (const char * _message)
{
	char *response = (char *)malloc (20*sizeof (char));
	char sA, sB, sideA[4], sideB[4];
	
	if (getDebug())
		logStream (MESSAGE_DEBUG) << "command to controller: " << _message << sendLog;

	int n = connDome->send (_message, response, 20, false);

        if (getDebug())
	{
		std::string res (response);
		logStream (MESSAGE_DEBUG) << "reponse from controller: " << res.substr (0, n) << sendLog;
	}

	if (n > 4)
	{
		if (response[0] == 'E') 
		{
			// echo from controller "Echo: [cmd]"
			return 10;
		}
		else
		{
			sA = response[2];
			sB = response[6];

			// side A position
			strncpy (sideA, response + 3, 3);
			sideA[3] = '\0';

			strncpy (sideB, response + 7, 3);
                        sideB[3] = '\0';


			sideApos->setValueString(sideA);
			sendValueAll(sideApos);
			sideBpos->setValueString(sideB);
			sendValueAll(sideBpos);

			logStream (MESSAGE_DEBUG) << "#1: " << sA << " #2:" << sideA << " #3: " << sB << " #4: " << sideB << sendLog;

			if (sA == '1' || sB == '1')
			{
				// moving to open
				dome_state = APMDOME_OPENING;
				return 0;
			} 

			if (sA == '2' || sB == '2')
			{
				// moving to close
				dome_state = APMDOME_CLOSING; 
			}
		
			if (sA == '3' || sB == '3')
			{
				// emergency stop
				dome_state = APMDOME_STOPPED;
			}

			if (sA == '4' || sB == '4')
			{
				// initial closing
				dome_state = APMDOME_INIT;
			}

			if ((strcmp (sideA, "100") == 0) && (strcmp (sideB, "100") == 0))
			{
				// dome fully opened
				dome_state = APMDOME_OPENED;				
			}

			if ((strcmp (sideA, "000") == 0) && (strcmp (sideB, "000") == 0))
                        {
                                // dome fully closed
                                dome_state = APMDOME_CLOSED;
                        }

			return 0;
		}
	}

	return -1;
}

int main (int argc, char **argv)
{
	APMDome device (argc, argv);
	return device.run ();
}
