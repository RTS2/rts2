#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <vector>

#include "dome.h"
#include "udpweather.h"

#define BAUDRATE B9600
#define CTENI_PORTU 0
#define ZAPIS_NA_PORT 4
#define STAV_PORTU 0
#define CHYBA_NEZNAMY_PRIKAZ 0x80
#define CHYBA_PRETECENI_TX_BUFFERU 0x81
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2

#define CAS_NA_OTEVRENI 30

// we should get packets every minute; 5 min timeout, as data from meteo station
// aren't as precise as we want and we observe dropouts quite often
#define BART_WEATHER_TIMEOUT  360

// how long after weather was bad can weather be good again; in
// seconds
#define BART_BAD_WEATHER_TIMEOUT   360
#define BART_BAD_WINDSPEED_TIMEOUT 360
#define BART_CONN_TIMEOUT	   360

typedef enum
{ ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_4, ZASUVKA_5, ZASUVKA_6, MOTOR,
  SMER, SVETLO, TOPENI, KONCAK_OTEVRENI_JIH, KONCAK_ZAVRENI_JIH,
  KONCAK_OTEVRENI_SEVER, KONCAK_ZAVRENI_SEVER, RESET_1, RESET_2
} vystupy;

struct typ_a
{
  unsigned char port;
  unsigned char pin;
} adresa[] =
{
  {
  PORT_A, 1},
  {
  PORT_A, 2},
  {
  PORT_A, 4},
  {
  PORT_A, 8},
  {
  PORT_A, 16},
  {
  PORT_A, 32},
  {
  PORT_B, 1},
  {
  PORT_B, 2},
  {
  PORT_B, 4},
  {
  PORT_B, 8},
  {
  PORT_B, 16},
  {
  PORT_B, 32},
  {
  PORT_B, 64},
  {
  PORT_B, 128},
  {
  PORT_C, 1},
  {
PORT_C, 32}};

#define NUM_ZAS		5

#define OFF		0
#define STANDBY		1
#define OBSERVING	2

// zasuvka c.1 kamera, c.3 kamera, c.5 montaz. c.6 topeni /Ford 21.10.04
int zasuvky_index[NUM_ZAS] =
  { ZASUVKA_1, ZASUVKA_2, ZASUVKA_3, ZASUVKA_5, ZASUVKA_6 };

enum stavy
{ ZAS_VYP, ZAS_ZAP };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
enum stavy zasuvky_stavy[3][NUM_ZAS] =
{
  // off
  {ZAS_ZAP, ZAS_ZAP, ZAS_VYP, ZAS_ZAP, ZAS_VYP},
  // standby
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP},
  // observnig
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

class WeatherVal
{
private:
  const char *name;
public:
  float value;

    WeatherVal (const char *in_name, float in_value)
  {
    name = in_name;
    value = in_value;
  }

  bool isValue (const char *in_name)
  {
    return !strcmp (name, in_name);
  }
};

class WeatherBuf
{
private:
  std::vector < WeatherVal > values;
public:
  WeatherBuf ();
  virtual ~ WeatherBuf (void);

  int parse (char *buf);
  void getValue (const char *name, float &val, int &status);
};

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
      if (!*buf)
	break;
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

class Rts2ConnBartWeather:public Rts2ConnFramWeather
{
private:
  Rts2DevDome * master;
public:
  Rts2ConnBartWeather (int in_weather_port, Rts2DevDome * in_master);
  virtual int receive (fd_set * set);
};

Rts2ConnBartWeather::Rts2ConnBartWeather (int in_weather_port,
					  Rts2DevDome * in_master):
Rts2ConnFramWeather (in_weather_port, BART_WEATHER_TIMEOUT, in_master)
{
  master = in_master;
}

