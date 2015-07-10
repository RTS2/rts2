#include "camd.h"

class AzCam:public rts2camd::Camera
{
	public:
		AzCam();
		virtual ~AzCam();

	protected:
		virtual int startExposure ();
		virtual int doReadout ();

	private:
		rts2core::ConnTCP *commandConn;
		rts2core::ConnTCP *dataConn;

		const char *azcamHost;
}

AzCam::AzCam ()
{
	azcamHost = NULL;

	addOption ('a', NULL, 1, "AZCAM hostname, hostname of the computer running AZCam");
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
		logStream (LOG_ERROR) << "you need to specify AZCAM host with -a argument" << sendLog;
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
}

int AzCam::callCommand (const char *cmd)
{
	char rbuf[200];

	// end character \r, 20 second wtime
	commandConn->writeRead (cmd, strlen(cmd), rbuf, 200, '\r', 20);
}

int AzCam::startExposure()
{
	callCommand ("exposure");
}

int AzCam::doReadout ()
{
	callCommand ("readout");
}
