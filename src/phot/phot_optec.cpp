/*! 
 * @file Photometer deamon. 
 *
 * @author petr
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <time.h>

#include "../utils/rts2device.h"
#include "kernel/phot.h"
#include "status.h"

class Rts2DevPhotOptec:public Rts2Device
{
private:
  char *phot_dev;
  int fd;

  int phot_command (char command, short arg);

  long req_time_usec;
  short req_time;
  int req_count;

  Rts2Conn *integrateConn;

  virtual int startIntegrate (float in_req_time, int in_req_count);

public:
    Rts2DevPhotOptec (int argc, char **argv);
    virtual ~ Rts2DevPhotOptec (void);

  virtual int processOption (int in_opt);
  virtual int init ();

  virtual Rts2Conn *createConnection (int in_sock, int conn_num);

  virtual int idle ();

  virtual int homeFilter ();

  int startIntegrate (Rts2Conn * conn, float in_req_time, int in_req_count);
  virtual int endIntegrate ();
  virtual int stopIntegrate ();

  virtual int moveFilter (Rts2Conn * conn, int new_filter);

  virtual void cancelPriorityOperations ();

  virtual int setMasterState (int new_state);
};

class Rts2DevConnPhot:public Rts2DevConn
{
private:
  Rts2DevPhotOptec * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2DevConnPhot (int in_sock, Rts2DevPhotOptec * in_master_device);
};

int
Rts2DevConnPhot::commandAuthorized ()
{
  if (isCommand ("ready"))
    {
      return 0;
    }
  else if (isCommand ("baseInfo"))
    {
      return 0;
    }
  else if (isCommand ("info"))
    {
      return 0;
    }
  else if (isCommand ("home"))
    {
      return master->homeFilter ();
    }
  else if (isCommand ("integrate"))
    {
      float req_time;
      int req_count;
      CHECK_PRIORITY;
      if (paramNextFloat (&req_time) || paramNextInteger (&req_count)
	  || !paramEnd ())
	return -2;

      return master->startIntegrate (this, req_time, req_count);
    }

  else if (isCommand ("stop"))
    {
      CHECK_PRIORITY;
      return master->stopIntegrate ();
    }

  else if (isCommand ("filter"))
    {
      int new_filter;
      if (paramNextInteger (&new_filter) || !paramEnd ())
	return -2;
//    CHECK_PRIORITY;
      return master->moveFilter (this, new_filter);
      return 0;
    }
  else if (isCommand ("help"))
    {
      send ("info - phot informations");
      send ("exit - exit from main loop");
      send ("help - print, what you are reading just now");
      send ("integrate <time> <count> - start integration");
      send ("stop - stop any running integration");
      return 0;
    }
  return Rts2DevConn::commandAuthorized ();
}

Rts2DevConnPhot::Rts2DevConnPhot (int in_sock, Rts2DevPhotOptec * in_master_device):Rts2DevConn (in_sock,
	     in_master_device)
{
  master = in_master_device;
}

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

Rts2DevPhotOptec::Rts2DevPhotOptec (int argc, char **argv):Rts2Device (argc, argv, DEVICE_TYPE_PHOT, 5559,
	    "PHOT")
{
  char *
  states_names[1] = { "phot" };
  setStateNames (1, states_names);
  addOption ('f', "phot_file", 1, "photometer file (default to /dev/phot0)");
  phot_dev = "/dev/phot0";
  fd = -1;
  integrateConn = NULL;
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
  ret = Rts2Device::init ();
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

Rts2Conn *
Rts2DevPhotOptec::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnPhot (in_sock, this);
}

int
Rts2DevPhotOptec::idle ()
{
  if (getState (0) & PHOT_MASK_INTEGRATE == PHOT_INTEGRATE)
    {
      if (req_count > 0)
	{
	  int ret;
	  unsigned short result;
	  ret = read (fd, &result, 2);
	  if (ret == 2)
	    {
	      switch (result)
		{
		case 'A':
		  result = 0;
		  ret = read (fd, &result, 2);
		  req_count--;
		  integrateConn->sendValue ("count", result, req_time);
		  setTimeout (req_time_usec);
		  break;
		case '0':
		  result = 0;
		  ret = read (fd, &result, 2);
		  integrateConn->sendValue ("filter_c", result / 33);
		  setTimeout (1000);
		  break;
		case '-':
		  result = 0;
		  integrateConn->send ("count 0 0");
		  req_count = 0;
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
	  setTimeout (1000000);
	  return endIntegrate ();
	}
    }
  return 0;
}

int
Rts2DevPhotOptec::homeFilter ()
{
  return phot_command (PHOT_CMD_RESET, 0);
}

int
Rts2DevPhotOptec::startIntegrate (float in_req_time, int in_req_count)
{
  req_time_usec = ((int) in_req_time) * 1000000;
  req_time = (short) in_req_time;
  req_count = in_req_count;

  phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
  return phot_command (PHOT_CMD_INTEGRATE, req_time);
}

int
Rts2DevPhotOptec::startIntegrate (Rts2Conn * conn, float in_req_time,
				  int in_req_count)
{
  int ret;
  ret = startIntegrate (in_req_time, in_req_count);
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "cannot start integration");
      return -1;
    }
  conn->sendValue ("integration", req_time);
  integrateConn = conn;
  maskState (0, PHOT_MASK_INTEGRATE, PHOT_INTEGRATE, "Integration started");
  return 0;
}

int
Rts2DevPhotOptec::endIntegrate ()
{
  req_count = 0;
  maskState (0, PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
	     "Integration finished");
  return phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
}

int
Rts2DevPhotOptec::stopIntegrate ()
{
  req_count = 0;
  maskState (0, PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
	     "Integration interrupted");
  return phot_command (PHOT_CMD_STOP_INTEGRATE, 0);
}

int
Rts2DevPhotOptec::moveFilter (Rts2Conn * conn, int new_filter)
{
  int ret;
  ret = phot_command (PHOT_CMD_MOVEFILTER, new_filter * 33);
  if (ret)
    return -1;
  conn->sendValue ("filter", new_filter);
  return 0;
}

void
Rts2DevPhotOptec::cancelPriorityOperations ()
{
  if (getState (0) & PHOT_MASK_INTEGRATE == PHOT_INTEGRATE)
    {
      stopIntegrate ();
    }
}

int
Rts2DevPhotOptec::setMasterState (int new_state)
{
  switch (new_state & SERVERD_STATUS_MASK)
    {
    case SERVERD_NIGHT:
      phot_command (PHOT_CMD_INTEGR_ENABLED, 1);
      break;
    default:			/* other - SERVERD_DAY, SERVERD_DUSK, SERVERD_MAINTANCE, SERVERD_OFF, etc */
      phot_command (PHOT_CMD_INTEGR_ENABLED, 0);
    }
  return 0;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevPhotOptec *device = new Rts2DevPhotOptec (argc, argv);

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
