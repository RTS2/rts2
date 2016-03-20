#include <fcntl.h>

#include "camd.h"

#include "connection/tcp.h"

namespace rts2camd
{

class AzCamDataConn:public rts2core::ConnTCP
{
	public:
		AzCamDataConn (rts2core::Block *_master, int _port);

		virtual int receive (fd_set * readset);

		size_t getDataSize () { return dataSize; } 

	private:
		size_t dataSize;
		size_t headerSize;
		char header[257];
		int outFile;
};

AzCamDataConn::AzCamDataConn (rts2core::Block *_master, int _port):ConnTCP (_master, _port)
{
	dataSize = -1;
	headerSize = 0;
	memset (header, 0, sizeof (header));
	outFile = 0;
}

int AzCamDataConn::receive (fd_set *readset)
{
	if (isConnState (CONN_DELETE))
		return -1;
	if (sock >= 0 && FD_ISSET (sock, readset))
	{
		int rec;
		if (isConnState (CONN_CONNECTING))
			return acceptConn ();

		if (headerSize < 256)
		{
			rec = recv (sock, header + headerSize, 256 - headerSize, 0);
			if (rec < 0)
				return -1;
			headerSize += rec;
			if (headerSize == 256)
			{
				std::cerr << "header " << header << std::endl;
				sscanf (header, "%ld", &dataSize);
			}
			return 0;
		}

		static char rbuf[2048];

		rec = recv (sock, rbuf, 2048, 0);

		if (rec > 0)
		{
			if (outFile == 0)
			{
				outFile = open ("/tmp/m.fits", O_CREAT | O_TRUNC | O_WRONLY, 00666);
				if (outFile < 0)
					return -1;
			}
			write (outFile, rbuf, rec);
			fsync (outFile);

			dataSize -= rec;
			if (dataSize <= 0)
			{
				close (outFile);
				dataSize = 0;
				headerSize = 0;
				return 0;
			}
		}
		return rec;
	}
	return 0;
}

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
		AzCamDataConn *dataConn;

		char rbuf[200];

		int callCommand (const char *cmd);
		int callExposure (const char *cmd, double p1, const char *p2);
		int callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6);

		int callArg (const char *cmd);

		int setCamera (const char *name, const char *value);
		int setCamera (const char *name, int value);

		HostString *azcamHost;
		const char *hostname;
};

}

using namespace rts2camd;

AzCam::AzCam (int argc, char **argv): Camera (argc, argv)
{
	azcamHost = NULL;
	hostname = NULL;

	createExpType ();

	addOption ('a', NULL, 1, "AZCAM hostname, hostname of the computer running AZCam");
	addOption ('n', NULL, 1, "local hostname, hostname of the computer running RTS2");
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
			azcamHost = new HostString (optarg, "2402");
			break;
		case 'n':
			hostname = optarg;
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

	if (hostname == NULL)
	{
		logStream (MESSAGE_ERROR) << "you need to specify local hostname with -n argument" << sendLog;
		return -1;
	}

	commandConn = new rts2core::ConnTCP (this, azcamHost->getHostname (), azcamHost->getPort ());
	dataConn = NULL;

	try
	{
		commandConn->init ();
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "cannot connect to " << azcamHost->getHostname () << ":" << azcamHost->getPort () << sendLog;
	}

	if (getDebug())
	{
		commandConn->setDebug ();
	}

	int ret = callCommand ("exposure.Abort()\r\n");
	if (ret)
		return ret;

	initCameraChip (101, 101, 0, 0);

	return initChips ();
}

int AzCam::callCommand (const char *cmd)
{
	// end character \r, 20 second wtime
	int ret = commandConn->writeRead (cmd, strlen(cmd), rbuf, 200, '\r', 20, false);
	if (ret >= 0)
	{
		rbuf[ret] = '\0';
		return (strncmp (rbuf, "OK", 2) == 0) ? 0 : -1;
	}
	return -1;
}

int AzCam::callExposure (const char *cmd, double p1, const char *p2)
{
	char buf[200];
	snprintf (buf, 200, "%s(%f,\'%s\','test')\r\n", cmd, p1, p2);
	try
	{
		commandConn->sendData (buf);
		return 0;
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "cannot start exposure " << err << sendLog;
		return -1;
	}
}

int AzCam::callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6)
{
	char buf[200];
	snprintf (buf, 200, "%s %d %d %d %d %d %d\r\n", cmd, p1, p2, p3, p4, p5, p6);
	return callCommand (buf);
}

int AzCam::callArg (const char *cmd)
{
	// end character \r, 20 second wtime
	int ret = commandConn->writeRead (cmd, strlen(cmd), rbuf, 200, '\r', 20, false);
	return ret >= 0 ? 0 : -1;
}

int AzCam::setCamera (const char *name, const char *value)
{
	size_t len = strlen (name) + strlen (value);
	char buf[8 + len];
	snprintf (buf, 8 + len, "%s = '%s'\r\n", name, value);
	return callArg (buf);
}

int AzCam::setCamera (const char *name, int value)
{
	size_t len = strlen (name) + 20;
	char buf[8 + len];
	snprintf (buf, 8 + len, "%s = %d\r\n", name, value);
	return callArg (buf);
}

int AzCam::startExposure()
{
	if (dataConn)
	{
		removeConnection (dataConn);
		delete dataConn;
	}

	dataConn = new AzCamDataConn (this, 0);
	if (getDebug ())
		dataConn->setDebug ();
	dataConn->init ();

	addConnection (dataConn);

	setFitsTransfer ();

	int ret = setCamera ("exposure.RemoteImageServer", 1);
	if (ret)
		return ret;

	ret = setCamera ("exposure.RemoteImageServerHost", hostname);
	if (ret)
		return ret;

	ret = setCamera ("exposure.RemoteImageServerPort", dataConn->getPort ());
	if (ret)
		return ret;

//	int ret = callCommand ("setROI", getUsedX (), getUsedX () + getUsedWidth () - 1, getUsedY (), getUsedY () + getUsedHeight () - 1, binningHorizontal (), binningVertical ());
//	if (ret)
//		return ret;

	const char *imgType[3] = {"object", "dark", "zero"};
	return callExposure ("exposure.Expose", getExposure(), imgType[getExpType ()]);
}

long AzCam::isExposing ()
{
	if (dataConn)
	{
		if (dataConn->getDataSize () >= 0)
			return -2;
		return 1000;
	}
	return -1;
}

int AzCam::doReadout ()
{
	if (dataConn)
	{
		if (dataConn->getDataSize () > 0)
			return 10;
		fitsDataTransfer ("/tmp/m.fits");
		return -2;
	}
	return -1;
}

int main (int argc, char **argv)
{
	AzCam device (argc, argv);
	device.run ();
}
