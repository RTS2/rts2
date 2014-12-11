/**
 * Standalone version of APM filter wheel (ESA TestBed telescope)
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

#include "filterd.h"

using namespace rts2filterd;

/*
 * Class for APM filter wheel
 *
 * @author Standa Vitek <standa@vitkovi.net>
 *
 */

class EsaFilter : public Filterd
{
	public:
		EsaFilter (int in_argc, char **in_argv);
		virtual int initHardware ();
	protected:
		virtual int processOption (int in_opt);
		virtual int getFilterNum (void);
		virtual int setFilterNum (int new_filter);
		virtual int homeFilter ();
	private:
		HostString *host;
		int sock;
		struct sockaddr_in servaddr, clientaddr;
		// rts2core::ConnAPM *apm;
		int sendUDPMessage (const char * in_message);
		int filterNum;
		int filterSleep;
};


EsaFilter::EsaFilter (int argc, char **argv):Filterd (argc, argv)
{
	host = NULL;
	filterNum = 0;
	filterSleep = 3;

	addOption ('e', NULL, 1, "IP and port (separated by :)");
	addOption ('s', "filter_sleep", 1, "how long wait for filter change");
}

int EsaFilter::processOption (int in_opt)
{
        switch (in_opt)
        {
                case 'e':
                        host = new HostString (optarg, "1000");
                        break;
		case 's':
			filterSleep = atoi (optarg);
			break;
                default:
                        return Filterd::processOption (in_opt);
        }
        return 0;
}

int EsaFilter::getFilterNum ()
{
	sendUDPMessage ("F999");

	return filterNum;
}

int EsaFilter::setFilterNum (int new_filter)
{
	char *m = (char *)malloc(5*sizeof(char));
	sprintf (m, "F00%d", new_filter);

	sendUDPMessage (m);

	filterNum = new_filter;

	sleep (filterSleep);

	return Filterd::setFilterNum (new_filter);
}

int EsaFilter::homeFilter ()
{
	setFilterNum (0);
	
	return 0;
}

int EsaFilter::initHardware ()
{
	if (host == NULL)
        {
                logStream (MESSAGE_ERROR) << "You must specify filter hostname (with -e option)." << sendLog;
                return -1;
        }

	// apm = new rts2core::ConnAPM (this, host->getHostname (), host->getPort ());

	sock = socket (AF_INET, SOCK_DGRAM, 0);

        bzero (&servaddr, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr (host->getHostname());
        servaddr.sin_port = htons (host->getPort());

        bzero (&clientaddr, sizeof (clientaddr));
        clientaddr.sin_family = AF_INET;
        clientaddr.sin_port = htons (host->getPort());
        clientaddr.sin_addr.s_addr = htonl (INADDR_ANY);

        if (bind (sock , (struct sockaddr*)&clientaddr, sizeof(clientaddr) ) == -1)
        {
                logStream (MESSAGE_ERROR) << "Unable to bind socket to port" << sendLog;
                return 1;
        }	

	return 0;
}

int EsaFilter::sendUDPMessage (const char * in_message)
{
	unsigned int slen = sizeof (clientaddr);
        char * status_message = (char *)malloc (20*sizeof (char));

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
	}
	else if (n == 4)
	{	
		filterNum = status_message[3];
	}

	return 0;
}

int main (int argc, char ** argv)
{
        EsaFilter device (argc, argv);

        return device.run ();
}
