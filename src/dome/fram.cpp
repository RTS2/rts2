#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <mcheck.h>

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#include "dome.h"

#define BAUDRATE B9600
#define ZAPIS_NA_PORT 4
#define STAV_PORTU 0
#define CHYBA_NEZNAMY_PRIKAZ 0x80
#define CHYBA_PRETECENI_TX_BUFFERU 0x81
#define PORT_A 0
#define PORT_B 1
#define PORT_C 2

// check time in usec (1000000 ms = 1s)
#define FRAM_CHECK_TIMEOUT 1000

// following times are in seconds!
#define FRAM_TIME_OPEN_RIGHT	26
#define FRAM_TIME_OPEN_LEFT	24
#define FRAM_TIME_CLOSE_RIGHT	30
#define FRAM_TIME_CLOSE_LEFT	25

typedef enum
{ VENTIL_AKTIVACNI,
  VENTIL_OTEVIRANI_PRAVY,
  VENTIL_OTEVIRANI_LEVY,
  VENTIL_ZAVIRANI_PRAVY,
  VENTIL_ZAVIRANI_LEVY,
  KOMPRESOR,
  ZASUVKA_PRAVA,
  ZASUVKA_LEVA,
  KONCAK_OTEVRENI_PRAVY,
  KONCAK_OTEVRENI_LEVY,
  KONCAK_ZAVRENI_PRAVY,
  KONCAK_ZAVRENI_LEVY
}
vystupy;

struct typ_a
{
  unsigned char port;
  unsigned char pin;
} adresa[] =
{
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
  PORT_A, 1},
  {
  PORT_A, 2},
  {
  PORT_A, 4},
  {
  PORT_B, 16},
  {
  PORT_B, 32},
  {
  PORT_B, 64},
  {
  PORT_B, 128}
};

#define NUM_ZAS		3

#define OFF		0
#define STANDBY		1
#define OBSERVING	2

// prvni zasuvka dalekohled, druha kamery, treti chlazeni
int zasuvky_index[NUM_ZAS] = { 0, 1, 2 };

enum stavy
{ ZAS_VYP, ZAS_ZAP };

// prvni index - cislo stavu (0 - off, 1 - standby, 2 - observing), druhy je stav zasuvky cislo j)
enum stavy zasuvky_stavy[3][NUM_ZAS] =
{
  // off
  {ZAS_VYP, ZAS_VYP, ZAS_VYP},
  // standby
  {ZAS_ZAP, ZAS_ZAP, ZAS_VYP},
  // observnig
  {ZAS_ZAP, ZAS_ZAP, ZAS_ZAP}
};

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

class Rts2ConnFramWeather:public Rts2Conn
{
private:
  Rts2DevDome * master;
  int weather_port;

protected:

public:
    Rts2ConnFramWeather (int in_weather_port, Rts2DevDome * in_master);
  virtual int init ();
  virtual int receive (fd_set * set);
};

Rts2ConnFramWeather::Rts2ConnFramWeather (int in_weather_port,
					  Rts2DevDome * in_master):
Rts2Conn (in_master)
{
  master = in_master;
  weather_port = in_weather_port;
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
  char buf[20];
  int data_size = 0;
  if (sock >= 0 && FD_ISSET (sock, set))
    {
      data_size = read (sock, buf, 3);
      syslog (LOG_DEBUG, "readed: %i %s", data_size, buf);
      if (buf[1] == '8')
	{
	  master->closeDome ();
	  master->sendMaster ("standby");
	}
      if (buf[1] == '1')
	{
	  master->openDome ();
	  master->sendMaster ("on");
	}
    }
  return data_size;
}

/***********************************************************************************************
 * 
 * Class for dome controller.
 *
 **********************************************************************************************/

class Rts2DevDomeFram:public Rts2DevDome
{
private:
  int dome_port;
  char *dome_file;

  unsigned char spinac[2];
  unsigned char stav_portu[3];

  int zjisti_stav_portu (unsigned char *stav);
  void zapni_pin (unsigned char c_port, unsigned char pin);
  void vypni_pin (unsigned char c_port, unsigned char pin);
  int isOn (int c_port);
  int handle_zasuvky (int state);

