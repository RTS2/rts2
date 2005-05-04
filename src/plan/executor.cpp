#include "../utils/rts2device.h"
#include "target.h"
#include "rts2execcli.h"

class Rts2Executor;

class Rts2ConnExecutor:public Rts2DevConn
{
private:
  Rts2Executor * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2ConnExecutor (int in_sock, Rts2Executor * in_master);
  void setOtherType (int other_device_type);
};

class Rts2Executor:public Rts2Device
{
private:
  Target * currentTarget;
  Target *nextTarget;
  void switchTarget ();
  struct ln_lnlat_posn *observer;

public:
    Rts2Executor (int argc, char **argv);
    virtual ~ Rts2Executor (void);
  virtual int init ();
  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
  virtual int idle ();

  virtual int info (Rts2Conn * conn);

  int setNext (int nextId);
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

void
Rts2ConnExecutor::setOtherType (int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      otherDevice = new Rts2DevClientTelescopeExec (this);
      break;
    case DEVICE_TYPE_CCD:
      otherDevice = new Rts2DevClientCameraExec (this);
      break;
    default:
      Rts2Conn::setOtherType (other_device_type);
    }
}

Rts2Executor::Rts2Executor (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_EXECUTOR, 5570, "EXEC")
{
  char *states_names[1] = { "executor" };
  currentTarget = NULL;
  nextTarget = NULL;
  setStateNames (1, states_names);
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

int
Rts2Executor::idle ()
{
  return Rts2Device::idle ();
}

int
Rts2Executor::info (Rts2Conn * conn)
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
  return 0;
}

void
Rts2Executor::switchTarget ()
{
  if (nextTarget)
    {
      // go to post-process
      //queTarget (currentTarget);
      currentTarget = nextTarget;
      nextTarget = NULL;
    }
  postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
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
