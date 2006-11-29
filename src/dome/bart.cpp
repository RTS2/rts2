#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <vector>

#include "dome.h"
#include "rts2connbufweather.h"

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

class Rts2DevDomeBart:public Rts2DevDome
{
private:
  int dome_port;
  int rain_port;
  char *dome_file;
  char *rain_detector;
  FILE *mrak2_log;

  unsigned char spinac[2];
  unsigned char stav_portu[3];

  Rts2ConnBufWeather *weatherConn;

  int zjisti_stav_portu ();
  void zapni_pin (unsigned char c_port, unsigned char pin);
  void vypni_pin (unsigned char c_port, unsigned char pin);
  int getPortState (int c_port)
  {
    return !!(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
  };

  int isOn (int c_port);
  int handle_zasuvky (int state);

  char *cloud_dev;
  int cloud_port;

  time_t nextCloudMeas;

  // cloud sensor functions
  int cloudHeating (char perc);
  // adjust cloud heating by air temperature.
  int cloudHeating ();
  int cloudMeasure (char angle);
  int cloudMeasureAll ();

  void checkCloud ();

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
  addOption ('c', "cloud_sensor", 1, "/dev/file for cloud sensor");
  dome_file = "/dev/ttyS0";
  rain_detector = NULL;
  rain_port = -1;

  domeModel = "BART_FORD_2";

  weatherConn = NULL;

  rain = 0;
  windspeed = nan ("f");

  cloud_dev = NULL;
  cloud_port = -1;

  nextCloudMeas = 0;

  // oteviram file pro mrakomer2_log...
  mrak2_log = fopen ("/var/log/mrakomer2", "a");
}

Rts2DevDomeBart::~Rts2DevDomeBart (void)
{
  close (rain_port);
  close (dome_port);
  fclose (mrak2_log);
}

int
Rts2DevDomeBart::zjisti_stav_portu ()
{
  unsigned char ta, tb, c = STAV_PORTU | PORT_A;
  int ret;
  write (dome_port, &c, 1);
  if (read (dome_port, &ta, 1) < 1)
    logStream (MESSAGE_ERROR) << "read error 0" << sendLog;
  read (dome_port, &stav_portu[PORT_A], 1);
  c = STAV_PORTU | PORT_B;
  write (dome_port, &c, 1);
  if (read (dome_port, &tb, 1) < 1)
    logStream (MESSAGE_ERROR) << "read error 1" << sendLog;
  ret = read (dome_port, &stav_portu[PORT_B], 1);
  logStream (MESSAGE_DEBUG) << "A stav:" << ta << " state:" <<
    stav_portu[PORT_A] << " B stav: " << tb << " state: " <<
    stav_portu[PORT_B] << sendLog;
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
  logStream (MESSAGE_DEBUG) << "port:" << c_port << " pin:" << pin <<
    " write:" << c << sendLog;
  write (dome_port, &c, 1);
  c = stav_portu[c_port] | pin;
  logStream (MESSAGE_DEBUG) << "zapni_pin: " << c << sendLog;
  write (dome_port, &c, 1);
}

void
Rts2DevDomeBart::vypni_pin (unsigned char c_port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu ();
  c = ZAPIS_NA_PORT | c_port;
  logStream (MESSAGE_DEBUG) << "port:" << c_port << " pin:" << pin <<
    " write:" << c << sendLog;
  write (dome_port, &c, 1);
  c = stav_portu[c_port] & (~pin);
  logStream (MESSAGE_DEBUG) << c << sendLog;
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
  logStream (MESSAGE_DEBUG) << "oteviram strechu" << sendLog;
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
  logStream (MESSAGE_DEBUG) << "zaviram strechu" << sendLog;

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
    case 'c':
      cloud_dev = optarg;
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
      logStream (MESSAGE_DEBUG) <<
	"Rts2DevDomeBart::isGoodWeather flags: " << flags << " rain: " <<
	(flags & TIOCM_RI) << sendLog;
      // ioctl failed or it's raining..
      if (ret || !(flags & TIOCM_RI))
	{
	  rain = 1;
	  setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT);
	  if (ignoreMeteo)
	    return 1;
	  return 0;
	}
      rain = 0;
    }
  if (ignoreMeteo)
    return 1;
  if (weatherConn)
    return weatherConn->isGoodWeather ();
  return 0;
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
      logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init open " <<
	strerror (errno) << sendLog;
      return -1;
    }

  ret = tcgetattr (dome_port, &oldtio);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init tcgetattr " <<
	strerror (errno) << sendLog;
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
      logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init tcsetattr " <<
	strerror (errno) << sendLog;
      return -1;
    }

  if (rain_detector)
    {
      // init rain detector
      int flags;
      rain_port = open (rain_detector, O_RDWR | O_NOCTTY);
      if (rain_port == -1)
	{
	  logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init cannot open " <<
	    rain_detector << " " << strerror (errno) << sendLog;
	  return -1;
	}
      ret = ioctl (rain_port, TIOCMGET, &flags);
      if (ret)
	{
	  logStream (MESSAGE_ERROR) <<
	    "Rts2DevDomeBart::init cannot get flags: " << strerror (errno) <<
	    sendLog;
	  return -1;
	}
      flags &= ~TIOCM_DTR;
      flags |= TIOCM_RTS;
      ret = ioctl (rain_port, TIOCMSET, &flags);
      if (ret)
	{
	  logStream (MESSAGE_ERROR) <<
	    "Rts2DevDomeBart::init cannot set flags: " << strerror (errno) <<
	    sendLog;
	  return -1;
	}
    }

  if (cloud_dev)
    {
      cloud_port = open (cloud_dev, O_RDWR | O_NOCTTY);
      if (cloud_port == -1)
	{
	  logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::init cannot open " <<
	    cloud_dev << " " << strerror (errno) << sendLog;
	  return -1;
	}
      // setup values..
      newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
      newtio.c_iflag = IGNPAR;
      newtio.c_oflag = 0;
      newtio.c_lflag = 0;
      newtio.c_cc[VMIN] = 0;
      newtio.c_cc[VTIME] = 1;

      tcflush (cloud_port, TCIOFLUSH);
      ret = tcsetattr (cloud_port, TCSANOW, &newtio);
      if (ret < 0)
	{
	  logStream (MESSAGE_ERROR) <<
	    "Rts2DevDomeBart::init cloud tcsetattr: " << strerror (errno) <<
	    sendLog;
	  return -1;
	}
    }

  if (ignoreMeteo)
    return 0;

  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  weatherConn =
	    new Rts2ConnBufWeather (1500, BART_WEATHER_TIMEOUT,
				    BART_CONN_TIMEOUT,
				    BART_BAD_WEATHER_TIMEOUT,
				    BART_BAD_WINDSPEED_TIMEOUT, this);
	  weatherConn->init ();
	  connections[i] = weatherConn;
	  break;
	}
    }
  if (i == MAX_CONN)
    {
      logStream (MESSAGE_ERROR) << "no free conn for Rts2ConnBufWeather" <<
	sendLog;
      return -1;
    }

  return 0;
}