int
Rts2ConnBartWeather::receive (fd_set * set)
{
  int ret;
  char Wbuf[500];
  int data_size = 0;
  float rtRainRate;
  float rtOutsideHum;
  float rtOutsideTemp;
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
      syslog (LOG_DEBUG, "readed: %i %s from: %s:%i", data_size, Wbuf,
	      inet_ntoa (from.sin_addr), ntohs (from.sin_port));
      // parse weather info
      //rtExtraTemp2=3.3, rtWindSpeed=0.0, rtInsideHum=22.0, rtWindDir=207.0, rtExtraTemp1=3.9, rtRainRate=0.0, rtOutsideHum=52.0, rtWindAvgSpeed=0.4, rtInsideTemp=23.4, rtExtraHum1=51.0, rtBaroCurr=1000.0, rtExtraHum2=51.0, rtOutsideTemp=0.5/
      WeatherBuf *weather = new WeatherBuf ();
      ret = weather->parse (Wbuf);
      weather->getValue ("rtIsRaining", rtRainRate, ret);
      weather->getValue ("rtWindAvgSpeed", windspeed, ret);
      weather->getValue ("rtOutsideHum", rtOutsideHum, ret);
      weather->getValue ("rtOutsideTemp", rtOutsideTemp, ret);
      if (ret)
	{
	  rain = 1;
	  setWeatherTimeout (BART_CONN_TIMEOUT);
	  return data_size;
	}
      rain = rtRainRate > 0 ? 1 : 0;
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
	    setWeatherTimeout (BART_BAD_WINDSPEED_TIMEOUT);
	  else
	    setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT);
	  master->closeDome ();
	  master->setMasterStandby ();
	}
    }
  return data_size;
}

class Rts2DevDomeBart:public Rts2DevDome
{
private:
  int dome_port;
  int rain_port;
  char *dome_file;
  char *rain_detector;

  unsigned char spinac[2];
  unsigned char stav_portu[3];

  Rts2ConnBartWeather *weatherConn;

