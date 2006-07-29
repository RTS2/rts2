#include "rts2connbufweather.h"

WeatherBuf::WeatherBuf ()
{

}

WeatherBuf::~WeatherBuf ()
{
  values.clear ();
}

int
WeatherBuf::parse (char *buf)
{
  char *name;
  char *value;
  char *endval;
  float fval;
  bool last = false;
  while (*buf)
    {
      // eat blanks
      while (*buf && isblank (*buf))
	buf++;
      name = buf;
      while (*buf && *buf != '=')
	buf++;
      if (!*buf)
	break;
      *buf = '\0';
      buf++;
      value = buf;
      while (*buf && *buf != ',')
	buf++;
      if (!*buf)
	last = true;
      *buf = '\0';
      fval = strtod (value, &endval);
      if (*endval)
	{
	  if (!strcmp (value, "no"))
	    {
	      fval = 0;
	    }
	  else if (!strcmp (value, "yes"))
	    {
	      fval = 1;
	    }
	  else
	    {
	      break;
	    }
	}
      values.push_back (WeatherVal (name, fval));
      if (!last)
	buf++;
    }
  if (*buf)
    return -1;
  return 0;
}

void
WeatherBuf::getValue (const char *name, float &val, int &status)
{
  if (status)
    return;
  for (std::vector < WeatherVal >::iterator iter = values.begin ();
       iter != values.end (); iter++)
    {
      if ((*iter).isValue (name))
	{
	  val = (*iter).value;
	  return;
	}
    }
  status = -1;
}

Rts2ConnBufWeather::Rts2ConnBufWeather (int in_weather_port, int in_weather_timeout, int in_conn_timeout, int in_bad_weather_timeout, int in_bad_windspeed_timeout, Rts2DevDome * in_master):
Rts2ConnFramWeather (in_weather_port, in_weather_timeout, in_master)
{
  conn_timeout = in_conn_timeout;
  bad_weather_timeout = in_bad_weather_timeout;
  bad_windspeed_timeout = in_bad_windspeed_timeout;
  master = in_master;
}

int
Rts2ConnBufWeather::receive (fd_set * set)
{
  int ret, ret_c;
  char Wbuf[500];
  int data_size = 0;
  float rtIsRaining;
  float rtRainRate;
  float rtWetness;
  float rtCloudTop;
  float rtCloudBottom;
  float rtOutsideHum;
  float rtOutsideTemp;
  float weatherTimeout;
  if (sock >= 0 && FD_ISSET (sock, set))
    {
      struct sockaddr_in from;
      socklen_t size = sizeof (from);
      data_size =
	recvfrom (sock, Wbuf, 500, 0, (struct sockaddr *) &from, &size);
      if (data_size < 0)
	{
	  syslog (LOG_DEBUG, "error in receiving weather data: %m");
	  return 1;
	}
      Wbuf[data_size] = 0;
#ifdef DEBUG_EXTRA
      syslog (LOG_DEBUG, "readed: %i %s from: %s:%i", data_size, Wbuf,
	      inet_ntoa (from.sin_addr), ntohs (from.sin_port));
#endif
      // parse weather info
      //rtExtraTemp2=3.3, rtWindSpeed=0.0, rtInsideHum=22.0, rtWindDir=207.0, rtExtraTemp1=3.9, rtRainRate=0.0, rtOutsideHum=52.0, rtWindAvgSpeed=0.4, rtInsideTemp=23.4, rtExtraHum1=51.0, rtBaroCurr=1000.0, rtExtraHum2=51.0, rtOutsideTemp=0.5/
      WeatherBuf *weather = new WeatherBuf ();
      ret = weather->parse (Wbuf);
      ret_c = ret;
      weather->getValue ("weatherTimeout", weatherTimeout, ret_c);
      // if we found weatherTimeout - that's message beeing send to us to set timeout
      if (!ret_c)
	{
	  badSetWeatherTimeout ((int) weatherTimeout);
	}

      weather->getValue ("rtIsRaining", rtIsRaining, ret);
      weather->getValue ("rtRainRate", rtRainRate, ret);
      weather->getValue ("rtWindAvgSpeed", windspeed, ret);
      weather->getValue ("rtOutsideHum", rtOutsideHum, ret);
      weather->getValue ("rtOutsideTemp", rtOutsideTemp, ret);
      if (ret && ret_c)
	{
	  rain = 1;
	  badSetWeatherTimeout (conn_timeout);
	  return data_size;
	}
      if (rtIsRaining > 0)
	{
	  ret_c = 0;
	  // try to get more information about nature of rain and cloud cover
	  weather->getValue ("rtCloudTop", rtCloudTop, ret_c);
	  weather->getValue ("rtCloudBottom", rtCloudBottom, ret_c);
	  weather->getValue ("rtWetness", rtWetness, ret_c);
	  if (ret_c == 0)
	    {
	      float dew;
	      float vapor;
	      vapor =
		(rtOutsideHum / 100) * 0.611 * exp (17.27 * rtOutsideTemp /
						    (rtOutsideTemp + 237.3));
	      dew =
		ceil (10 *
		      ((116.9 + 237.3 * log (vapor)) / (16.78 -
							log (vapor)))) / 10;
	      syslog (LOG_DEBUG,
		      "Rts2ConnBufWeather::parse rtCloudBottom %f rtCloudTop %f rtRainRate %f rtWetness %f vapor %f dew %f",
		      rtCloudBottom, rtCloudTop, rtRainRate, rtWetness, vapor,
		      dew);
	      if ((rtCloudBottom - rtCloudTop) > 2.5 && rtRainRate == 0
		  && rtWetness < 15.0 && fabs (rtOutsideTemp - dew) < 3.0)
		rain = 0;
	      else if ((rtCloudBottom - rtCloudTop) > 3.0 && rtRainRate == 0
		       && rtWetness < 7.0 && fabs (rtOutsideTemp - dew) < 6.0)
		rain = 0;
	      else
		rain = 1;
	    }
	  else
	    {
	      rain = 1;
	    }
	}
      else
	{
	  rain = 0;
	}
      master->setTemperatur (rtOutsideTemp);
      master->setRain (rain);
      master->setHumidity (rtOutsideHum);
      master->setWindSpeed (windspeed);
      delete weather;

      time (&lastWeatherStatus);
      syslog (LOG_DEBUG, "windspeed: %f rain: %i date: %li status: %i",
	      windspeed, rain, lastWeatherStatus, ret);
      if (rain != 0 || windspeed > master->getMaxPeekWindspeed ())
	{
	  time (&lastBadWeather);
	  if (rain == 0 && windspeed > master->getMaxWindSpeed ())
	    badSetWeatherTimeout (bad_windspeed_timeout);
	  else
	    badSetWeatherTimeout (bad_weather_timeout);
	}
      // ack message
      sendto (sock, "Ack", 3, 0, (struct sockaddr *) &from, sizeof (from));
    }
  return data_size;
}
