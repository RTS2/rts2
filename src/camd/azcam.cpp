#include "camd.h"

#include "connection/tcp.h"

namespace rts2camd
{

class AzCam:public rts2camd::Camera
{
	public:
		AzCam (int argc, char **argv);
		virtual ~AzCam ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int startExposure ();
		virtual long isExposing ();
		virtual int doReadout ();

	private:
		rts2core::ConnTCP *commandConn;
		rts2core::ConnTCP *dataConn;

		char rbuf[200];

		int callCommand (const char *cmd);
		int callCommand (const char *cmd, double p1, const char *p2);
		int callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6);

		HostString *azcamHost;
};

}

using namespace rts2camd;

AzCam::AzCam (int argc, char **argv): Camera (argc, argv)
{
	azcamHost = NULL;

	addOption ('a', NULL, 1, "AZCAM hostname, hostname of the computer running AZCam");
}

AzCam::~AzCam ()
{
	delete dataConn;
	delete commandConn;
	delete azcamHost;
}

int AzCam::processOption (int opt)
{
	switch (opt)
	{
		case 'a':
			azcamHost = new HostString (optarg, "123");
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;	
}

int AzCam::initHardware()
{
	if (azcamHost == NULL)
	{
		logStream (MESSAGE_ERROR) << "you need to specify AZCAM host with -a argument" << sendLog;
		return -1;
	}

	commandConn = new rts2core::ConnTCP (this, azcamHost->getHostname (), azcamHost->getPort ());
	dataConn = new rts2core::ConnTCP (this, 1234);

	try
	{
		commandConn->init ();
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "cannot connect to " << azcamHost->getHostname () << ":" << azcamHost->getPort () << sendLog;
	}
	dataConn->init ();

	if (getDebug())
	{
		commandConn->setDebug ();
		dataConn->setDebug ();
	}

	addConnection (dataConn);

	return 0;
}

int AzCam::callCommand (const char *cmd)
{
	// end character \r, 20 second wtime
	int ret = commandConn->writeRead (cmd, strlen(cmd), rbuf, 200, '\r', 20);
	if (ret >= 0)
		rbuf[ret] = '\0';
	return ret > 0 ? (strncmp (rbuf, "OK", 2) == 0) : 0;
}

int AzCam::callCommand (const char *cmd, double p1, const char *p2)
{
	char buf[200];
	snprintf (buf, 200, "%s %f \'%s\'\r\n", cmd, p1, p2);
	return callCommand (buf);
}

int AzCam::callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6)
{
	char buf[200];
	snprintf (buf, 200, "%s %d %d %d %d %d %d\r\n", cmd, p1, p2, p3, p4, p5, p6);
	return callCommand (buf);
}

int AzCam::startExposure()
{
	int ret = callCommand ("setROI", getUsedX (), getUsedX () + getUsedWidth () - 1, getUsedY (), getUsedY () + getUsedHeight () - 1, binningHorizontal (), binningVertical ());
	if (ret)
		return ret;

	const char *imgType[3] = {"object", "dark", "zero"};
	return callCommand ("StartExposure", getExposure(), imgType[getExpType ()]);
}

long AzCam::isExposing ()
{
	int ret = callCommand ("GetExposureTimeRemaining\r\n");
	if (ret)
		return -1;
	char retConf[200];
	double retT;
	if (sscanf (rbuf, "%200s %lf", retConf, &retT) != 2)
		return -1;
	if (retT <= 0)
		return -2;
	return retT * 1000;
}

int AzCam::doReadout ()
{
	callCommand ("readout");
	return 0;
}

int main (int argc, char **argv)
{
	AzCam device (argc, argv);
	device.run ();
}
