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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include "dome.h"

#define ESATBT_DOME_OPENING		1
#define ESATBT_DOME_CLOSING		2
#define ESATBT_DOME_OPENED		3
#define ESATBT_DOME_CLOSED		4
#define ESATBT_DOME_STOPPED		5
#define ESATBT_DOME_INIT		6

using namespace rts2dome;

namespace rts2dome
{

/**
 * Class for APM dome driver.
 *
 * @author Standa Vítek <standa@vitkovi.net>
 */
class EsaDome:public Dome
{
	public:
		EsaDome (int argc, char **argv);
		virtual int initHardware ();

		virtual int commandAuthorized (rts2core::Connection *conn);

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

		int dome_state;

		HostString *host;

		int sock;
                struct sockaddr_in servaddr, clientaddr;

		bool isMoving ();
		int getUDPStatus ();
		int sendUDPMessage (const char * in_message);
};

}

EsaDome::EsaDome (int argc, char **argv):Dome (argc, argv)
{
	host = NULL;

	addOption ('e', NULL, 1, "ESA dome IP and port (separated by :)");
	createValue(domeStatus, "dome_status", "current dome status", true);
	createValue(sideApos, "sideA_pos", "last known side A position", true);
	createValue(sideBpos, "sideB_pos", "last known side B position", true);
}

int EsaDome::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("stop"))
	{
		sendUDPMessage ("D666");
		return 0;	
	}
	
	return Dome::commandAuthorized (conn);
}

int EsaDome::processOption (int in_opt)
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

int EsaDome::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify dome hostname (with -e option)." << sendLog;
		return -1;
	}

	sock = socket (AF_INET, SOCK_DGRAM, 0);

        bzero (&servaddr, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr (host->getHostname ());
        servaddr.sin_port = htons (host->getPort ());
 
	bzero (&clientaddr, sizeof (clientaddr));
	clientaddr.sin_family = AF_INET; 
	clientaddr.sin_port = htons (host->getPort());
    	clientaddr.sin_addr.s_addr = htonl (INADDR_ANY);

	if (bind (sock , (struct sockaddr*)&clientaddr, sizeof(clientaddr) ) == -1)
    	{
        	logStream (MESSAGE_ERROR) << "Unable to bind socket to port" << sendLog;
        	return 1;
    	}

	if (!isMoving ())
        {
                // initial dome close
		sendUDPMessage ("D777");

                maskState (DOME_DOME_MASK, DOME_CLOSING, "closing dome after init");
        }

	return 0;
}

int EsaDome::setValue(rts2core::Value *old_value, rts2core::Value *new_value)
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

int EsaDome::info ()
{
	switch(dome_state)
	{
		case ESATBT_DOME_CLOSING:
			domeStatus->setValueString("Closing");
			break;
		case ESATBT_DOME_OPENING:
			domeStatus->setValueString("Openning");
			break;
		case ESATBT_DOME_CLOSED:
			domeStatus->setValueString("Closed");
			break;
		case ESATBT_DOME_OPENED:
			domeStatus->setValueString("Opened");
			break;
		case ESATBT_DOME_STOPPED:
			domeStatus->setValueString("Stopped");
			break;
		case ESATBT_DOME_INIT:
			domeStatus->setValueString("Closing (init)");
			break;
		default:
			domeStatus->setValueString("Unknown...");
	}
	
	sendValueAll(domeStatus);

	return Dome::info ();
}

int EsaDome::startOpen ()
{
	if (isMoving () || dome_state == ESATBT_DOME_OPENED)                
		return 0;

	sendUDPMessage ("D100");
	domeStatus->setValueString("Openning");
	sendValueAll(domeStatus);
	
	return 0;
}

long EsaDome::isOpened ()
{
	// return (getState () & DOME_DOME_MASK) == DOME_CLOSED ? 0 : -2;
	if (isMoving () || dome_state == ESATBT_DOME_CLOSED)
		return USEC_SEC;

	if (dome_state == ESATBT_DOME_OPENED)
		return -2;

	return USEC_SEC;
}
                
int EsaDome::endOpen ()
{
	return 0;
}
                
int EsaDome::startClose ()
{
	if (isMoving () || dome_state == ESATBT_DOME_CLOSED)
		return 0;

	sendUDPMessage ("D000");
	domeStatus->setValueString("Closing");
	sendValueAll(domeStatus);
	return 0;
}
                
long EsaDome::isClosed ()
{
	// return (getState () & DOME_DOME_MASK) == DOME_OPENED ? 0 : -2;
	if (isMoving () || dome_state == ESATBT_DOME_OPENED)
                return USEC_SEC;

        if (dome_state == ESATBT_DOME_CLOSED)
                return -2;

        return USEC_SEC;
}
                
int EsaDome::endClose ()
{
	return 0;
}

bool EsaDome::isMoving ()
{
	int response = getUDPStatus();

	if (dome_state == ESATBT_DOME_CLOSING || dome_state == ESATBT_DOME_OPENING)
		return true;

	return false;
}

int EsaDome::getUDPStatus ()
{
	return sendUDPMessage ("D999");
}

int EsaDome::sendUDPMessage (const char * in_message)
{
	unsigned int slen = sizeof (clientaddr);
	char * status_message = (char *)malloc (20*sizeof (char));
	char sA, sB, sideA[4], sideB[4];
	
	if (getDebug())
		logStream (MESSAGE_DEBUG) << "command to controller: " << in_message << sendLog;
	
	sendto (sock, in_message, strlen(in_message), 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

	int n = recvfrom (sock, status_message, 20, 0, (struct sockaddr *) &clientaddr, &slen);

        if (getDebug())
		logStream (MESSAGE_DEBUG) << "reponse from controller: " << status_message << sendLog;

	if (n > 4)
	{
		if (status_message[0] == 'E') 
		{
			// echo from controller "Echo: [cmd]"
			return 10;
		}
		else
		{
			sA = status_message[2];
			sB = status_message[6];

			// side A position
			strncpy (sideA, status_message+3, 3);
			sideA[3] = '\0';

			strncpy (sideB, status_message+7, 3);
                        sideB[3] = '\0';


			sideApos->setValueString(sideA);
			sendValueAll(sideApos);
			sideBpos->setValueString(sideB);
			sendValueAll(sideBpos);

			logStream (MESSAGE_DEBUG) << "#1: " << sA << " #2:" << sideA << " #3: " << sB << " #4: " << sideB << sendLog;

			if (sA == '1' || sB == '1')
			{
				// moving to open
				dome_state = ESATBT_DOME_OPENING;
				return 0;
			} 

			if (sA == '2' || sB == '2')
			{
				// moving to close
				dome_state = ESATBT_DOME_CLOSING; 
			}
		
			if (sA == '3' || sB == '3')
			{
				// emergency stop
				dome_state = ESATBT_DOME_STOPPED;
			}

			if (sA == '4' || sB == '4')
			{
				// initial closing
				dome_state = ESATBT_DOME_INIT;
			}

			if ((strcmp (sideA, "100") == 0) && (strcmp (sideB, "100") == 0))
			{
				// dome fully opened
				dome_state = ESATBT_DOME_OPENED;				
			}

			if ((strcmp (sideA, "000") == 0) && (strcmp (sideB, "000") == 0))
                        {
                                // dome fully closed
                                dome_state = ESATBT_DOME_CLOSED;
                        }

			return 0;
		}
	}

	return -1;
}

int main (int argc, char **argv)
{
	EsaDome device (argc, argv);
	return device.run ();
}
