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

#include "rts2conngrb.h"

class Rts2DevGrb:public Rts2Device
{
public:
  Rts2DevGrb (int argc, char **argv);
    virtual ~ Rts2DevGrb ();
  virtual int init ();
};

Rts2DevGrb::Rts2DevGrb (int argc, char **argv):
Rts2Device (argc, argv, DEVICE_TYPE_GRB, 5563, "GRB")
{
}

Rts2DevGrb::~Rts2DevGrb (void)
{
}

int
Rts2DevGrb::init ()
{
  int ret;

  Rts2ConnGrb *conngrb;

  ret = Rts2Device::init ();
  if (ret)
    return ret;

  // add connection..
  conngrb = new Rts2ConnGrb ("iam1a", 2224, this);
  // wait till grb connection init..
  while (1)
    {
      ret = conngrb->init ();
      if (!ret)
	break;
      syslog (LOG_ERR,
	      "Rts2DevGrb::init cannot init conngrb, sleeping fro 60 sec");
      sleep (60);
    }
  addConnection (conngrb);
  return ret;
}

Rts2DevGrb *grb;

int
main (int argc, char **argv)
{
  int ret;
  grb = new Rts2DevGrb (argc, argv);
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