  int zjisti_stav_portu ();
  void zapni_pin (unsigned char c_port, unsigned char pin);
  void vypni_pin (unsigned char c_port, unsigned char pin);
  int getPortState (int c_port)
  {
    return !!(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
  };

  int isOn (int c_port);
  int handle_zasuvky (int state);

  int ignoreMeteo;

protected:
  virtual int processOption (int in_opt);
  virtual int isGoodWeather ();

public:
  Rts2DevDomeBart (int argc, char **argv);
  virtual ~ Rts2DevDomeBart (void);

  virtual int init ();

  virtual int idle ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int openDome ();
  virtual long isOpened ();
  virtual int endOpen ();
  virtual int closeDome ();
  virtual long isClosed ();
  virtual int endClose ();

  virtual int observing ();
  virtual int standby ();
  virtual int off ();

  virtual int changeMasterState (int new_state);
};

Rts2DevDomeBart::Rts2DevDomeBart (int in_argc, char **in_argv):
Rts2DevDome (in_argc, in_argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");
  addOption ('R', "rain_detector", 1, "/dev/file for rain detector");
  addOption ('I', "ignore_meteo", 0, "whenever to ignore meteo station");
  dome_file = "/dev/ttyS0";
  rain_detector = NULL;
  rain_port = -1;

  domeModel = "BART_FORD_2";

  weatherConn = NULL;
  ignoreMeteo = 0;

  rain = 0;
  windspeed = nan ("f");
}

Rts2DevDomeBart::~Rts2DevDomeBart (void)
{
  close (rain_port);
  close (dome_port);
}

int
Rts2DevDomeBart::zjisti_stav_portu ()
{
  unsigned char ta, tb, c = STAV_PORTU | PORT_A;
  int ret;
  write (dome_port, &c, 1);
  if (read (dome_port, &ta, 1) < 1)
    syslog (LOG_ERR, "read error 0");
  read (dome_port, &stav_portu[PORT_A], 1);
  c = STAV_PORTU | PORT_B;
  write (dome_port, &c, 1);
  if (read (dome_port, &tb, 1) < 1)
    syslog (LOG_ERR, "read error 1");
  ret = read (dome_port, &stav_portu[PORT_B], 1);
  syslog (LOG_DEBUG, "A stav: %x state: %x B stav: %x state: %x", ta,
	  stav_portu[PORT_A], tb, stav_portu[PORT_B]);
  if (ret < 1)
    return -1;
  return 0;
}

void
Rts2DevDomeBart::zapni_pin (unsigned char c_port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu ();
  c = ZAPIS_NA_PORT | c_port;
  syslog (LOG_DEBUG, "port:%xh pin:%xh write: %x:", c_port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[c_port] | pin;
  syslog (LOG_DEBUG, "zapni_pin: %xh", c);
  write (dome_port, &c, 1);
}

void
Rts2DevDomeBart::vypni_pin (unsigned char c_port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu ();
  c = ZAPIS_NA_PORT | c_port;
  syslog (LOG_DEBUG, "port:%xh pin:%xh write: %x:", c_port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[c_port] & (~pin);
  syslog (LOG_DEBUG, "%xh", c);
  write (dome_port, &c, 1);
}

int
Rts2DevDomeBart::isOn (int c_port)
{
  int ret;
  ret = zjisti_stav_portu ();
  if (ret)
    return -1;
  return !(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}

#define ZAP(i) zapni_pin(adresa[i].port,adresa[i].pin)
#define VYP(i) vypni_pin(adresa[i].port,adresa[i].pin)

int
Rts2DevDomeBart::openDome ()
{
  if (!isOn (KONCAK_OTEVRENI_JIH))
    return endOpen ();
  if (!isGoodWeather ())
    return -1;
  VYP (MOTOR);
  sleep (1);
  VYP (SMER);
  sleep (1);
  ZAP (MOTOR);
  syslog (LOG_DEBUG, "oteviram strechu");
  return Rts2DevDome::openDome ();
}

long
Rts2DevDomeBart::isOpened ()
{
  int ret;
  ret = zjisti_stav_portu ();
  if (ret)
    return ret;
  if (isOn (KONCAK_OTEVRENI_JIH))
    return USEC_SEC;
  return -2;
}

int
Rts2DevDomeBart::endOpen ()
{
  VYP (MOTOR);
  zjisti_stav_portu ();		//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  setTimeout (USEC_SEC);
  return Rts2DevDome::endOpen ();
}

int
Rts2DevDomeBart::closeDome ()
{
  int motor;
  int smer;
  if (!isOn (KONCAK_ZAVRENI_JIH))
    return endClose ();
  motor = isOn (MOTOR);
  smer = isOn (SMER);
  if (motor == -1 || smer == -1)
    {
      // errror
      return -1;
    }
  if (!motor)
    {
      // closing in progress
      if (!smer)
	return 0;
      VYP (MOTOR);
      sleep (1);
    }
  ZAP (SMER);
  sleep (1);
  ZAP (MOTOR);
  syslog (LOG_DEBUG, "zaviram strechu");

  return Rts2DevDome::closeDome ();
}

long
Rts2DevDomeBart::isClosed ()
{
  int ret;
  ret = zjisti_stav_portu ();
  if (ret)
    return ret;
  if (isOn (KONCAK_ZAVRENI_JIH))
    return USEC_SEC;
  return -2;
}

int
Rts2DevDomeBart::endClose ()
{
  int motor;
  motor = isOn (MOTOR);
  if (motor == -1)
    return -1;
  if (motor)
    return Rts2DevDome::endClose ();
  VYP (MOTOR);
  sleep (1);
  VYP (SMER);
  zjisti_stav_portu ();
  return Rts2DevDome::endClose ();
}

int
Rts2DevDomeBart::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      dome_file = optarg;
      break;
    case 'R':
      rain_detector = optarg;
      break;
    case 'I':
      ignoreMeteo = 1;
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevDomeBart::isGoodWeather ()
{
  int flags;
  int ret;
  if (rain_port > 0)
    {
      ret = ioctl (rain_port, TIOCMGET, &flags);
      syslog (LOG_DEBUG,
	      "Rts2DevDomeBart::isGoodWeather flags: %08x %i rain:%i", flags,
	      flags, (flags & TIOCM_RI));
      // ioctl failed or it's raining..
      if (ret || !(flags & TIOCM_RI))
	{
	  rain = 1;
	  setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT);
	  return 0;
	}
      rain = 0;
    }
  if (weatherConn)
    return weatherConn->isGoodWeather ();
  return 1;
}

int
Rts2DevDomeBart::init ()
{
  struct termios oldtio, newtio;
  int i;

  int ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  dome_port = open (dome_file, O_RDWR | O_NOCTTY);

  if (dome_port == -1)
    {
      syslog (LOG_ERR, "Rts2DevDomeBart::init open %m");
      return -1;
    }

  ret = tcgetattr (dome_port, &oldtio);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2DevDomeBart::init tcgetattr %m");
      return -1;
    }

  newtio = oldtio;

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 1;

  tcflush (dome_port, TCIOFLUSH);
  ret = tcsetattr (dome_port, TCSANOW, &newtio);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2DevDomeBart::init tcsetattr %m");
      return -1;
    }

  if (rain_detector)
    {
      // init rain detector
      int flags;
      rain_port = open (rain_detector, O_RDWR | O_NOCTTY);
      if (rain_port == -1)
	{
	  syslog (LOG_ERR, "Rts2DevDomeBart::init cannot open %s : %m",
		  rain_detector);
	  return -1;
	}
      ret = ioctl (rain_port, TIOCMGET, &flags);
      if (ret)
	{
	  syslog (LOG_ERR, "Rts2DevDomeBart::init cannot get flags: %m");
	  return -1;
	}
      flags &= ~TIOCM_DTR;
      flags |= TIOCM_RTS;
      ret = ioctl (rain_port, TIOCMSET, &flags);
      if (ret)
	{
	  syslog (LOG_ERR, "Rts2DevDomeBart::init cannot set flags: %m");
	  return -1;
	}
    }

  if (ignoreMeteo)
    return 0;

  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  weatherConn = new Rts2ConnBartWeather (1500, this);
	  weatherConn->init ();
	  connections[i] = weatherConn;
	  break;
	}
    }
  if (i == MAX_CONN)
    {
      syslog (LOG_ERR, "no free conn for Rts2ConnBartWeather");
      return -1;
    }

  return 0;
}

