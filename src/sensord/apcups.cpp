/* 
 * System sensor, displaying free disk space and more.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "sensord.h"

#include "../utils/rts2connnosend.h"

#include <iomanip>
#include <netdb.h>
#include <fcntl.h>

namespace rts2sensor
{

	/**
	 * Class for communication with APC UPS.
	 *
	 * @author Petr Kubanek <petr@kubanek.net>
	 */
	class ConnApcUps: public Rts2ConnNoSend
	{
		private:
			const char *hostname;
			int port;

		public:
			/**
			 * Create new connection to APC UPS daemon.
			 *
			 * @param _master   Reference to master holding this connection.
			 *
			 * @param _hostname APC UPSD IP address or hostname.
			 * @param _port     Portnumber of APC UPSD daemon (default to 3551).
			 */
			ConnApcUps (Rts2Block *_master, const char *_hostname, int _port);

			/**
			 * Init TCP/IP connection to given host.
			 *
			 * @return -1 on error, 0 on success.
			 */
			virtual int init ();

			/**
			 * Call command, get reply.
			 *
			 * @param cmd       Command.
			 * @param _buf       Buffer holding reply data.
			 * @param _buf_size  Size of buffer for reply data.
			 */
			int command (const char *cmd, char *_buf, int _buf_size);
	};

	/**
	 * Sensor for accessing APC UPS informations.
	 *
	 * @author Petr Kubanek <petr@kubanek.net>
	 */
	class ApcUps:public SensorWeather
	{
		private:
			HostString *host;
			ConnApcUps *connApc;

		protected:
			virtual int processOption (int opt);
			virtual int info ();

			virtual int init ();

		public:
			ApcUps (int argc, char **argv);
			virtual ~ApcUps (void);
	};

}


using namespace rts2sensor;


ConnApcUps::ConnApcUps (Rts2Block *_master, const char *_hostname, int _port)
:Rts2ConnNoSend (_master)
{
	hostname = _hostname;
	port = _port;
}


int
ConnApcUps::init ()
{
	int ret;
	struct sockaddr_in apc_addr;
	struct hostent *hp;

	sock = socket (AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
                logStream (MESSAGE_ERROR) << "Cannot create socket for an APC UPS TCP/IP connection, error: " << strerror (errno) << sendLog;
                return -1;
        }

        apc_addr.sin_family = AF_INET;
        hp = gethostbyname(hostname);
        bcopy ( hp->h_addr, &(apc_addr.sin_addr.s_addr), hp->h_length);
        apc_addr.sin_port = htons(port);

        ret = connect (sock, (struct sockaddr *) &apc_addr, sizeof(apc_addr));
        if (ret == -1)
        {
                logStream (MESSAGE_ERROR) << "Cannot connect socket, error: " << strerror (errno) << sendLog;
                return -1;
        }

        ret = fcntl (sock, F_SETFL, O_NONBLOCK);
        if (ret)
                return -1;
        return 0;
}


int
ConnApcUps::command (const char *cmd, char *_buf, int _buf_size)
{
	int ret;
	uint16_t len = htons (strlen (cmd));
	ret = send (sock, &len, 2, 0);
	if (ret != 2)
	{
		logStream (MESSAGE_DEBUG) << "Cannot send data to TCP/IP port" << sendLog;
		connectionError (-1);
		return -1;
	}

	// receive data..
	int left = 2;
	bool data = false;

	while (left > 0)
	{
		fd_set read_set;

	        struct timeval read_tout;
	        read_tout.tv_sec = 20;
	        read_tout.tv_usec = 0;
	
	        FD_ZERO (&read_set);
	
	        FD_SET (sock, &read_set);
	
	        ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
	        if (ret < 0)
	        {
	                logStream (MESSAGE_ERROR) << "error calling select function for apc ups, error: " << strerror (errno)
	                        << sendLog;
	                connectionError (-1);
	                return -1;
	        }
	        else if (ret == 0)
	        {
	                logStream (MESSAGE_ERROR) << "Timeout during call to select function. Calling connection error." << sendLog;
	                connectionError (-1);
	                return -1;
	        }
	
	        char reply_data[left];
	        ret = recv (sock, reply_data, left, 0);
	        if (ret < 0)
	        {
	                logStream (MESSAGE_ERROR) << "Cannot read from APC UPS socket, error " << strerror (errno) << sendLog;
	                connectionError (-1);
	                return -1;
	        }
		if (!data)
		{
			left = ntohs (*((uint16_t *) reply_data));
			data = true;	
		}
		else
		{
			left -= ret;
			reply_data[ret] = '\0';
			std::cout << reply_data << std::endl;
		}
	}
	return 0;
}


int
ApcUps::processOption (int opt)
{
	switch (opt)
	{
		case 'a':
			host = new HostString (optarg, "3551");
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}


int
ApcUps::init ()
{
  	int ret;
	ret = SensorWeather::init ();
	if (ret)
		return ret;
	
	connApc = new ConnApcUps (this, host->getHostname (), host->getPort ());
	ret = connApc->init ();
	if (ret)
		return ret;
	ret = info ();
	if (ret)
		return ret;
	return 0;
}


int
ApcUps::info ()
{
	int ret;
	char reply[500];
	ret = connApc->command ("status", reply, 500);
	return SensorWeather::info ();
}


ApcUps::ApcUps (int argc, char **argv):SensorWeather (argc, argv)
{
	addOption ('a', NULL, 1, "hostname[:port] of apcupds");
}


ApcUps::~ApcUps (void)
{
	delete connApc;
}


int
main (int argc, char **argv)
{
	ApcUps device = ApcUps (argc, argv);
	return device.run ();
}
