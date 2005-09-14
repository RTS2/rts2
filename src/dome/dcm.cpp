/*! 
 * @file Dome control deamon for Dome Control Modules - BOOTESes
 *
 * $Id$
 *
 * Packtets are separated by space; they are in ascii, and ends with 0
 * followed by checksum calculated using function ..
 *
 * <dome_name> YYYY-MM-DD hh:mm:ssZ <temperature> <humidity> <rain> <switch1 - o|c> <switch2 .. 4>
 * <status>
 *
 * where:
 * <dome_name> is name of dome (1A or 1B)
 * YYYY-MM-DD hh:mm:ssZ is UTC date of message
 * temperature is float number in oC
 * humidity is float 0-100
 * rain is 1 when it's raining, otherwise 0
 * switch1-n are switch position - o for open (not switched), c for
 * closed (switched)
 * status is ok or error. Ok when all sensors are ready
 *
 * Example packet:
 *
 * 1A 2005-07-21 23:56:56 -10.67 98.9 0 c c o o ok
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !_GNU_SOURCE */

#include <signal.h>
#include <fcntl.h>

#include "dome.h"
#include "udpweather.h"

class Rts2DevDomeDcm;

class Rts2ConnDcm:public Rts2ConnFramWeather
{
private:
  Rts2DevDomeDcm * master;
public:
  Rts2ConnDcm (int in_weather_port, Rts2DevDomeDcm * in_master);
    virtual ~ Rts2ConnDcm (void);
  virtual int receive (fd_set * set);
};

class Rts2DevDomeDcm:public Rts2DevDome
{
private:
  Rts2ConnFramWeather * weatherConn;
  int dcm_weather_port;

protected:
    virtual int processOption (int in_opt);
public:
    Rts2DevDomeDcm (int argc, char **argv);
    virtual ~ Rts2DevDomeDcm (void);
  virtual int init ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
};

Rts2ConnDcm::Rts2ConnDcm (int in_weather_port, Rts2DevDomeDcm * in_master):
Rts2ConnFramWeather (in_weather_port, in_master)
{
  master = in_master;
}

Rts2ConnDcm::~Rts2ConnDcm (void)
{
}

int
Rts2ConnDcm::receive (fd_set * set)
{
  int ret;
  char Wbuf[100];
  char Wstatus[10];
  char dome_name[10];
  int data_size = 0;
  struct tm statDate;
  float temp;
  float humidity;

  int sw1, sw2, sw3, sw4;

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
#ifdef DEBUG_ALL
      syslog (LOG_DEBUG, "readed: %i '%s' from: %s:%i", data_size, Wbuf,
	      inet_ntoa (from.sin_addr), ntohs (from.sin_port));
#endif
      // parse weather info
      // * 1A 2005-07-21 23:56:56 -10.67 98.9 0 c c o o ok
      ret =
	sscanf (Wbuf,
		"%s %i-%u-%u %u:%u:%f %f %f %i %i %i %i %i %s",
		dome_name, &statDate.tm_year, &statDate.tm_mon,
		&statDate.tm_mday, &statDate.tm_hour, &statDate.tm_min,
		&sec_f, &temp, &humidity, &rain, &sw1, &sw2, &sw3, &sw4,
		Wstatus);
      if (ret != 15)
	{
	  syslog (LOG_ERR, "sscanf on udp data returned: %i ('%s')", ret,
		  Wbuf);
	  rain = 1;
	  setWeatherTimeout (FRAM_CONN_TIMEOUT);
	  return data_size;
	}
      statDate.tm_isdst = 0;
      statDate.tm_year -= 1900;
      statDate.tm_mon--;
      statDate.tm_sec = (int) sec_f;
      lastWeatherStatus = mktime (&statDate);
      if (strcmp (Wstatus, "ok"))
	{
	  // if sensors doesn't work, switch rain on
	  rain = 1;
	}
      // log only rain messages..they are interesting
      if (rain)
	syslog (LOG_DEBUG, "rain: %i date: %li status: %s",
		rain, lastWeatherStatus, Wstatus);
      master->setTemperatur (temp);
      master->setHumidity (humidity);
      master->setRain (rain);
      master->setSwState ((sw1 << 3) | (sw2 << 2) | (sw3 << 1) | (sw4));
      if (sw1 && sw2 && !sw3 && !sw4)
	{
	  master->setMasterOn ();
	}
      else if (!sw1 && !sw2)
	{
	  master->setMasterStandby ();
	}
    }
  return data_size;
}

Rts2DevDomeDcm::Rts2DevDomeDcm (int in_argc, char **in_argv):
Rts2DevDome (in_argc, in_argv)
{
  weatherConn = NULL;
  addOption ('W', "dcm_weather", 1,
	     "UPD port number of packets from DCM (default to 4998)");
  dcm_weather_port = 4998;
}

Rts2DevDomeDcm::~Rts2DevDomeDcm (void)
{
}

int
Rts2DevDomeDcm::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'W':
      dcm_weather_port = atoi (optarg);
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevDomeDcm::init ()
{
  int ret;
  int i;

  ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  weatherConn = new Rts2ConnDcm (dcm_weather_port, this);
  weatherConn->init ();

  ret = addConnection (weatherConn);
  if (ret)
    {
      delete weatherConn;
      return -1;
    }

  return 0;
}

int
Rts2DevDomeDcm::ready ()
{
  return 0;
}

int
Rts2DevDomeDcm::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeDcm::info ()
{
  return 0;
}

Rts2DevDomeDcm *device;

void
switchoffSignal (int sig)
{
  if (device)
    delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevDomeDcm (argc, argv);

  signal (SIGINT, switchoffSignal);
  signal (SIGTERM, switchoffSignal);

  int ret;
  ret = device->init ();
//  device->sendFramMail ("FRAM DOME restart");
  if (ret)
    {
      fprintf (stderr, "Cannot initialize dome - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
