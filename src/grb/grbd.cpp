#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*!
 *
 * Receive info from GCN via socket, put them to DB.
 *
 * Based on http://gcn.gsfc.nasa.gov/socket_demo.c 
 * socket_demo     Ver: 3.29   23 Mar 05,
 * which is CVSed with GRBC. Only "active" satellite packets are received.
 *
 * If new version of socket_demo.c show up, we need to invesigate
 * modifications to include it.
 */

#include "../utils/rts2command.h"
#include "grbd.h"

#include <signal.h>

class Rts2DevConnGrbd:public Rts2DevConn
{
private:
  Rts2DevGrb * master;
public:
  Rts2DevConnGrbd (int in_sock, Rts2DevGrb * in_master);
    virtual ~ Rts2DevConnGrbd (void);

  virtual int commandAuthorized ();
};

Rts2DevConnGrbd::Rts2DevConnGrbd (int in_sock, Rts2DevGrb * in_master):
Rts2DevConn (in_sock, in_master)
{
  master = in_master;
}

Rts2DevConnGrbd::~Rts2DevConnGrbd (void)
{
}

int
Rts2DevConnGrbd::commandAuthorized ()
{
  if (isCommand ("test"))
    {
      int tar_id;
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->newGcnGrb (tar_id);
    }
  return Rts2DevConn::commandAuthorized ();
}

Rts2DevGrb::Rts2DevGrb (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_GRB, 5563, "GRB")
{
  gcncnn = NULL;
  gcn_host = NULL;
  gcn_port = -1;
  do_hete_test = 0;

  addOption ('S', "gcn_host", 1, "GCN host name");
  addOption ('P', "gcn_port", 1, "GCN port");
  addOption ('T', "test", 0,
	     "process test notices (default to off - don't process them)");
}

Rts2DevGrb::~Rts2DevGrb (void)
{
  if (gcn_host)
    delete[]gcn_host;
}

int
Rts2DevGrb::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'S':
      gcn_host = new char[strlen (optarg) + 1];
      strcpy (gcn_host, optarg);
      break;
    case 'P':
      gcn_port = atoi (optarg);
      break;
    case 'T':
      do_hete_test = 1;
      break;
    default:
      return Rts2DeviceDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevGrb::init ()
{
  int ret;

  Rts2Config *config;

  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  config = Rts2Config::instance ();

  // get some default, if we cannot get them from command line
  if (!gcn_host)
    {
      gcn_host = new char[200];
      ret = config->getString ("grbd", "server", gcn_host, 200);
      if (ret)
	return -1;
    }

  if (gcn_port < 0)
    {
      ret = config->getInteger ("grbd", "port", gcn_port);
      if (ret)
	return -1;
    }

  // add connection..
  gcncnn = new Rts2ConnGrb (gcn_host, gcn_port, do_hete_test, this);
  // wait till grb connection init..
  while (1)
    {
      ret = gcncnn->init ();
      if (!ret)
	break;
      syslog (LOG_ERR,
	      "Rts2DevGrb::init cannot init conngrb, sleeping for 60 sec");
      sleep (60);
    }
  addConnection (gcncnn);
  return ret;
}

Rts2DevConn *
Rts2DevGrb::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConnGrbd (in_sock, this);
}

int
Rts2DevGrb::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("last_packet", gcncnn->lastPacket ());
  conn->sendValue ("delta", gcncnn->delta ());
  conn->sendValue ("last_target", gcncnn->lastTarget ());
  conn->sendValue ("last_target_time", gcncnn->lastTargetTime ());
  return 0;
}

// that method is called when somebody want to immediatelly observe GRB
int
Rts2DevGrb::newGcnGrb (int tar_id)
{
  Rts2Conn *exec;
  exec = getOpenConnection ("EXEC");
  if (exec)
    {
      exec->queCommand (new Rts2CommandExecGrb (this, tar_id));
    }
  else
    {
      syslog (LOG_ERR, "FATAL! No executor running to post grb ID %i!",
	      tar_id);
      return -1;
    }
  return 0;
}

Rts2DevGrb *grb;

void
killSignal (int sig)
{
  if (grb)
    delete grb;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  grb = new Rts2DevGrb (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  ret = grb->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot init GRB device, exiting");
      delete grb;
      return 1;
    }
  grb->run ();
  delete grb;
}
