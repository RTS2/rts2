#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"
#include "rts2execcli.h"

#include <vector>
#include <signal.h>

class Rts2Executor;

class Rts2ConnExecutor:public Rts2DevConn
{
private:
  Rts2Executor * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2ConnExecutor (int in_sock, Rts2Executor * in_master);
};

class Rts2Executor:public Rts2DeviceDb
{
private:
  Target * currentTarget;
  Target *nextTarget;
  void doSwitch ();
  void switchTarget ();

  int scriptCount;		// -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
    std::vector < Target * >targetsQue;
  struct ln_lnlat_posn *observer;

  int ignoreDay;

public:
    Rts2Executor (int argc, char **argv);
    virtual ~ Rts2Executor (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);
  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual void postEvent (Rts2Event * event);

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
  virtual int sendInfo (Rts2Conn * conn);

  virtual int changeMasterState (int new_state);

  int setNext (int nextId);
  int setNow (int nextId);
  void queTarget (Target * in_target);
};

int
Rts2ConnExecutor::commandAuthorized ()
{
  int tar_id;
  if (isCommand ("now"))
    {
      // change observation imediatelly - in case of burst etc..
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->setNow (tar_id);
    }
  else if (isCommand ("next"))
    {
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->setNext (tar_id);
    }
  return Rts2DevConn::commandAuthorized ();
}

Rts2ConnExecutor::Rts2ConnExecutor (int in_sock, Rts2Executor * in_master):Rts2DevConn (in_sock,
	     in_master)
{
  master = in_master;
}

Rts2Executor::Rts2Executor (int argc, char **argv):
Rts2DeviceDb (argc, argv, DEVICE_TYPE_EXECUTOR, 5570, "EXEC")
{
  char *states_names[1] = { "executor" };
  currentTarget = NULL;
  nextTarget = NULL;
  setStateNames (1, states_names);
  scriptCount = -1;

  addOption ('I', "ignore_day", 0, "observe even during daytime");
  ignoreDay = 0;
}

Rts2Executor::~Rts2Executor (void)
{
  if (currentTarget)
    delete currentTarget;
  if (nextTarget)
    delete nextTarget;
}

int
Rts2Executor::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      ignoreDay = 1;
      break;
    default:
      return Rts2DeviceDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2Executor::init ()
{
  int ret;
  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;
  // set priority..
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 20"));
  Rts2Config *config;
  config = Rts2Config::instance ();
  observer = config->getObserver ();
  return 0;
}

Rts2DevConn *
Rts2Executor::createConnection (int in_sock, int conn_num)
{
  return new Rts2ConnExecutor (in_sock, this);
}

Rts2DevClient *
Rts2Executor::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      return new Rts2DevClientTelescopeExec (conn);
    case DEVICE_TYPE_CCD:
      return new Rts2DevClientCameraExec (conn);
    default:
      return Rts2DeviceDb::createOtherType (conn, other_device_type);
    }
}

void
Rts2Executor::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SCRIPT_STARTED:
      if (scriptCount < 0)
	scriptCount = 1;
      else
	scriptCount++;
      break;
    case EVENT_LAST_READOUT:
      scriptCount--;
      if (scriptCount == 0)
	switchTarget ();
      break;
    case EVENT_MOVE_FAILED:
      if (currentTarget)
	{
	  // get us lover priority to prevent moves to such dangerous
	  // position
	  currentTarget->changePriority (-100,
					 ln_get_julian_from_sys () +
					 12 * (1.0 / 1440.0));
	}
      if (scriptCount == 0)
	switchTarget ();
      break;
    }
  Rts2Device::postEvent (event);
}

int
Rts2Executor::idle ()
{
  return Rts2Device::idle ();
}

