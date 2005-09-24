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
protected:
  virtual void moveEnd ();
public:
    Rts2DevClientTelescopeSel (Rts2Conn * in_connection);
};

Rts2DevClientTelescopeSel::Rts2DevClientTelescopeSel (Rts2Conn * in_connection):Rts2DevClientTelescope
  (in_connection)
{
}

void
Rts2DevClientTelescopeSel::moveEnd ()
{
  connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
  Rts2DevClientTelescope::moveEnd ();
}

class Rts2DevClientExecutorSel:public Rts2DevClientExecutor
{
protected:
  virtual void lastReadout ();
public:
    Rts2DevClientExecutorSel (Rts2Conn * in_connection);
};

Rts2DevClientExecutorSel::Rts2DevClientExecutorSel (Rts2Conn * in_connection):Rts2DevClientExecutor
  (in_connection)
{
}

void
Rts2DevClientExecutorSel::lastReadout ()
{
  connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
  Rts2DevClientExecutor::lastReadout ();
}

class Rts2SelectorDev:public Rts2DeviceDb
{
private:
  Rts2Selector * sel;

  time_t last_selected;

  int next_id;

  int idle_select;
public:
    Rts2SelectorDev (int argc, char **argv);
    virtual ~ Rts2SelectorDev (void);
  virtual int processOption (int in_opt);
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

  virtual int sendInfo (Rts2Conn * conn)
  {
    return 0;
  }

  virtual int sendBaseInfo (Rts2Conn * conn)
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

Rts2SelectorDev::Rts2SelectorDev (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_SELECTOR, 5562, "SEL")
{
  sel = NULL;
  next_id = -1;
  time (&last_selected);

  idle_select = 300;
  addOption ('I', "idle_select", 1,
	     "selection timeout (reselect every I seconds)");
}

Rts2SelectorDev::~Rts2SelectorDev (void)
{
  if (sel)
    delete sel;
}

int
Rts2SelectorDev::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      idle_select = atoi (optarg);
      break;
    default:
      return Rts2DeviceDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2SelectorDev::init ()
{
  int ret;
  struct ln_lnlat_posn *observer;

  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();
  observer = config->getObserver ();

  sel = new Rts2Selector (observer);

  return 0;
}

int
Rts2SelectorDev::idle ()
{
  time_t now;
  time (&now);
  if (now > last_selected + idle_select)
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
