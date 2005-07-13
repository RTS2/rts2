/*! 
 * @file Photometer deamon. 
 *
 * @author petr
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define FILTER_STEP  33

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
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

  int filter;

  Rts2Conn *integrateConn;

  virtual int startIntegrate (float in_req_time, int in_req_count);

public:
    Rts2DevPhotOptec (int argc, char **argv);
    virtual ~ Rts2DevPhotOptec (void);

  virtual int processOption (int in_opt);
  virtual int init ();

  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);

  virtual int idle ();

  virtual int deleteConnection (Rts2Conn * conn)
  {
    if (integrateConn == conn)
      integrateConn = NULL;
    return Rts2Device::deleteConnection (conn);
  }

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

  int startIntegrate (Rts2Conn * conn, float in_req_time, int in_req_count);
  virtual int endIntegrate ();
  virtual int stopIntegrate ();

  virtual int moveFilter (Rts2Conn * conn, int new_filter);
  virtual int enableFilter (Rts2Conn * conn);

  virtual void cancelPriorityOperations ();

  virtual int changeMasterState (int new_state);

  virtual int sendInfo (Rts2Conn * conn);
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
  if (isCommand ("home"))
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
    }
  else if (isCommand ("enable"))
    {
      if (!paramEnd ())
	return -2;
      CHECK_PRIORITY;
      return master->enableFilter (this);
    }
  else if (isCommand ("help"))
    {
      send ("info - phot informations");
      send ("exit - exit from main loop");
      send ("help - print, what you are reading just now");
      send ("integrate <time> <count> - start integration");
      send ("enable - enable filter movements");
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

Rts2DevConn *
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
		    integrateConn->sendValue ("count_ov", result[1],
					      req_time);
		  else
		    integrateConn->sendValue ("count", result[1], req_time);
		  setTimeout (req_time_usec);
		  break;
		case '0':
		  filter = result[1] / FILTER_STEP;
		  integrateConn->sendValue ("filter", filter);
		  setTimeout (1000);
		  break;
		case '-':
		  integrateConn->send ("count 0 0");
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
  return Rts2Device::idle ();
}

int
Rts2DevPhotOptec::homeFilter ()
{
  filter = 0;
  return phot_command (PHOT_CMD_RESET, 0);
}

int
Rts2DevPhotOptec::startIntegrate (float in_req_time, int in_req_count)
{
  req_time = (short) (in_req_time * 1000);
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
  conn->sendValue ("integration", req_time / 1000.0);
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
  integrateConn = NULL;
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
  ret = phot_command (PHOT_CMD_MOVEFILTER, new_filter * FILTER_STEP);
  if (ret)
    return -1;
  filter = new_filter;
  conn->sendValue ("filter", new_filter);
  return 0;
}

int
Rts2DevPhotOptec::enableFilter (Rts2Conn * conn)
{
  int ret;
  ret = phot_command (PHOT_CMD_INTEGR_ENABLED, 1);
  if (ret)
    return -1;
  return 0;
}

void
Rts2DevPhotOptec::cancelPriorityOperations ()
{
  if (getState (0) & PHOT_MASK_INTEGRATE == PHOT_INTEGRATE)
    {
      stopIntegrate ();
    }
  Rts2Device::cancelPriorityOperations ();
}

int
Rts2DevPhotOptec::changeMasterState (int new_state)
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
Rts2DevPhotOptec::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("filter", filter);
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
