/*! 
 * @file Dome control deamon for Dublin dome.
 *
 * $Id$
 *
 * @author john
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "dpci8255.h"
#include "dpci8255lib.c"

#include "dome.h"
#include "udpweather.h"

#define OPEN		1
#define CLOSE		2

class Rts2DevDomeDublin:public Rts2DevDome
{
private:
  int dome_port;
  char *dome_file;

  Rts2ConnFramWeather *weatherConn;

public:
    Rts2DevDomeDublin (int argc, char **argv);
    virtual ~ Rts2DevDomeDublin (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int idle ();

  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();

  virtual int openDome ();
/* That should be implemented, using some sensors on dome
  virtual long isOpened (); 
  virtual int endOpen (); */
  virtual int closeDome ();
/* That should be implemented, using some sensors on dome
  virtual long isClosed ();
  virtual int endClose ();*/
}

Rts2DevDomeDublin::Rts2DevDomeDublin (int argc, char **argv):
Rts2DevDome (argc, argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");

  dome_file = "/dev/ttyS0";
  domeModel = "DUBLIN_DOME";

  weatherConn = NULL;
}

Rts2DevDomeDublin::~Rts2DevDomeDublin (void)
{
  close (dome_port);
}

int
Rts2DevDomeFram::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      dome_file = optarg;
      break;
    default:
      return Rts2DevDome::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevDomeDublin::init ()
{
  int ret;
  ret = Rts2DevDome::init ();
  if (ret)
    return ret;

  dome_port = open (dome_file, O_RDONLY);

  if (dome_port == -1)
    return -1;

  WritePci8255ControlWord (dome_port, CONTROLWORD1, 0x80);
  WritePci8255ControlWord (dome_port, CONTROLWORD2, 0x80);
  WritePci8255Port (dome_port, PORTA1, 0);
  WritePci8255Port (dome_port, PORTA2, 0);

  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  weatherConn = new Rts2ConnFramWeather (5002, this);
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
  if (weatherConn->isGoodWeather ())
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

  return 0;
}

int
Rts2DevDomeDublin::closeDome ()
{
  WritePci8255Port (dome_port, PORTA2, CLOSE);
  sleep (1);
  WritePci8255Port (dome_port, PORTA2, 0);

  return Rts2DevDome::closeDome ();

// ADD SOME STUFF ABOUT CAMERA/TELESCOPE POWER HERE?
/*   set_pin(CLOSE);
   clear_pin(OPEN);
   d_info.dome = 0;
*/
  return 0;
}

int
Rts2DevDomeDublin::openDome ()
{
  if (!weatherConn->isGoodWeather ())
    return -1;

  WritePci8255Port (dome_port, PORTA2, OPEN);
  sleep (1);
  WritePci8255Port (dome_port, PORTA2, 0);
// ADD SOME STUFF ABOUT CAMERA/TELESCOPE POWER HERE?
/*  set_pin(OPEN);
  clear_pin(CLOSE);
  d_info.dome = 1;*/
  return Rts2DevDome::openDome ();
}