  int openLeft ();
  int openRight ();

  int closeRight ();
  int closeLeft ();

  int stopMove ();

  time_t timeoutEnd;

  void setMotorTimeout (time_t timeout);

  /** 
   * Check if timeout was exceeded.
   *
   * @return 0 when timeout wasn't reached, 1 when timeout was
   * exceeded.
   */

  int checkMotorTimeout ();

  enum
  { MOVE_NONE, MOVE_OPEN_LEFT, MOVE_OPEN_LEFT_WAIT, MOVE_OPEN_RIGHT,
    MOVE_OPEN_RIGHT_WAIT,
    MOVE_CLOSE_RIGHT, MOVE_CLOSE_RIGHT_WAIT, MOVE_CLOSE_LEFT,
    MOVE_CLOSE_LEFT_WAIT
  } movingState;
public:
    Rts2DevDomeFram (int argc, char **argv);
    virtual ~ Rts2DevDomeFram (void);
  virtual int init ();

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
};

int
Rts2DevDomeFram::zjisti_stav_portu (unsigned char *stav)
{
  unsigned char t, c = STAV_PORTU | PORT_A;
  write (dome_port, &c, 1);
  if (read (dome_port, &t, 1) < 1)
    syslog (LOG_ERR, "read error 0");
  else
    syslog (LOG_DEBUG, "stav: A: %x:", t);
  read (dome_port, &stav[PORT_A], 1);
  syslog (LOG_DEBUG, "A state: %x", stav[PORT_A]);
  c = STAV_PORTU | PORT_B;
  write (dome_port, &c, 1);
  if (read (dome_port, &t, 1) < 1)
    syslog (LOG_ERR, "read error 1");
  else
    syslog (LOG_DEBUG, " B: %x:", t);
  read (dome_port, &stav[PORT_B], 1);
  syslog (LOG_DEBUG, "B state: %x", stav[PORT_B]);
  return 0;
}

