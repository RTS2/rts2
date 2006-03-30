/*! 
 * @file Dome control deamon for Dublin dome.
 *
 * $Id$
 *
 * @author john
 */
#include <signal.h>

#include <sys/io.h>
#include <asm/io.h>

#include "dome.h"
#include "rts2connbufweather.h"

#define	ROOF_TIMEOUT	300	// in seconds

#define OPEN		2
#define CLOSE		4

#define OPENING		2
#define CLOSING		0

#define WATCHER_DOME_OPEN	1
#define WATCHER_DOME_CLOSED	0
#define WATCHER_DOME_UNKNOWN	-1

#define GOOD		0
#define BAD		-1

#define WATCHER_METEO_TIMEOUT	80

#define WATCHER_BAD_WEATHER_TIMEOUT	3600
#define WATCHER_BAD_WINDSPEED_TIMEOUT	360
#define WATCHER_CONN_TIMEOUT		360

#define BASE 0xde00

class Rts2DevDomeDublin:public Rts2DevDome
{
private:
  int dome_state;
  int ignoreMeteo;

  Rts2ConnBufWeather *weatherConn;
  bool isMoving ();
protected:
    virtual int processOption (int in_opt);
  virtual int isGoodWeather ();

public:
    Rts2DevDomeDublin (int argc, char **argv);
    virtual ~ Rts2DevDomeDublin (void);
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
};

Rts2DevDomeDublin::Rts2DevDomeDublin (int in_argc, char **in_argv):
Rts2DevDome (in_argc, in_argv)
{
  domeModel = "DUBLIN_DOME";
  addOption ('I', "ignore_meteo", 0, "whenever to ignore meteo station");
  ignoreMeteo = 0;

  weatherConn = NULL;
}

Rts2DevDomeDublin::~Rts2DevDomeDublin (void)
{
  outb (0, BASE);
// SWITCH OFF INTERFACE
  outb (0, BASE + 1);
}

int
Rts2DevDomeDublin::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      ignoreMeteo = 1;
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevDomeDublin::isGoodWeather ()
{
  if (weatherConn)
    return weatherConn->isGoodWeather ();
  return 1;
}

int
Rts2DevDomeDublin::init ()
{
  int ret, i;
  ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  dome_state = WATCHER_DOME_UNKNOWN;

  ioperm (BASE, 4, 1);

// SET CONTROL WORD
  outb (137, BASE + 3);


// INITIALIZE ALL PORTS TO 0

  for (i = 0; i <= 2; i++)
    {
      outb (0, BASE + i);
    }

// SWITCH ON INTERFACE
  outb (1, BASE + 1);

  if (ignoreMeteo)
    return 0;

  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  weatherConn =
	    new Rts2ConnBufWeather (5002, WATCHER_METEO_TIMEOUT,
				    WATCHER_CONN_TIMEOUT,
				    WATCHER_BAD_WEATHER_TIMEOUT,
				    WATCHER_BAD_WINDSPEED_TIMEOUT, this);
	  weatherConn->init ();
	  connections[i] = weatherConn;
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

int
Rts2DevDomeDublin::idle ()
{
  // check for weather..
  if (isGoodWeather ())
    {
      if (((getMasterState () & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	  && ((getState (0) & DOME_DOME_MASK) == DOME_CLOSED))
	{
	  // after centrald reply, that he switched the state, dome will
	  // open
	  sendMaster ("on");
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
Rts2DevDomeDublin::ready ()
{
  return 0;
}

int
Rts2DevDomeDublin::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeDublin::info ()
{
  sw_state = 1;

  if (weatherConn)
    {
      rain = weatherConn->getRain ();
      windspeed = weatherConn->getWindspeed ();
    }
  nextOpen = getNextOpen ();

  return 0;
}

bool Rts2DevDomeDublin::isMoving ()
{
  int
    result;
  int
    moving = 0;
  int
    count;
  for (count = 0; count < 5; count++)
    {
      result = (inb (BASE + 2));
      // we think it's moving
      if (result & 2)
	moving++;
    }
  // motor is moving when we get moving at more then half instances of read
  if (count < moving * 2)
    return true;
  return false;
}

int
Rts2DevDomeDublin::openDome ()
{
  if (!isGoodWeather ())
    return -1;
  if (isMoving () || dome_state == WATCHER_DOME_OPEN)
    return 0;

  outb (OPEN, BASE);

  sleep (1);
  outb (0, BASE);

  return Rts2DevDome::openDome ();
}

long
Rts2DevDomeDublin::isOpened ()
{
  return (isMoving ()? USEC_SEC : -2);
}

int
Rts2DevDomeDublin::endOpen ()
{
  dome_state = WATCHER_DOME_OPEN;
  return Rts2DevDome::endOpen ();
}

int
Rts2DevDomeDublin::closeDome ()
{
  // we cannot close dome when we are still moving
  if (isMoving () || dome_state == WATCHER_DOME_CLOSED)
    return 0;

  outb (CLOSE, BASE);

  sleep (1);
  outb (0, BASE);

  return Rts2DevDome::closeDome ();
}

long
Rts2DevDomeDublin::isClosed ()
{
  return (isMoving ()? USEC_SEC : -2);
}

int
Rts2DevDomeDublin::endClose ()
{
  dome_state = WATCHER_DOME_CLOSED;
  return Rts2DevDome::endClose ();
}

Rts2DevDomeDublin *device;

void
switchoffSignal (int sig)
{
  syslog (LOG_DEBUG, "switchoffSignal signale: %i", sig);
  delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevDomeDublin (argc, argv);

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
