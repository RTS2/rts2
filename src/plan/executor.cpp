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
  Target *priorityTarget;
  void doSwitch ();
  void switchTarget ();

  int scriptCount;		// -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
  int waitState;
    std::vector < Target * >targetsQue;
  struct ln_lnlat_posn *observer;

  int ignoreDay;
  double grb_sep_limit;
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
  virtual int sendBaseInfo (Rts2Conn * conn);
  virtual int info ()
  {
    updateScriptCount ();
    return 0;
  }
  virtual int sendInfo (Rts2Conn * conn);

  virtual int changeMasterState (int new_state);

  int setNext (int nextId);
  int setNow (int nextId);
  int setNow (Target * newTarget);
  int setGrb (int grbId);
  void queTarget (Target * in_target);
  void updateScriptCount ();
};

int
Rts2ConnExecutor::commandAuthorized ()
{
  int tar_id;
  if (isCommand ("grb"))
    {
      // change observation if we are to far from GRB position..
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->setGrb (tar_id);
    }
  else if (isCommand ("now"))
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
  priorityTarget = NULL;
  setStateNames (1, states_names);
  scriptCount = -1;

  addOption ('I', "ignore_day", 0, "observe even during daytime");
  ignoreDay = 0;

  grb_sep_limit = -1;

  waitState = 0;
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
  config->getDouble ("grbd", "seplimit", grb_sep_limit);
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
  Rts2DevClient *cli;
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      cli = new Rts2DevClientTelescopeExec (conn);
      break;
    case DEVICE_TYPE_CCD:
      cli = new Rts2DevClientCameraExec (conn);
      break;
    case DEVICE_TYPE_FOCUS:
      cli = new Rts2DevClientFocusImage (conn);
      break;
    case DEVICE_TYPE_DOME:
      cli = new Rts2DevClientDomeImage (conn);
      break;
    default:
      cli = Rts2DeviceDb::createOtherType (conn, other_device_type);
    }
  cli->postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
  return cli;
}