int
Rts2Executor::sendInfo (Rts2Conn * conn)
{
  if (currentTarget)
    {
      conn->sendValue ("current", currentTarget->getTargetID ());
      conn->sendValue ("obsid", currentTarget->getObsId ());
    }
  else
    {
      conn->sendValue ("current", -1);
      conn->sendValue ("obsid", -1);
    }
  if (nextTarget)
    {
      conn->sendValue ("next", nextTarget->getTargetID ());
    }
  else
    {
      conn->sendValue ("next", -1);
    }
  conn->sendValue ("script_count", scriptCount);
  return 0;
}

int
Rts2Executor::changeMasterState (int new_state)
{
  if (ignoreDay)
    return Rts2DeviceDb::changeMasterState (new_state);

  switch (new_state)
    {
    case SERVERD_EVENING:
    case SERVERD_DAWN:
    case SERVERD_NIGHT:
    case SERVERD_DUSK:
    case SERVERD_MORNING:
      if (!currentTarget && nextTarget)
	{
	  switchTarget ();
	}
      break;
    }
  return Rts2DeviceDb::changeMasterState (new_state);
}

int
Rts2Executor::setNext (int nextId)
{
  if (nextTarget)
    {
      if (nextTarget->getTargetID () == nextId)
	// we observe the same target..
	return 0;
      delete nextTarget;
    }
  nextTarget = createTarget (nextId, observer);
  if (!nextTarget)
    return -1;
  if (!currentTarget)
    switchTarget ();
  else
    infoAll ();
  return 0;
}

int
Rts2Executor::setNow (int nextId)
{
  Target *newTarget;
  newTarget = createTarget (nextId, observer);
  if (!newTarget)
    {
      // error..
      return -2;
    }
  if (currentTarget)
    {
      currentTarget->endObservation (-1);
      queTarget (currentTarget);
    }
  currentTarget = newTarget;

  queAll (new Rts2CommandKillAll (this));

  // move operation will be qued after kill command, so we get correct
  // behaviur..et least we should get it
  postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));

  // at this situation, we would like to get rid of nextTarget as
  // well:(
  if (nextTarget)
    {
      delete nextTarget;
      nextTarget = NULL;
    }
  return 0;
}

void
Rts2Executor::doSwitch ()
{
  int ret;
  int nextId;
  if (nextTarget)
    {
      // go to post-process
      if (currentTarget)
	{
	  if (nextTarget)
	    {
	      nextId = nextTarget->getTargetID ();
	    }
	  else
	    {
	      nextId = -1;
	    }
	  ret = currentTarget->endObservation (nextId);
	  if (ret != 1 || nextId != currentTarget->getTargetID ())
	    // don't que only in case nextTarget and currentTarget are
	    // same and endObservation returns 1
	    {
	      queTarget (currentTarget);
	      currentTarget = nextTarget;
	      nextTarget = NULL;
	    }
	  else
	    {
	      currentTarget = nextTarget;
	      nextTarget = NULL;
	    }
	}
    }
  postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
}

void
Rts2Executor::switchTarget ()
{
  if (ignoreDay)
    {
      doSwitch ();
    }
  else
    {
      // we will not observe during daytime..
      switch (getMasterState ())
	{
	case SERVERD_EVENING:
	case SERVERD_DUSK:
	case SERVERD_NIGHT:
	case SERVERD_DAWN:
	case SERVERD_MORNING:
	  doSwitch ();
	  break;
	default:
	  if (currentTarget)
	    queTarget (currentTarget);
	  currentTarget = NULL;
	  delete nextTarget;
	  nextTarget = NULL;
	}
    }
  infoAll ();
}

void
Rts2Executor::queTarget (Target * in_target)
{
  int ret;
  ret = in_target->postprocess ();
  if (!ret)
    targetsQue.push_back (in_target);
}

Rts2Executor *executor;

void
killSignal (int sig)
{
  if (executor)
    delete executor;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  executor = new Rts2Executor (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = executor->init ();
  if (ret)
    {
      fprintf (stderr, "cannot start executor");
      return ret;
    }
  executor->run ();
  delete executor;
  return 0;
}
