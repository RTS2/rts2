#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
/*!
 * Used for generating new plan entries.
 *
 * @author petr
 */

#include "../utils/rts2devclient.h"
#include "rts2selector.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utils/rts2event.h"
#include "../utils/rts2command.h"

#include <signal.h>

class Rts2DevClientTelescopeSel:public Rts2DevClientTelescope
{
public:
  Rts2DevClientTelescopeSel (Rts2Conn * in_connection);
  virtual void stateChanged (Rts2ServerState * state);
};

Rts2DevClientTelescopeSel::Rts2DevClientTelescopeSel (Rts2Conn * in_connection):Rts2DevClientTelescope
  (in_connection)
{
}

void
Rts2DevClientTelescopeSel::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("telescope"))
    {
      if ((state->value & TEL_MASK_MOVING) == TEL_OBSERVING
	  || (state->value & TEL_MASK_MOVING) == TEL_PARKING)
	{
	  connection->getMaster ()->
	    postEvent (new Rts2Event (EVENT_IMAGE_OK));
	}
    }
  Rts2DevClientTelescope::stateChanged (state);
}

class Rts2SelectorDev:public Rts2DeviceDb
{
private:
  Rts2Selector * sel;

  time_t last_selected;

  int next_id;
public:
    Rts2SelectorDev (int argc, char **argv);
    virtual ~ Rts2SelectorDev (void);
  virtual int init ();
  virtual int idle ();

  virtual int ready ()
  {
    return 0;
  }

  virtual int baseInfo ()
  {
    return 0;
  }

  virtual int info ()
  {
    return 0;
  }

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);
  virtual void postEvent (Rts2Event * event);
  virtual int changeMasterState (int new_state);

  int selectNext ();		// return next observation..
  int updateNext ();
};

Rts2SelectorDev::Rts2SelectorDev (int argc, char **argv):
Rts2DeviceDb (argc, argv, DEVICE_TYPE_SELECTOR, 5562, "SEL")
{
  sel = NULL;
  next_id = -1;
  time (&last_selected);
}

Rts2SelectorDev::~Rts2SelectorDev (void)
{
  if (sel)
    delete sel;
}

int
Rts2SelectorDev::init ()
{
  int ret;
  char horizontFile[250];
  struct ln_lnlat_posn *observer;

  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();
  observer = config->getObserver ();

  // add read config..when we will get config
  config->getString ("observatory", "horizont", horizontFile, 250);

  sel = new Rts2Selector (observer, horizontFile);

  return 0;
}

int
Rts2SelectorDev::idle ()
{
  time_t now;
  time (&now);
  if (now > last_selected + 500)
    {
      updateNext ();
      time (&last_selected);
    }
  return Rts2DeviceDb::idle ();
}

Rts2DevClient *
Rts2SelectorDev::createOtherType (Rts2Conn * conn, int other_device_type)
{
  Rts2DevClient *ret;
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      return new Rts2DevClientTelescopeSel (conn);
    case DEVICE_TYPE_EXECUTOR:
      ret = Rts2DeviceDb::createOtherType (conn, other_device_type);
      updateNext ();
      if (next_id > 0)
	conn->queCommand (new Rts2CommandExecNext (this, next_id));
      return ret;
    default:
      return Rts2DeviceDb::createOtherType (conn, other_device_type);
    }
}

void
Rts2SelectorDev::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_IMAGE_OK:
      updateNext ();
      break;
    }
  Rts2DeviceDb::postEvent (event);
}

int
Rts2SelectorDev::selectNext ()
{
  return sel->selectNext (getMasterState ());
}

int
Rts2SelectorDev::updateNext ()
{
  Rts2Conn *exec;
  next_id = selectNext ();
  if (next_id > 0)
    {
      exec = getOpenConnection ("EXEC");
      if (exec)
	{
	  exec->queCommand (new Rts2CommandExecNext (this, next_id));
	}
      return 0;
    }
  return -1;
}

int
Rts2SelectorDev::changeMasterState (int new_master_state)
{
  updateNext ();
  return Rts2DeviceDb::changeMasterState (new_master_state);
}

Rts2SelectorDev *selector;

void
killSignal (int sig)
{
  if (selector)
    delete selector;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  selector = new Rts2SelectorDev (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = selector->init ();
  if (ret)
    return ret;
  selector->run ();
  delete selector;
  return 0;
}
