/**
 * Receive info from GCN via socket, put them to DB.
 *
 * Based on http://gcn.gsfc.nasa.gov/socket_demo.c
 * socket_demo     Ver: 3.29   23 Mar 05,
 * which is CVSed with GRBC. Only "active" satellite packets are processed.
 *
 * If new version of socket_demo.c show up, we need to invesigate
 * modifications to include it.
 *
 * @author petr
 */

#include "../utils/rts2command.h"
#include "grbd.h"

Rts2DevGrb::Rts2DevGrb (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_GRB, "GRB")
{
	gcncnn = NULL;
	gcn_host = NULL;
	gcn_port = -1;
	do_hete_test = 0;
	forwardPort = -1;
	addExe = NULL;
	execFollowups = 0;

	createValue (last_packet, "last_packet", "time from last packet", false);

	createValue (delta, "delta", "delta time from last packet", false);

	createValue (last_target, "last_target", "id of last GRB target", false);

	createValue (last_target_time, "last_target_time", "time of last target",
		false);

	createValue (execConnection, "exec", "exec connection", false);

	addOption ('s', "gcn_host", 1, "GCN host name");
	addOption ('p', "gcn_port", 1, "GCN port");
	addOption ('t', "test", 0,
		"process test notices (default to off - don't process them)");
	addOption ('f', "forward", 1, "forward incoming notices to that port");
	addOption ('a', "add_exe", 1,
		"execute that command when new GCN packet arrives");
	addOption ('U', "exec_followups", 0,
		"execute observation and add_exe script even for follow-ups without error box (currently Swift follow-ups of INTEGRAL and HETE GRBs)");
}


Rts2DevGrb::~Rts2DevGrb (void)
{
	delete[]gcn_host;
	delete[]addExe;
}


int
Rts2DevGrb::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 's':
			gcn_host = new char[strlen (optarg) + 1];
			strcpy (gcn_host, optarg);
			break;
		case 'p':
			gcn_port = atoi (optarg);
			break;
		case 't':
			do_hete_test = 1;
			break;
		case 'f':
			forwardPort = atoi (optarg);
			break;
		case 'a':
			addExe = optarg;
			break;
		case 'U':
			execFollowups = 1;
			break;
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevGrb::reloadConfig ()
{
	int ret;
	Rts2Config *config;
	ret = Rts2DeviceDb::reloadConfig ();
	if (ret)
		return ret;

	config = Rts2Config::instance ();

	// get some default, if we cannot get them from command line
	if (!gcn_host)
	{
		std::string gcn_h;
		ret = config->getString ("grbd", "server", gcn_h);
		if (ret)
			return -1;
		gcn_host = new char[gcn_h.length () + 1];
		strcpy (gcn_host, gcn_h.c_str ());
	}

	if (gcn_port < 0)
	{
		ret = config->getInteger ("grbd", "port", gcn_port);
		if (ret)
			return -1;
	}

	// try to get exe from config
	if (!addExe)
	{
		std::string conf_addExe;
		ret = config->getString ("grbd", "add_exe", conf_addExe);
		if (!ret)
		{
			addExe = new char[conf_addExe.length () + 1];
			strcpy (addExe, conf_addExe.c_str ());
		}
	}
	if (gcncnn)
	{
		deleteConnection (gcncnn);
		delete gcncnn;
	}
	// add connection..
	gcncnn =
		new Rts2ConnGrb (gcn_host, gcn_port, do_hete_test, addExe, execFollowups,
		this);
	// wait till grb connection init..
	while (1)
	{
		ret = gcncnn->init ();
		if (!ret)
			break;
		logStream (MESSAGE_ERROR)
			<< "Rts2DevGrb::init cannot init conngrb, sleeping for 60 sec" <<
			sendLog;
		sleep (60);
		if (getEndLoop ())
			return -1;
	}
	addConnection (gcncnn);

	return ret;
}


int
Rts2DevGrb::init ()
{
	int ret;
	ret = Rts2DeviceDb::init ();
	if (ret)
		return ret;

	// add forward connection
	if (forwardPort > 0)
	{
		int ret2;
		Rts2GrbForwardConnection *forwardConnection;
		forwardConnection = new Rts2GrbForwardConnection (this, forwardPort);
		ret2 = forwardConnection->init ();
		if (ret2)
		{
			logStream (MESSAGE_ERROR)
				<< "Rts2DevGrb::init cannot create forward connection, ignoring ("
				<< ret2 << ")" << sendLog;
			delete forwardConnection;
			forwardConnection = NULL;
		}
		else
		{
			addConnection (forwardConnection);
		}
	}
	return ret;
}


int
Rts2DevGrb::info ()
{
	last_packet->setValueDouble (gcncnn->lastPacket ());
	delta->setValueDouble (gcncnn->delta ());
	last_target->setValueString (gcncnn->lastTarget ());
	last_target_time->setValueDouble (gcncnn->lastTargetTime ());
	execConnection->setValueInteger (getOpenConnection ("EXEC") ? 1 : 0);
	return Rts2DeviceDb::info ();
}


void
Rts2DevGrb::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case RTS2_EVENT_GRB_PACKET:
			infoAll ();
			break;
	}
	Rts2DeviceDb::postEvent (event);
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
		logStream (MESSAGE_ERROR) <<
			"FATAL! No executor running to post grb ID " << tar_id << sendLog;
		return -1;
	}
	return 0;
}


int
Rts2DevGrb::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("test"))
	{
		int tar_id;
		if (conn->paramNextInteger (&tar_id) || !conn->paramEnd ())
			return -2;
		return newGcnGrb (tar_id);
	}
	return Rts2DeviceDb::commandAuthorized (conn);
}


int
main (int argc, char **argv)
{
	Rts2DevGrb grb = Rts2DevGrb (argc, argv);
	return grb.run ();
}
