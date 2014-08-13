/* 
 * ESA Test Bed Telescope dome driver.
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

#define ESATBT_DOME_CLOSED		0
#define ESATBT_DOME_OPEN		1
#define ESATBT_DOME_MOVING		3
#define ESATBT_DOME_UNKNOWN		4

using namespace rts2dome;

namespace rts2dome
{

/**
 * ESA Test Bed Telescope dome driver.
 *
 * @author Standa Vítek <standa@vitkovi.net>
 */
class EsaDome:public Dome
{
	public:
		EsaDome (int argc, char **argv);
		virtual int initHardware ();

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
		rts2core::ValueInteger *sw_state;
		rts2core::ValueString * domeStatus;
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

	dome_state = ESATBT_DOME_UNKNOWN;

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
                // close roof - security measurement
                // startClose ();
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
#ifdef BULL
	int response = getUDPStatus ();

	switch(response)
	{
		case ESATBT_DOME_CLOSED:
			domeStatus->setValueString("Closed");
			// status = CLOSED;
			break;
		case ESATBT_DOME_OPEN:
			domeStatus->setValueString("Open");
			// status = OPENED;
	}
	
	sendValueAll(domeStatus);
#endif
	return Dome::info ();
}

int EsaDome::startOpen ()
{
	if (isMoving () || dome_state == ESATBT_DOME_OPEN)                
		return 0;

	sendUDPMessage ("D100");
	
	return 0;
}

long EsaDome::isOpened ()
{
	return (getState () & DOME_DOME_MASK) == DOME_CLOSED ? 0 : -2;
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

	return 0;
}
                
long EsaDome::isClosed ()
{
	return (getState () & DOME_DOME_MASK) == DOME_OPENED ? 0 : -2;
}
                
int EsaDome::endClose ()
{
	return 0;
}

bool EsaDome::isMoving ()
{
	int response = getUDPStatus();

	if (response == ESATBT_DOME_MOVING)
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
	

	logStream (MESSAGE_DEBUG) << "command to controller: " << in_message << sendLog;
	
	sendto (sock, in_message, strlen(in_message), 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

	int n = recvfrom (sock, status_message, 20, 0, (struct sockaddr *) &clientaddr, &slen);

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

			logStream (MESSAGE_DEBUG) << "side A: " << sideA << " side B: " << sideB << sendLog;

			if (sA == '1' || sA == '2' || sB == '1' || sB == '2')
			{
				// moving
				return ESATBT_DOME_MOVING;
			} 

			if ((strcmp (sideA, "100") == 0) && (strcmp (sideB, "100") == 0))
			{
				// dome fully opened
				return ESATBT_DOME_OPEN;				
			}

			if ((strcmp (sideA, "000") == 0) && (strcmp (sideB, "000") == 0))
                        {
                                // dome fully closed
                                return ESATBT_DOME_CLOSED;
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
