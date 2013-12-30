/* 
 * Sensor for Observatory de Sierra Nevada weather system.
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

#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>

namespace rts2sensord
{

/**
 * Class for OSN weather sensor.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Osn: public SensorWeather
{
	private:
		rts2core::ValueFloat *temp;
		rts2core::ValueFloat *humidity;
		rts2core::ValueFloat *wind1;
		rts2core::ValueFloat *wind2;

	protected:
		virtual int info ();

		virtual bool isGoodWeather ();	
	
	public:
		Osn (int argc, char **argv);
};

}

using namespace rts2sensord;

int
Osn::info ()
{
	int sockfd, bytes_read, ret, i;
	struct sockaddr_in dest;
	char buff[500];
	fd_set rfds;
	struct timeval tv;
	double _temp, _humi, _wind1, _wind2;
	char *token;

	if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
	{
		logStream (MESSAGE_ERROR) << "Unable to create socket to OSN:" << strerror (errno) << sendLog;
		return -1;
	}

	memset (&dest, 0, sizeof (dest));
	dest.sin_family = PF_INET;
	dest.sin_addr.s_addr = inet_addr ("193.146.151.70");
	dest.sin_port = htons (6341);
	ret = connect (sockfd, (struct sockaddr *) &dest, sizeof (dest));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Unable to connect to SNOW:" << strerror (errno) << sendLog;
		return -1;
	}

	/* send command to SNOW */
	sprintf (buff, "%s", "RCD");
	ret = write (sockfd, buff, strlen (buff));
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Cannot write data:" << strerror (errno) << sendLog;
		return -1;
	}

	FD_ZERO (&rfds);
	FD_SET (sockfd, &rfds);

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	ret = select (FD_SETSIZE, &rfds, NULL, NULL, &tv);

	if (ret == 1)
	{
		bytes_read = read (sockfd, buff, sizeof (buff));
		if (bytes_read > 0)
		{
			for (i = 5; i < bytes_read; i++)
				if (buff[i] == ',')
					buff[i] = '.';
		}
	}
	else
	{
		logStream (MESSAGE_ERROR) << "No meteo data from OSN. Check the connection." << sendLog;
		return -1;
	}

	close (sockfd);

	token = strtok ((char *) buff + 5, "|");
	for (i = 6; i < 45; i++)
	{
		token = (char *) strtok (NULL, "|#");
		if (i == 8)
			_wind1 = atof (token);
		if (i == 11)
			_wind2 = atof (token);
		if (i == 23)
			_temp = atof (token);
		if (i == 29)
			_humi = atof (token);
	}

	temp->setValueFloat (_temp);
	humidity->setValueFloat (_humi);
	wind1->setValueFloat (_wind1);
	wind2->setValueFloat (_wind2);

	return SensorWeather::info ();
}


bool
Osn::isGoodWeather ()
{
	if (wind1->getValueFloat () > 50 || wind2->getValueFloat () > 50 || humidity->getValueFloat () > 99)
		return false;
	return SensorWeather::isGoodWeather ();
}


Osn::Osn (int argc, char **argv)
:SensorWeather (argc, argv)
{
	createValue (temp, "TEMP", "temperature in degrees C");
	createValue (humidity, "HUMI", "(outside) humidity");
	createValue (wind1, "WIND_E", "windspeed on east mast");
	createValue (wind2, "WIND_W", "windspeed on west mast");
}


int
main (int argc, char **argv)
{
	Osn device = Osn (argc, argv);
	return device.run ();
}
