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
		virtual int doReadout ();

	private:
		rts2core::ConnTCP *commandConn;
		rts2core::ConnTCP *dataConn;

		int callCommand (const char *cmd);

		const char *azcamHost;
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
}

int AzCam::processOption (int opt)
{
	switch (opt)
	{
		case 'a':
			azcamHost = optarg;
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

	commandConn = new rts2core::ConnTCP (this, azcamHost, 123);
	dataConn = new rts2core::ConnTCP (this, azcamHost, 1234);

	commandConn->init ();
	dataConn->init ();

	if (getDebug())
	{
		commandConn->setDebug ();
		dataConn->setDebug ();
	}
	return 0;
}

int AzCam::callCommand (const char *cmd)
{
	char rbuf[200];

	// end character \r, 20 second wtime
	commandConn->writeRead (cmd, strlen(cmd), rbuf, 200, '\r', 20);

	return 0;
}

int AzCam::startExposure()
{
	callCommand ("exposure");
	return 0;
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
