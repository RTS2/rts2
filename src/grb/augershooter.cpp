/**
 * Receive Auger socket shower info, send now request to executor.
 *
 * @author petr
 */

#include "augershooter.h"
#include "../utils/rts2command.h"

Rts2DevAugerShooter::Rts2DevAugerShooter (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_AUGERSH, "AUGRSH")
{
  shootercnn = NULL;
  port = 1240;
  addOption ('S', "shooter_port", 1,
	     "port on which to listen for auger connection");
}

Rts2DevAugerShooter::~Rts2DevAugerShooter (void)
{
}

int
Rts2DevAugerShooter::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'S':
      port = atoi (optarg);
      break;
    default:
      return Rts2DeviceDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevAugerShooter::init ()
{
  int ret;
  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;

  Rts2Config *config = Rts2Config::instance ();

  shootercnn = new Rts2ConnShooter (port, this);

  ret = shootercnn->init ();

  if (ret)
    return ret;

  addConnection (shootercnn);

  return ret;
}

int
Rts2DevAugerShooter::newShower ()
{
  Rts2Conn *exec;
  exec = getOpenConnection ("EXEC");
  if (exec)
    {
      exec->queCommand (new Rts2CommandExecShower (this));
    }
  else
    {
      syslog (LOG_ERR, "FATAL! No executor running to post shower!");
      return -1;
    }
  return 0;
}

Rts2DevAugerShooter *device = NULL;

void
killSignal (int sig)
{
  delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  device = new Rts2DevAugerShooter (argc, argv);
  ret = device->init ();
  if (ret)
    return ret;
  ret = device->run ();
  delete device;
  return ret;
}