void
Rts2Executor::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_OBSERVE:
      break;
    case EVENT_SCRIPT_STARTED:
      // we don't care about that now..
      break;
    case EVENT_LAST_READOUT:
    case EVENT_SCRIPT_ENDED:
      updateScriptCount ();
      if (currentTarget)
	{
	  if (scriptCount == 0 && currentTarget->observationStarted ())
	    {
	      switchTarget ();
	    }
	  // scriptCount is not 0, but we hit continues target..
	  else if (currentTarget->isContinues ()
		   && (nextTarget == NULL
		       || nextTarget->getTargetID () ==
		       currentTarget->getTargetID ()))
	    {
	      // that will eventually hit devclient which post that message, which
	      // will set currentTarget to this value and handle it same way as EVENT_OBSERVE,
	      // which is exactly what we want
	      event->setArg ((void *) currentTarget);
	    }
	}
      else
	{
	  if (scriptCount == 0)
	    switchTarget ();
	}
      break;
    case EVENT_MOVE_OK:
      if (waitState)
	{
	  postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
	  break;
	}
      postEvent (new Rts2Event (EVENT_OBSERVE));
      break;
    case EVENT_MOVE_FAILED:
      if (*((int *) event->getArg ()) == DEVICE_ERROR_KILL && priorityTarget)
	{
	  // we are free to start new hig-priority observation
	  currentTarget = priorityTarget;
	  priorityTarget = NULL;
	  postEvent (new
		     Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
	  postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET));
	  break;
	}
      if (waitState)
	{
	  postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
	}
      else if (currentTarget)
	{
	  // get us lover priority to prevent moves to such dangerous
	  // position
	  currentTarget->changePriority (-100,
					 ln_get_julian_from_sys () +
					 12 * (1.0 / 1440.0));
	}
      updateScriptCount ();
      if (scriptCount == 0)
	switchTarget ();
      break;
    case EVENT_ENTER_WAIT:
      waitState = 1;
      break;
    case EVENT_CLEAR_WAIT:
      waitState = 0;
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
Rts2Executor::sendBaseInfo (Rts2Conn * conn)
{
  return 0;
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
  if (priorityTarget)
    {
      conn->sendValue ("priority_target", priorityTarget->getTargetID ());
    }
  else
    {
      conn->sendValue ("priority_target", -1);
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
      if (!currentTarget && nextTarget)
	{
	  switchTarget ();
	}
      break;
    case SERVERD_MORNING:
    case SERVERD_DAY:
      // we need to stop observation that is continuus
      // and at the mean time kept pointer to it, so we can delete it
      // first we free nextTarget pointer..
      if (nextTarget)
	{
	  delete nextTarget;
	}
      // that will guaranite that in isContinues call, we will find currentTarget
      // NULL, so we will call switchTarget, which will do the job..
      // delete on nextTarget will call endObservation, so we will end observation of current target
      nextTarget = currentTarget;
      currentTarget = NULL;
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

  return setNow (newTarget);
}

int
Rts2Executor::setNow (Target * newTarget)
{
  if (currentTarget)
    {
      currentTarget->endObservation (-1);
      queTarget (currentTarget);
    }
  currentTarget = NULL;
  priorityTarget = newTarget;

  // at this situation, we would like to get rid of nextTarget as
  // well
  if (nextTarget)
    {
      delete nextTarget;
      nextTarget = NULL;
    }

  postEvent (new Rts2Event (EVENT_KILL_ALL));
  queAll (new Rts2CommandKillAll (this));

  return 0;
}

int
Rts2Executor::setGrb (int grbId)
{
  Target *grbTarget;
  struct ln_hrz_posn grbHrz;
  int ret;

  // is during night and ready?
  if (getMasterState () != SERVERD_NIGHT)
    {
      syslog (LOG_DEBUG, "Rts2Executor::setGrb daylight GRB ignored");
      return -2;
    }

  grbTarget = createTarget (grbId, observer);

  if (!grbTarget)
    {
      return -2;
    }
  ret = grbTarget->getAltAz (&grbHrz);
  // don't care if we don't get altitude from libnova..it's completly weirde, but we should not
  // miss GRB:(
  if (!ret)
    {
      syslog (LOG_DEBUG, "Rts2Executor::setGrb grbHrz alt:%f", grbHrz.alt);
      if (grbHrz.alt < 0)
	{
	  delete grbTarget;
	  return -2;
	}
    }

  // if we don't observe anything..bring us to GRB..
  if (!currentTarget)
    {
      nextTarget = grbTarget;
      switchTarget ();
      return 0;
    }
  ret = grbTarget->compareWithTarget (currentTarget, grb_sep_limit);
  if (ret == 0)
    {
      return setNow (grbTarget);
    }
  // otherwise set us as next target
  if (nextTarget)
    delete nextTarget;
  nextTarget = grbTarget;
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
	  if (!(ret == 1 && nextId == currentTarget->getTargetID ()))
	    // don't que only in case nextTarget and currentTarget are
	    // same and endObservation returns 1
	    {
	      queTarget (currentTarget);
	      currentTarget = nextTarget;
	      nextTarget = NULL;
	    }
	}
      else
	{
	  currentTarget = nextTarget;
	  nextTarget = NULL;
	}
    }
  postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
  postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET));
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
	  doSwitch ();
	  break;
	default:
	  if (currentTarget)
	    {
	      currentTarget->endObservation (-1);
	      queTarget (currentTarget);
	    }
	  currentTarget = NULL;
	  if (nextTarget)
	    {
	      delete nextTarget;
	      nextTarget = NULL;
	    }
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
  else
    delete in_target;
}

void
Rts2Executor::updateScriptCount ()
{
  scriptCount = 0;
  postEvent (new Rts2Event (EVENT_MOVE_QUESTION, (void *) &scriptCount));
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
