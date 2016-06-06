/* 
 * Infrastructure for Pierre Auger UDP connection.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "framudp.h"
#include "fram-weather.h"
#include "libnova_cpp.h"

#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace rts2sensord;

ConnFramWeather::ConnFramWeather (int _weather_port, int _weather_timeout, FramWeather * _master):rts2core::ConnNoSend (_master)
{
	master = _master;
	weather_port = _weather_port;
}

int ConnFramWeather::init ()
{
	struct sockaddr_in bind_addr;
	int ret;

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons (weather_port);
	bind_addr.sin_addr.s_addr = htonl (INADDR_ANY);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) << "ConnFramWeather::init socket: "
			<< strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnFramWeather::init fcntl: "
			<< strerror (errno) << sendLog;
		return -1;
	}
	ret = bind (sock, (struct sockaddr *) &bind_addr, sizeof (bind_addr));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnFramWeather::init bind: " <<
			strerror (errno) << sendLog;
	}
	return ret;
}

int ConnFramWeather::receive (rts2core::Block *block)
{
	int ret;
	char Wbuf[100];
	char Wstatus[10];
	int data_size = 0;
	struct ln_date statDate;
	int rain = 1;
	float windspeed;
	if (sock >= 0 && block->isForRead (sock))
	{
		struct sockaddr_in from;
		socklen_t size = sizeof (from);
		data_size =
			recvfrom (sock, Wbuf, 80, 0, (struct sockaddr *) &from, &size);
		if (data_size < 0)
		{
			logStream (MESSAGE_DEBUG) << "error in receiving weather data: %m"
				<< strerror (errno) << sendLog;
			return 1;
		}
		Wbuf[data_size] = 0;
		// parse weather info
		ret = sscanf (Wbuf,
			"windspeed=%f km/h rain=%i date=%i-%u-%u time=%u:%u:%lfZ status=%s",
			&windspeed, &rain, &statDate.years, &statDate.months,
			&statDate.days, &statDate.hours, &statDate.minutes,
			&statDate.seconds, Wstatus);
		if (ret != 9)
		{
			logStream (MESSAGE_ERROR) << "sscanf on udp data returned: " << ret << sendLog;
			int wtimeout = 3600;
			ret = sscanf (Wbuf, "weatherTimeout=%i", &wtimeout);
			if (ret != 1)
			{
				logStream (MESSAGE_ERROR) << "invalid timeout specified in weather timeout, invalid data" << Wbuf << sendLog;
				master->setWeatherTimeout (wtimeout, "cannot parse weatherTimeout");
			}
			master->setWeatherTimeout (wtimeout, "cannot parse packet from weather station");
			logStream (MESSAGE_DEBUG) << "Weathertimeout " << wtimeout << sendLog;
			// ack message
			ret = sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from, sizeof (from));
			master->infoAll ();
			return data_size;
		}
		master->setWeather (windspeed, rain, Wstatus, &statDate);

		// ack message
		ret = sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from, sizeof (from));
	}
	return data_size;
}