int
Rts2DevDomeBart::idle ()
{
  // check for weather..
  checkCloud ();
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
Rts2DevDomeBart::cloudHeating (char perc)
{
  int ret;
  char buf[35];
  ret = read (cloud_port, buf, 34);	// "flush"
  ret = write (cloud_port, &perc, 1);
  if (ret != 1)
    return -1;
  sleep (1);
  ret = read (cloud_port, buf, 14);
  if (ret <= 0)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::cloudHeating read: " <<
	strerror (errno) << " ret: " << ret << sendLog;
      return -1;
    }
  buf[ret] = '\0';
  logStream (MESSAGE_DEBUG) << "Rts2DevDomeBart::cloudHeating read: " << buf
    << sendLog;
  return 0;
}

int
Rts2DevDomeBart::cloudHeating ()
{
  char step = 'b';
  if (temperature > 5)
    return 0;
  step += (char) ((-temperature + 5) / 4.0);
  if (step > 'k')
    step = 'k';
  return cloudHeating (step);
}

int
Rts2DevDomeBart::cloudMeasure (char angle)
{
  int ret;
  char buf[35];
  int ang, ground, space;
  ret = read (cloud_port, buf, 34);	// "flush"
  ret = write (cloud_port, &angle, 1);
  if (ret != 1)
    return -1;
  sleep (4);
  ret = read (cloud_port, buf, 20);
  if (ret <= 0)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::cloudMeasure read: " <<
	strerror (errno) << "ret:" << ret << sendLog;
      return -1;
    }
  buf[ret] = '\0';
  // now parse readed values
  // A 1;G 18;S 18
  ret = sscanf (buf, "A %i;G %i; S %i", &ang, &ground, &space);
  if (ret != 3)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeBart::cloudMeasure invalid cloud sensor return: " << buf
	<< sendLog;
      return -1;
    }
  logStream (MESSAGE_DEBUG) <<
    "Rts2DevDomeBart::cloudMeasure angle: " << ang << " ground: " << ground <<
    " space: " << space << sendLog;
  return 0;
}

