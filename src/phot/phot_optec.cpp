/*! 
 * @file Photometer deamon. 
 *
 * @author petr
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define FILTER_STEP  33

#include "phot.h"
#include "../utils/rts2device.h"
#include "kernel/phot.h"

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>

class Rts2DevPhotOptec:public Rts2DevPhot
{
private:
  char *phot_dev;
  int fd;

  int phot_command (char command, short arg);

protected:
    virtual int startIntegrate ();

  virtual int endIntegrate ();
public:
    Rts2DevPhotOptec (int argc, char **argv);
    virtual ~ Rts2DevPhotOptec (void);

  virtual int processOption (int in_opt);
  virtual int init ();

  virtual int idle ();

  virtual int ready ()
  {
    return 0;
  };
  virtual int baseInfo ()
  {
    return 0;
  };
  virtual int info ()
  {
    return 0;
  };

  virtual int homeFilter ();
  virtual int moveFilter (int new_filter);
  virtual int stopIntegrate ();
  virtual int enableMove ();
  virtual int disableMove ();
};

int
Rts2DevPhotOptec::phot_command (char command, short arg)
{
  char cmd_buf[3];
  int ret;
  cmd_buf[0] = command;
  *((short *) (&(cmd_buf[1]))) = arg;
  ret = write (fd, cmd_buf, 3);
  if (ret == 3)
    return 0;
  return -1;
}

Rts2DevPhotOptec::Rts2DevPhotOptec (int argc, char **argv):Rts2DevPhot (argc,
	     argv)
{
  addOption ('f', "phot_file", 1, "photometer file (default to /dev/phot0)");
  phot_dev = "/dev/phot0";
  fd = -1;
}

Rts2DevPhotOptec::~Rts2DevPhotOptec (void)
{
  close (fd);
}

int
Rts2DevPhotOptec::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      phot_dev = optarg;
      break;
    default:
      return Rts2Device::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevPhotOptec::init ()
{
  int ret;
  ret = Rts2DevPhot::init ();
  if (ret)
    return ret;

  fd = open (phot_dev, O_RDWR | O_NONBLOCK);
  if (fd == -1)
    {
      perror ("opening photometr");
      return -1;
    }
  // reset occurs on open of photometer file..
  return 0;
}

int
Rts2DevPhotOptec::idle ()
{
  if (getState (0) & PHOT_MASK_INTEGRATE == PHOT_INTEGRATE)
    {
      if (req_count > 0)
	{
	  int ret;
	  unsigned short result[2];
	  ret = read (fd, &result[0], 2);
	  result[1] = 0;
	  ret = read (fd, &result[1], 2);
	  if (!integrateConn)
	    return endIntegrate ();
	  if (ret == 2)
	    {
	      switch (result[0])
		{
		case 'A':
		case 'B':
		  req_count--;
		  if (result[0] == 'B')
		    sendCount (result[1], (short) (req_time * 1000.0), 1);
		  else
		    sendCount (result[1], (short) (req_time * 1000.0), 0);
		  setTimeout ((long) req_time * USEC_SEC);
		  break;
		case '0':
		  filter = result[1] / FILTER_STEP;
		  integrateConn->sendValue ("filter", filter);
		  setTimeout (1000);
		  break;
		case '-':
		  sendCount (0, 0, 1);
		  endIntegrate ();
		  break;
		}
	    }
	}
      else
	{
	  // wait for data in quick mode
	  setTimeout (1000);
	}
      if (req_count <= 0)
	{
	  setTimeout (USEC_SEC);
	  return endIntegrate ();
	}
    }
  return Rts2DevPhot::idle ();
}

int
Rts2DevPhotOptec::homeFilter ()
{
  return phot_command (PHOT_CMD_RESET, 0);
}

int
Rts2DevPhotOptec::startIntegrate ()
{
  phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
  return phot_command (PHOT_CMD_INTEGRATE, (short) req_time * 1000);
}

int
Rts2DevPhotOptec::endIntegrate ()
{
  req_count = 0;
  phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
  return Rts2DevPhot::endIntegrate ();
}

int
Rts2DevPhotOptec::stopIntegrate ()
{
  req_count = 0;
  phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
  return Rts2DevPhot::stopIntegrate ();
}

int
Rts2DevPhotOptec::moveFilter (int new_filter)
{
  return phot_command (PHOT_CMD_MOVEFILTER, new_filter * FILTER_STEP);
}

int
Rts2DevPhotOptec::enableMove ()
{
  return phot_command (PHOT_CMD_INTEGR_ENABLED, 1);
}

int
Rts2DevPhotOptec::disableMove ()
{
  return phot_command (PHOT_CMD_INTEGR_ENABLED, 0);
}

Rts2DevPhotOptec *device;

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
  device = new Rts2DevPhotOptec (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize optec photometer - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
