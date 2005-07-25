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

#include <signal.h>

#include "grbd.h"

Rts2DevGrb::Rts2DevGrb (int argc, char **argv):
Rts2DeviceDb (argc, argv, DEVICE_TYPE_GRB, 5563, "GRB")
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

int
Rts2DevGrb::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("last_packet", gcncnn->lastPacket ());
  conn->sendValue ("delta", gcncnn->delta ());
  conn->sendValue ("last_target", gcncnn->lastTarget ());
  conn->sendValue ("last_target_time", gcncnn->lastTargetTime ());
}

// that method is called when somebody want to immediatelly observe GRB
int
Rts2DevGrb::newGcnGrb (int grb_id, int grb_seqn, int grb_type, double grb_ra,
		       double grb_dec, int grb_is_grb, time_t * grb_date,
		       float grb_errorbox)
{
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
