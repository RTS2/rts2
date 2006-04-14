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

#include <signal.h>
#include <time.h>

class Rts2DevPhotDummy:public Rts2DevPhot
{
private:
  int filterCount;
protected:
    virtual int startIntegrate ();
public:
    Rts2DevPhotDummy (int argc, char **argv);

  virtual int scriptEnds ();

  virtual long getCount ();

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
  virtual int startFilterMove (int new_filter);
  virtual long isFilterMoving ();
  virtual int enableMove ();
  virtual int disableMove ();
};

Rts2DevPhotDummy::Rts2DevPhotDummy (int in_argc, char **in_argv):
Rts2DevPhot (in_argc, in_argv)
{
}

int
Rts2DevPhotDummy::scriptEnds ()
{
  startFilterMove (0);
  return Rts2DevPhot::scriptEnds ();
}

long
Rts2DevPhotDummy::getCount ()
{
  sendCount (random (), req_time, 0);
  return (long (req_time * USEC_SEC));
}

int
Rts2DevPhotDummy::homeFilter ()
{
  return 0;
}

int
Rts2DevPhotDummy::startIntegrate ()
{
  return 0;
}

int
Rts2DevPhotDummy::startFilterMove (int new_filter)
{
  filter = new_filter;
  filterCount = 10;
  return Rts2DevPhot::startFilterMove (new_filter);
}

long
Rts2DevPhotDummy::isFilterMoving ()
{
  if (filterCount <= 0)
    return -2;
  filterCount--;
  return USEC_SEC / 10;
}

int
Rts2DevPhotDummy::enableMove ()
{
  return 0;
}

int
Rts2DevPhotDummy::disableMove ()
{
  return 0;
}

Rts2DevPhotDummy *device;

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
  device = new Rts2DevPhotDummy (argc, argv);

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
