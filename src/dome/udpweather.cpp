/**********************************************************************************************
 *
 * Class for weather info connection.
 *
 * Bind to specified port, send back responds packet, changed state
 * acordingly to weather service, close/open dome as well (using
 * master pointer)
 *
 * To be used in Pierre-Auger site in Argentina.
 *
 *********************************************************************************************/

#include "udpweather.h"

#include <fcntl.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

Rts2ConnFramWeather::Rts2ConnFramWeather (int in_weather_port,
int in_weather_timeout,
Rts2DevDome * in_master):
Rts2ConnNoSend (in_master)
{
	master = in_master;
	weather_port = in_weather_port;
	weather_timeout = in_weather_timeout;

	lastWeatherStatus = 0;
	time (&lastBadWeather);
}


void
Rts2ConnFramWeather::setWeatherTimeout (time_t wait_time)
{
	master->setWeatherTimeout (wait_time);
}


void
Rts2ConnFramWeather::cancelIgnore ()
{
	master->setIgnoreMeteo (false);
}


void
Rts2ConnFramWeather::badSetWeatherTimeout (time_t wait_time)
{
	master->setWeatherTimeout (wait_time);
	master->closeDomeWeather ();
}


int
Rts2ConnFramWeather::init ()
{
	struct sockaddr_in bind_addr;
	int ret;

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons (weather_port);
	bind_addr.sin_addr.s_addr = htonl (INADDR_ANY);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnFramWeather::init socket: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnFramWeather::init fcntl: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	ret = bind (sock, (struct sockaddr *) &bind_addr, sizeof (bind_addr));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2ConnFramWeather::init bind: " <<
			strerror (errno) << sendLog;
	}
	return ret;
}


int
Rts2ConnFramWeather::receive (fd_set * set)
{
	int ret;
	char Wbuf[100];
	char Wstatus[10];
	int data_size = 0;
	struct tm statDate;
	float sec_f;
	if (sock >= 0 && FD_ISSET (sock, set))
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
		logStream (MESSAGE_DEBUG) << "readed: " << data_size << " " << Wbuf <<
			" from " << inet_ntoa (from.sin_addr) << " " << ntohs (from.
			sin_port) <<
			sendLog;
		// parse weather info
		ret =
			sscanf (Wbuf,
			"windspeed=%f km/h rain=%i date=%i-%u-%u time=%u:%u:%fZ status=%s",
			&windspeed, &rain, &statDate.tm_year, &statDate.tm_mon,
			&statDate.tm_mday, &statDate.tm_hour, &statDate.tm_min,
			&sec_f, Wstatus);
		if (ret != 9)
		{
			logStream (MESSAGE_ERROR) << "sscanf on udp data returned: " << ret
				<< sendLog;
			int wtimeout = 3600;
			ret = sscanf (Wbuf, "weatherTimeout=%i", &wtimeout);
			if (ret != 1)
			{
				rain = 1;
				badSetWeatherTimeout (FRAM_CONN_TIMEOUT);
				return data_size;
			}
			cancelIgnore ();
			badSetWeatherTimeout (wtimeout);
			logStream (MESSAGE_DEBUG) << "Weathertimeout " << wtimeout <<
				sendLog;
			// ack message
			ret =
				sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from,
				sizeof (from));
			master->infoAll ();
			return data_size;
		}
		statDate.tm_isdst = 0;
		statDate.tm_year -= 1900;
		statDate.tm_mon--;
		statDate.tm_sec = (int) sec_f;
		lastWeatherStatus = mktime (&statDate);
		if (strcmp (Wstatus, "watch"))
		{
			// if sensors doesn't work, switch rain on
			rain = 1;
		}
		logStream (MESSAGE_DEBUG) << "windspeed: " << windspeed << " rain: " <<
			rain << " date: " << lastWeatherStatus << " status: " << Wstatus <<
			sendLog;
		if (rain != 0 || windspeed > master->getMaxPeekWindspeed ())
		{
			time (&lastBadWeather);
			if (rain == 0 && windspeed > master->getMaxWindSpeed ())
				badSetWeatherTimeout (FRAM_BAD_WINDSPEED_TIMEOUT);
			else
				badSetWeatherTimeout (FRAM_BAD_WEATHER_TIMEOUT);
		}
		// ack message
		ret =
			sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from, sizeof (from));
		master->infoAll ();
	}
	return data_size;
}


int
Rts2ConnFramWeather::isGoodWeather ()
{
	time_t now;
	time (&now);
	// if no conenction, set nextGoodWeather appopritery
	if (now - lastWeatherStatus > weather_timeout)
	{
		setWeatherTimeout (FRAM_CONN_TIMEOUT);
		return 0;
	}
	if (windspeed > master->getMaxWindSpeed () || rain != 0
		|| (master->getNextOpen () > now))
		return 0;
	return 1;
}
