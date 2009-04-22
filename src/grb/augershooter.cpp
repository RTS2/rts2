/**
 * Receive Auger socket shower info, send now request to executor.
 *
 * @author petr
 */

#include "augershooter.h"
#include "../utils/rts2command.h"

using namespace rts2too;

AugerShooter::AugerShooter (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_AUGERSH, "AUGRSH")
{
	shootercnn = NULL;
	port = 1240;
	addOption ('s', "shooter_port", 1,
		"port on which to listen for auger connection");

	createValue (lastAugerDate, "last_date", "date of last shower", false);
	createValue (lastAugerRa, "last_ra", "RA of last shower", false,
		RTS2_DT_RA);
	createValue (lastAugerDec, "last_dec", "DEC of last shower", false,
		RTS2_DT_DEC);
}


AugerShooter::~AugerShooter (void)
{
}


int
AugerShooter::processOption (int in_opt)
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
AugerShooter::init ()
{
	int ret;
	ret = Rts2DeviceDb::init ();
	if (ret)
		return ret;

	Rts2Config *config = Rts2Config::instance ();
	double minEnergy;
	minEnergy = 20;
	config->getDouble ("augershooter", "minenergy", minEnergy);

	int maxTime = 600;
	config->getInteger ("augershooter", "maxtime", maxTime);

	shootercnn = new ConnShooter (port, this, minEnergy, maxTime);

	ret = shootercnn->init ();

	if (ret)
		return ret;

	addConnection (shootercnn);

	return ret;
}


int
AugerShooter::newShower (double lastDate, double ra, double dec)
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
AugerShooter::wasSeen (double lastDate, double ra, double dec)
{
	return (fabs (lastDate - lastAugerDate->getValueDouble ()) < 5
		&& ra == lastAugerRa->getValueDouble ()
		&& dec == lastAugerDec->getValueDouble ());
}


int
main (int argc, char **argv)
{
	AugerShooter device = AugerShooter (argc, argv);
	return device.run ();
}