void
Rts2DevDomeFram::zapni_pin (unsigned char port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu (stav_portu);
  c = ZAPIS_NA_PORT | port;
  syslog (LOG_DEBUG, "port:%xh pin:%xh write: %x:", port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[port] | pin;
  syslog (LOG_DEBUG, "zapni_pin: %xh", c);
  write (dome_port, &c, 1);
}

void
Rts2DevDomeFram::vypni_pin (unsigned char port, unsigned char pin)
{
  unsigned char c;
  zjisti_stav_portu (stav_portu);
  c = ZAPIS_NA_PORT | port;
  syslog (LOG_DEBUG, "port:%xh pin:%xh write: %x:", port, pin, c);
  write (dome_port, &c, 1);
  c = stav_portu[port] & (~pin);
  syslog (LOG_DEBUG, "%xh", c);
  write (dome_port, &c, 1);
}

int
Rts2DevDomeFram::isOn (int c_port)
{
  zjisti_stav_portu (stav_portu);
  return !(stav_portu[adresa[c_port].port] & adresa[c_port].pin);
}

void
Rts2DevDomeFram::setMotorTimeout (time_t timeout)
{
  time_t now;
  time (&now);
  timeoutEnd = now + timeout;
}

int
Rts2DevDomeFram::checkMotorTimeout ()
{
  time_t now;
  time (&now);
  if (now >= timeoutEnd)
    syslog (LOG_DEBUG, "timeout reached: %i state: %i", now - timeoutEnd,
	    movingState);
  return (now >= timeoutEnd);
}

#define ZAP(i) zapni_pin(adresa[i].port,adresa[i].pin)
#define VYP(i) vypni_pin(adresa[i].port,adresa[i].pin)

int
Rts2DevDomeFram::openLeft ()
{
  movingState = MOVE_OPEN_LEFT;
  ZAP (KOMPRESOR);
  sleep (1);
  syslog (LOG_DEBUG, "opening left door");
  ZAP (VENTIL_OTEVIRANI_LEVY);
  ZAP (VENTIL_AKTIVACNI);
  movingState = MOVE_OPEN_LEFT_WAIT;
  setMotorTimeout (FRAM_TIME_OPEN_LEFT);
  return 0;
}

int
Rts2DevDomeFram::openRight ()
{
  // otevri pravou strechu
  movingState = MOVE_OPEN_RIGHT;
  VYP (VENTIL_AKTIVACNI);
  VYP (VENTIL_OTEVIRANI_LEVY);
  ZAP (KOMPRESOR);
  sleep (1);
  syslog (LOG_DEBUG, "opening right door");
  ZAP (VENTIL_OTEVIRANI_PRAVY);
  ZAP (VENTIL_AKTIVACNI);
  movingState = MOVE_OPEN_RIGHT_WAIT;
  setMotorTimeout (FRAM_TIME_OPEN_RIGHT);
  return 0;
}

int
Rts2DevDomeFram::closeRight ()
{
  ZAP (KOMPRESOR);
  sleep (1);
  syslog (LOG_DEBUG, "closing right door");
  ZAP (VENTIL_ZAVIRANI_PRAVY);
  ZAP (VENTIL_AKTIVACNI);
  movingState = MOVE_CLOSE_RIGHT_WAIT;
  setMotorTimeout (FRAM_TIME_CLOSE_RIGHT);
  return 0;
}

int
Rts2DevDomeFram::closeLeft ()
{
  VYP (VENTIL_AKTIVACNI);
  VYP (VENTIL_ZAVIRANI_PRAVY);
  ZAP (KOMPRESOR);
  sleep (1);
  syslog (LOG_DEBUG, "closing left door");
  ZAP (VENTIL_ZAVIRANI_LEVY);
  ZAP (VENTIL_AKTIVACNI);
  movingState = MOVE_CLOSE_LEFT_WAIT;
  setMotorTimeout (FRAM_TIME_CLOSE_LEFT);
  return 0;
}

int
Rts2DevDomeFram::stopMove ()
{
  vypni_pin (adresa[VENTIL_AKTIVACNI].port,
	     adresa[VENTIL_AKTIVACNI].pin | adresa[KOMPRESOR].pin);
  movingState = MOVE_NONE;
  return 0;
}

int
Rts2DevDomeFram::openDome ()
{
  if (movingState != MOVE_NONE)
    return -1;
  if (isOn (KONCAK_OTEVRENI_LEVY) && isOn (KONCAK_OTEVRENI_LEVY))
    {
      return endOpen ();
    }

  vypni_pin (adresa[VENTIL_OTEVIRANI_PRAVY].port,
	     adresa[VENTIL_OTEVIRANI_PRAVY].pin
	     | adresa[VENTIL_ZAVIRANI_PRAVY].pin
	     | adresa[VENTIL_ZAVIRANI_LEVY].pin);

  if (!isOn (KONCAK_OTEVRENI_LEVY))
    {
      // otevri levou strechu
      openLeft ();
    }
  else				// like !isOn (KONCAK_OTEVRENI_PRAVY - see && at begining
    {
      openRight ();
    }

  setTimeout (10);
  return Rts2DevDome::openDome ();
}

long
Rts2DevDomeFram::isOpened ()
{
  syslog (LOG_DEBUG, "isOpened %i", movingState);
  switch (movingState)
    {
    case MOVE_OPEN_LEFT_WAIT:
      // check for timeout..
      if (!(isOn (KONCAK_OTEVRENI_LEVY) || checkMotorTimeout ()))
	// go to end return..
	break;
      // follow on..
    case MOVE_OPEN_RIGHT:
      openRight ();
      break;
    case MOVE_OPEN_RIGHT_WAIT:
      if (!(isOn (KONCAK_OTEVRENI_PRAVY) || checkMotorTimeout ()))
	break;
    default:
      return -2;
    }
  return FRAM_CHECK_TIMEOUT;
}

int
Rts2DevDomeFram::endOpen ()
{
  stopMove ();
  VYP (VENTIL_OTEVIRANI_PRAVY);
  VYP (VENTIL_OTEVIRANI_LEVY);
  return Rts2DevDome::endOpen ();
}

int
Rts2DevDomeFram::closeDome ()
{
  if (movingState != MOVE_NONE)
    {
      stopMove ();
    }
  if (isOn (KONCAK_ZAVRENI_PRAVY) && isOn (KONCAK_ZAVRENI_LEVY))
    return endClose ();

  vypni_pin (adresa[VENTIL_OTEVIRANI_PRAVY].port,
	     adresa[VENTIL_OTEVIRANI_PRAVY].pin
	     | adresa[VENTIL_ZAVIRANI_LEVY].pin
	     | adresa[VENTIL_OTEVIRANI_LEVY].pin);
  if (!isOn (KONCAK_ZAVRENI_PRAVY))
    {
      closeRight ();
    }
  else
    {
      closeLeft ();
    }
  return Rts2DevDome::closeDome ();
}

long
Rts2DevDomeFram::isClosed ()
{
  syslog (LOG_DEBUG, "isClosed %i", movingState);
  switch (movingState)
    {
    case MOVE_CLOSE_RIGHT_WAIT:
      if (!(isOn (KONCAK_ZAVRENI_PRAVY) || checkMotorTimeout ()))
	break;
      // close dome..
    case MOVE_CLOSE_LEFT:
      closeLeft ();
      break;
    case MOVE_CLOSE_LEFT_WAIT:
      if (!(isOn (KONCAK_ZAVRENI_LEVY) || checkMotorTimeout ()))
	break;
    default:
      return -2;
    }
  return FRAM_CHECK_TIMEOUT;
}

int
Rts2DevDomeFram::endClose ()
{
  stopMove ();
  VYP (VENTIL_ZAVIRANI_LEVY);
  VYP (VENTIL_ZAVIRANI_PRAVY);
  zjisti_stav_portu (stav_portu);	//kdyz se to vynecha, neposle to posledni prikaz nebo znak
  return Rts2DevDome::endClose ();
}

int
Rts2DevDomeFram::init ()
{
  struct termios oldtio, newtio;
  int i;

  int ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  dome_port = open (dome_file, O_RDWR | O_NOCTTY);
  if (dome_port == -1)
    return -1;

  tcgetattr (dome_port, &oldtio);

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VMIN] = 0;
  newtio.c_cc[VTIME] = 1;

  tcflush (dome_port, TCIOFLUSH);
  tcsetattr (dome_port, TCSANOW, &newtio);

  vypni_pin (adresa[VENTIL_AKTIVACNI].port,
	     adresa[VENTIL_AKTIVACNI].pin | adresa[KOMPRESOR].pin);

  movingState = MOVE_NONE;

  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  connections[i] = new Rts2ConnFramWeather (5007, this);
	  connections[i]->init ();
	  break;
	}
    }
  if (i == MAX_CONN)
    {
      syslog (LOG_ERR, "no free conn for Rts2ConnFramWeather");
      return -1;
    }

  return 0;
}

