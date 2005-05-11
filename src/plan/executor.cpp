#include "../utils/rts2device.h"
#include "target.h"
#include "rts2execcli.h"

#include <vector>

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

class Rts2Executor:public Rts2Device
{
private:
  Target * currentTarget;
  Target *nextTarget;
  void switchTarget ();
  struct ln_lnlat_posn *observer;

  int scriptCount;		// -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
    std::vector < Target * >targetsQue;
protected:
    virtual int info ()
  {
    return 0;
  }

public:
    Rts2Executor (int argc, char **argv);
  virtual ~ Rts2Executor (void);
  virtual int init ();
  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual void postEvent (Rts2Event * event);

  virtual int idle ();

  virtual int sendInfo (Rts2Conn * conn);

  int setNext (int nextId);
  void queTarget (Target * in_target);
};

int
Rts2ConnExecutor::commandAuthorized ()
{
  if (isCommand ("next"))
    {
      int tar_id;
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
Rts2Device (argc, argv, DEVICE_TYPE_EXECUTOR, 5570, "EXEC")
{
  char *states_names[1] = { "executor" };
  currentTarget = NULL;
  nextTarget = NULL;
  setStateNames (1, states_names);
  scriptCount = -1;
}

Rts2Executor::~Rts2Executor (void)
{

}

int
Rts2Executor::init ()
{
  int ret;
  ret = Rts2Device::init ();
  if (ret)
    return ret;
  // set priority..
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 20"));
  return 0;
}

Rts2Conn *
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
      return Rts2Device::createOtherType (conn, other_device_type);
    }
}

void
Rts2Executor::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SCRIPT_STARTED:
      if (scriptCount < 0)
	// start observation in DB??
	scriptCount = 1;
      else
	scriptCount++;
      break;
    case EVENT_LAST_READOUT:
      scriptCount--;
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
    }
  else
    {
      conn->send ("current -1");
    }
  if (nextTarget)
    {
      conn->sendValue ("next", nextTarget->getTargetID ());
    }
  else
    {
      conn->send ("next -1");
    }
  return 0;
}

int
Rts2Executor::setNext (int nextId)
{
  if (nextTarget)
    {
      if (nextTarget->getTargetID () == nextId)
	return 0;
      delete nextTarget;
    }
  nextTarget = createTarget (nextId, observer);
  if (!currentTarget)
    switchTarget ();
  else
    infoAll ();
  return 0;
}

void
Rts2Executor::switchTarget ()
{
  if (nextTarget)
    {
      // go to post-process
      if (currentTarget)
	queTarget (currentTarget);
      currentTarget = nextTarget;
      nextTarget = NULL;
    }
  postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
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

int
main (int argc, char **argv)
{
  int ret;
  executor = new Rts2Executor (argc, argv);
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
