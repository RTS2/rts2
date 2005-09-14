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

Rts2ConnFramWeather::Rts2ConnFramWeather (int in_weather_port,
					  Rts2DevDome * in_master):
Rts2ConnNoSend (in_master)
{
  master = in_master;
  weather_port = in_weather_port;

  lastWeatherStatus = 0;
  time (&lastBadWeather);
  nextGoodWeather = lastBadWeather + FRAM_CONN_TIMEOUT;
}

void
Rts2ConnFramWeather::setWeatherTimeout (time_t wait_time)
{
  time_t next;
  time (&next);
  next += wait_time;
  if (next > nextGoodWeather)
    nextGoodWeather = next;
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
      syslog (LOG_ERR, "Rts2ConnFramWeather::init socket: %m");
      return -1;
    }
  ret = fcntl (sock, F_SETFL, O_NONBLOCK);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2ConnFramWeather::init fcntl: %m");
      return -1;
    }
  ret = bind (sock, (struct sockaddr *) &bind_addr, sizeof (bind_addr));
  if (ret)
    {
      syslog (LOG_ERR, "Rts2ConnFramWeather::init bind: %m");
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
	  syslog (LOG_DEBUG, "error in receiving weather data: %m");
	  return 1;
	}
      Wbuf[data_size] = 0;
      syslog (LOG_DEBUG, "readed: %i %s from: %s:%i", data_size, Wbuf,
	      inet_ntoa (from.sin_addr), ntohs (from.sin_port));
      // parse weather info
      ret =
	sscanf (Wbuf,
		"windspeed=%f km/h rain=%i date=%i-%u-%u time=%u:%u:%fZ status=%s",
		&windspeed, &rain, &statDate.tm_year, &statDate.tm_mon,
		&statDate.tm_mday, &statDate.tm_hour, &statDate.tm_min,
		&sec_f, Wstatus);
      if (ret != 9)
	{
	  syslog (LOG_ERR, "sscanf on udp data returned: %i", ret);
	  rain = 1;
	  setWeatherTimeout (FRAM_CONN_TIMEOUT);
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
      syslog (LOG_DEBUG, "windspeed: %f rain: %i date: %li status: %s",
	      windspeed, rain, lastWeatherStatus, Wstatus);
      if (rain != 0 || windspeed > master->getMaxPeekWindspeed ())
	{
	  time (&lastBadWeather);
	  if (rain == 0 && windspeed > master->getMaxWindSpeed ())
	    setWeatherTimeout (FRAM_BAD_WINDSPEED_TIMEOUT);
	  else
	    setWeatherTimeout (FRAM_BAD_WEATHER_TIMEOUT);
	  master->closeDome ();
	  master->setMasterStandby ();
	}
      // ack message
      ret =
	sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from, sizeof (from));
    }
  return data_size;
}

int
Rts2ConnFramWeather::isGoodWeather ()
{
  time_t now;
  time (&now);
  // if no conenction, set nextGoodWeather appopritery
  if (now - lastWeatherStatus > FRAM_WEATHER_TIMEOUT)
    {
      setWeatherTimeout (FRAM_CONN_TIMEOUT);
      return 0;
    }
  if (windspeed > master->getMaxWindSpeed () || rain != 0
      || (nextGoodWeather > now))
    return 0;
  return 1;
}