Rts2DevDomeFram::Rts2DevDomeFram (int argc, char **argv):Rts2DevDome (argc,
	     argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");
  dome_file = "/dev/ttyS0";

  movingState = MOVE_NONE;
}

Rts2DevDomeFram::~Rts2DevDomeFram (void)
{
  close (dome_port);
}

int
Rts2DevDomeFram::handle_zasuvky (int state)
{
  int i;
  for (i = 0; i < NUM_ZAS; i++)
    {
      int zasuvka_num = zasuvky_index[i];
      if (zasuvky_stavy[state][i] == ZAS_VYP)
	{
	  vypni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
      else
	{
	  zapni_pin (adresa[zasuvka_num].port, adresa[zasuvka_num].pin);
	}
    }
  return 0;
}

int
Rts2DevDomeFram::ready ()
{
  return 0;
}

int
Rts2DevDomeFram::info ()
{
  return 0;
}

int
Rts2DevDomeFram::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeFram::off ()
{
  closeDome ();
  handle_zasuvky (OFF);
  return 0;
}

int
Rts2DevDomeFram::standby ()
{
  handle_zasuvky (STANDBY);
  closeDome ();
  return 0;
}

int
Rts2DevDomeFram::observing ()
{
  handle_zasuvky (OBSERVING);
  openDome ();
  return 0;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevDomeFram *device = new Rts2DevDomeFram (argc, argv);

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
