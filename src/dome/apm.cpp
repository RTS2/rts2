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

#define EVENT_APMDOME_DELAY_INIT	RTS2_LOCAL_EVENT + 667

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
                virtual int info ();
		virtual void postEvent (rts2core::Event *event);
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

	addOption ('e', NULL, 1, "ESA dome IP and port (separated by :)");
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
	else if (conn->isCommand ("init"))
	{
		sendUDPMessage ("D777");
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
                default:
                        return Dome::processOption (in_opt);
        }
        return 0;
}

void APMDome::postEvent (rts2core::Event *event)
{
	if (event->getType () == EVENT_APMDOME_DELAY_INIT)
	{
		sendUDPMessage ("D777");
		return;
	}
	
	Dome::postEvent (event);
}

int APMDome::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify dome hostname (with -e option)." << sendLog;
		return -1;
	}

	connDome = new rts2core::ConnAPM (host->getPort (), this, host->getHostname());
	
	int ret = connDome->init();
        
	if (ret)
		return ret;

	sendUDPMessage ("D999");

	setIdleInfoInterval (1);

        maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome after init");

	return 0;
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
	
	return Dome::info ();
}

int APMDome::startOpen ()
{
	if (isMoving () || dome_state == APMDOME_OPENED)                
		return 0;

	sendUDPMessage ("D100");
	
	return 0;
}

long APMDome::isOpened ()
{
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

	sendUDPMessage ("D777");
	return 0;
}
                
long APMDome::isClosed ()
{
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
	if (dome_state == APMDOME_CLOSING || dome_state == APMDOME_OPENING || dome_state == APMDOME_INIT)
		return true;

	return false;
}

int APMDome::sendUDPMessage (const char * _message)
{
	char *response = (char *)malloc (20*sizeof (char));
	char sA, sB, sideA[4], sideB[4];
	bool nowait = false;	
	
	// logStream (MESSAGE_DEBUG) << "command to controller: " << _message << sendLog;

	if (strcmp (_message, "D666") == 0)
		nowait = true;

	int n = connDome->sendReceive (_message, response, 20, nowait);

	// std::string res (response);
	// logStream (MESSAGE_DEBUG) << "reponse from controller: " << res.substr (0, n) << sendLog;
	
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

			// logStream (MESSAGE_DEBUG) << "#1: " << sA << " #2:" << sideA << " #3: " << sB << " #4: " << sideB << sendLog;

			if (sA == '1' || sB == '1')
			{
				// moving to open
				dome_state = APMDOME_OPENING;
				domeStatus->setValueString("Opening");
			} 

			if (sA == '2' || sB == '2')
			{
				// moving to close
				dome_state = APMDOME_CLOSING; 
				domeStatus->setValueString("Closing");
			}
		
			if (sA == '3' || sB == '3')
			{
				// emergency stop
				dome_state = APMDOME_STOPPED;
				domeStatus->setValueString("Stopped");
			}

			if (sA == '4' || sB == '4')
			{
				// initial closing
				dome_state = APMDOME_INIT;
				domeStatus->setValueString("Closing / init");
			}

			if ((strcmp (sideA, "100") == 0) && (strcmp (sideB, "100") == 0) && sA == '0' && sB == '0')
			{
				// dome fully opened
				dome_state = APMDOME_OPENED;				
				domeStatus->setValueString("Opened");
			}

			if ((strcmp (sideA, "000") == 0) && (strcmp (sideB, "000") == 0) && sA == '0' && sB == '0')
                        {
                                // dome fully closed
                                dome_state = APMDOME_CLOSED;
				domeStatus->setValueString("Closed");
                        }

			sendValueAll(domeStatus);
		}
	}

	return 0;
}

int main (int argc, char **argv)
{
	APMDome device (argc, argv);
	return device.run ();
}