int
Rts2DevDomeBart::idle ()
{
  // check for weather..
  if (isGoodWeather ())
    {
      if (((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	  && ((getState (0) & DOME_DOME_MASK) == DOME_CLOSED))
	{
	  // after centrald reply, that he switched the state, dome will
	  // open
	  domeWeatherGood ();
	}
    }
  else
    {
      int ret;
      // close dome - don't thrust centrald to be running and closing
      // it for us
      ret = closeDome ();
      if (ret == -1)
	{
	  setTimeout (10 * USEC_SEC);
	}
      setMasterStandby ();
    }
  return Rts2DevDome::idle ();
}

int
Rts2DevDomeBart::handle_zasuvky (int state)
{
  int i;
  for (i = 0; i < NUM_ZAS; i++)
    {
      int zasuvka_num = zasuvky_index[i];
      if (zasuvky_stavy[state][i] == ZAS_VYP)
	{
	  zapni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
      else
	{
	  vypni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
      sleep (1);		// doplnil Ford
    }
  return 0;
}

int
Rts2DevDomeBart::ready ()
{
  return 0;
}

int
Rts2DevDomeBart::info ()
{
  int ret;
  ret = zjisti_stav_portu ();
  if (ret)
    return -1;
  sw_state = getPortState (KONCAK_OTEVRENI_JIH);
  sw_state |= (getPortState (KONCAK_OTEVRENI_SEVER) << 1);
  sw_state |= (getPortState (KONCAK_ZAVRENI_JIH) << 2);
  sw_state |= (getPortState (KONCAK_ZAVRENI_SEVER) << 3);
  if (weatherConn)
    {
      rain |= weatherConn->getRain ();
      windspeed = weatherConn->getWindspeed ();
    }
  nextOpen = getNextOpen ();
  return 0;
}

int
Rts2DevDomeBart::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeBart::off ()
{
  closeDome ();
  handle_zasuvky (OFF);
  return 0;
}

int
Rts2DevDomeBart::standby ()
{
  handle_zasuvky (STANDBY);
  closeDome ();
  return 0;
}

int
Rts2DevDomeBart::observing ()
{
  handle_zasuvky (OBSERVING);
  openDome ();
  return 0;
}

int
Rts2DevDomeBart::changeMasterState (int new_state)
{
  observingPossible = 0;
  if ((new_state & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
    {
      switch (new_state & SERVERD_STATUS_MASK)
	{
	case SERVERD_EVENING:
	case SERVERD_MORNING:
	case SERVERD_DUSK:
	case SERVERD_NIGHT:
	case SERVERD_DAWN:
	  return standby ();
	default:
	  return off ();
	}
    }
  switch (new_state)
    {
    case SERVERD_EVENING:
    case SERVERD_NIGHT:
    case SERVERD_DUSK:
    case SERVERD_DAWN:
      return observing ();
    case SERVERD_MORNING:
      return standby ();
    default:
      return off ();
    }
}


Rts2DevDomeBart *device;

void
killSignal (int sig)
{
  if (device)
    delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevDomeBart (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize dome - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