// posle 
int
Rts2DevDomeBart::cloudMeasureAll ()
{
  time_t now;

  int ret;
  char buf[35];
  char buf_m = 'm';
  int ground, s45, s90, s135;
  ret = read (cloud_port, buf, 34);
  ret = write (cloud_port, &buf_m, 1);
  if (ret != 1)
    return -1;
  sleep (4);
  ret = read (cloud_port, buf, 34);
  if (ret <= 0)
    {
      logStream (MESSAGE_ERROR) << "Rts2DevDomeBart::cloudMeasure read: " <<
	strerror (errno) << " ret: " << ret << sendLog;
      return -1;
    }
  buf[ret] = '\0';
  // now parse readed values
  // G 0;S45 -31;S90 -38;S135 -36
  ret =
    sscanf (buf, "G %i;S45 %i;S90 %i;S135 %i", &ground, &s45, &s90, &s135);
  if (ret != 4)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2DevDomeBart::cloudMeasure invalid cloud sensor return: " << buf
	<< sendLog;
      return -1;
    }
  logStream (MESSAGE_DEBUG) <<
    "Rts2DevDomeBart::cloudMeasure ground: " << ground << " S45: " << s45 <<
    " S90: " << s90 << " S135: " << s135 << sendLog;
  time (&now);
  fprintf (mrak2_log,
	   "%li - G %i;S45 %i;S90 %i;S135 %i;Temp %.1f;Hum %.0f;Rain %i\n",
	   (long int) now, ground, s45, s90, s135, temperature, humidity,
	   rain);
  setCloud (s90 - ground);
  fflush (mrak2_log);
  return 0;
}

void
Rts2DevDomeBart::checkCloud ()
{
  time_t now;
  if (cloud_port < 0)
    return;
  time (&now);
  if (now < nextCloudMeas)
    return;

  if (rain)
    {
      fprintf (mrak2_log,
	       "%li - G nan;S45 nan;S90 nan;S135 nan;Temp %.1f;Hum %.0f;Rain %i\n",
	       (long int) now, temperature, humidity, rain);
      fflush (mrak2_log);
      nextCloudMeas = now + 300;
      return;
    }

  // check that master is in right state..
  switch (getMasterState () & ~SERVERD_STANDBY_MASK)
    {
    case SERVERD_EVENING:
    case SERVERD_DUSK:
    case SERVERD_NIGHT:
    case SERVERD_DAWN:
      cloudHeating ();
      cloudMeasureAll ();
      nextCloudMeas = now + 60;	// TODO doresit dopeni kazdych 10 sec
      break;
    default:
      cloudHeating ();
      cloudMeasureAll ();
      // 5 minutes mesasurements during the day phase
      nextCloudMeas = now + 300;
    }
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
  return Rts2DevDome::info ();
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

int
main (int argc, char **argv)
{
  Rts2DevDomeBart *device = new Rts2DevDomeBart (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}
