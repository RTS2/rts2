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
  addOption ('s', "shooter_port", 1,
	     "port on which to listen for auger connection");

  lastAugerDate = new Rts2ValueTime ("last_date");
  addValue (lastAugerDate);
  lastAugerRa = new Rts2ValueDouble ("last_ra");
  addValue (lastAugerRa);
  lastAugerDec = new Rts2ValueDouble ("last_dec");
  addValue (lastAugerDec);
}

Rts2DevAugerShooter::~Rts2DevAugerShooter (void)
{
}

int
Rts2DevAugerShooter::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 's':
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
  double minEnergy;

  minEnergy = 20;
  config->getDouble ("augershooter", "minenergy", minEnergy);

  shootercnn = new Rts2ConnShooter (port, this, minEnergy);

  ret = shootercnn->init ();

  if (ret)
    return ret;

  addConnection (shootercnn);

  return ret;
}

int
Rts2DevAugerShooter::newShower (double lastDate, double ra, double dec)
{
  Rts2Conn *exec;
  lastAugerDate->setValueDouble (lastDate);
  lastAugerRa->setValueDouble (ra);
  lastAugerDec->setValueDouble (dec);
  exec = getOpenConnection ("EXEC");
  if (exec)
    {
      exec->queCommand (new Rts2CommandExecShower (this));
    }
  else
    {
      logStream (MESSAGE_ERROR) <<
	"FATAL! No executor running to post shower!" << sendLog;
      return -1;
    }
  return 0;
}

bool
Rts2DevAugerShooter::wasSeen (double lastDate, double ra, double dec)
{
  return (fabs (lastDate - lastAugerDate->getValueDouble ()) < 5
	  && ra == lastAugerRa->getValueDouble ()
	  && dec == lastAugerDec->getValueDouble ());
}

int
main (int argc, char **argv)
{
  Rts2DevAugerShooter *device = new Rts2DevAugerShooter (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}
