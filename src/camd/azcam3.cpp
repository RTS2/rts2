/*
 * Driver for Arizona Camera (TCP-IP server) CCD.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fcntl.h>
#include "camd.h"
#include "connection/tcp.h"

namespace rts2camd
{

class AzCam3DataConn:public rts2core::ConnTCP
{
	public:
		AzCam3DataConn (rts2core::Block *_master, int _port);

		virtual int receive (rts2core::Block *block);

		ssize_t getDataSize () { return dataSize; }

	private:
		ssize_t dataSize;
		size_t headerSize;
		char header[257];
		int outFile;
};

AzCam3DataConn::AzCam3DataConn (rts2core::Block *_master, int _port):ConnTCP (_master, _port)
{
	dataSize = -1;
	headerSize = 0;
	memset (header, 0, sizeof (header));
	outFile = 0;
}

int AzCam3DataConn::receive (rts2core::Block *block)
{
	if (isConnState (CONN_DELETE))
		return -1;
	if (sock >= 0 && block->isForRead (sock))
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

class AzCam3:public rts2camd::Camera
{
	public:
		AzCam3 (int argc, char **argv);
		virtual ~AzCam3 ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int startExposure ();
		virtual int stopExposure ();
		virtual long isExposing ();
		virtual int doReadout ();

		virtual int shiftStoreStart (rts2core::Connection *conn, float exptime);
		virtual int shiftStoreShift (rts2core::Connection *conn, int shift, float exptime);
		virtual int shiftStoreEnd (rts2core::Connection *conn);

	private:
		rts2core::ConnTCP *commandConn;
		AzCam3DataConn *dataConn;

		rts2core::ValueFloat *exposureRemaining;
		rts2core::ValueLong *pixelsRemaining;

		char rbuf[200];

		int callCommand (const char *cmd);
		int callCommand (const char *cmd, int p1);
		int callCommand (const char *cmd, double p1);
		int callCommand (const char *cmd, double p1, const char *p2, const char *p3);
		int callExposure (const char *cmd, double p1, const char *p2);
		int callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6);

		int callArg (const char *cmd);

		int setCamera (const char *name, const char *value);
		int setCamera (const char *name, int value);

		long getLong (const char *cmd);
		double getDouble (const char *cmd);

		HostString *azcamHost;
		const char *hostname;
};

}

using namespace rts2camd;

AzCam3::AzCam3 (int argc, char **argv): Camera (argc, argv)
{
	azcamHost = NULL;
	hostname = NULL;

	createExpType ();
	createShiftStore ();

	createValue (exposureRemaining, "exposure_rem", "[s] AZCam remaining exposure time", false);
	createValue (pixelsRemaining, "pixels_rem", "AZCam remaining readout pixels", false);

	addOption ('a', NULL, 1, "AZCAM hostname, hostname of the computer running AZCam");
	addOption ('n', NULL, 1, "local hostname, hostname of the computer running RTS2");
}

AzCam3::~AzCam3 ()
{
	delete dataConn;
	delete commandConn;
	delete azcamHost;
}

int AzCam3::processOption (int opt)
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

int AzCam3::initHardware()
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

	callCommand ("controller.ReadoutAbort()\r\n");

	initCameraChip (101, 101, 0, 0);

	return initChips ();
}

int AzCam3::callCommand (const char *cmd)
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

int AzCam3::callCommand (const char *cmd, int p1)
{
	char buf[200];
	snprintf (buf, 200, "%s(%d)\r\n", cmd, p1);
	return callCommand (buf);
}

int AzCam3::callCommand (const char *cmd, double p1)
{
	char buf[200];
	snprintf (buf, 200, "%s(%f)\r\n", cmd, p1);
	return callCommand (buf);
}

int AzCam3::callCommand (const char *cmd, double p1, const char *p2, const char *p3)
{
	char buf[200];
	snprintf (buf, 200, "%s(%f,'%s','%s')\r\n", cmd, p1, p2, p3);
	return callCommand (buf);
}

int AzCam3::callExposure (const char *cmd, double p1, const char *p2)
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

int AzCam3::callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6)
{
	char buf[200];
	snprintf (buf, 200, "%s %d %d %d %d %d %d\r\n", cmd, p1, p2, p3, p4, p5, p6);
	return callCommand (buf);
}

int AzCam3::callArg (const char *cmd)
{
	// end character \r, 20 second wtime
	int ret = commandConn->writeRead (cmd, strlen(cmd), rbuf, 200, '\r', 20, false);
	return ret >= 0 ? 0 : -1;
}

int AzCam3::setCamera (const char *name, const char *value)
{
	size_t len = strlen (name) + strlen (value);
	char buf[8 + len];
	snprintf (buf, 8 + len, "%s = '%s'\r\n", name, value);
	return callArg (buf);
}

int AzCam3::setCamera (const char *name, int value)
{
	size_t len = strlen (name) + 20;
	char buf[8 + len];
	snprintf (buf, 8 + len, "%s = %d\r\n", name, value);
	return callArg (buf);
}

long AzCam3::getLong (const char *cmd)
{
	callCommand (cmd);
	if (strncmp (rbuf, "OK ", 3) != 0)
		throw rts2core::Error ("invalid reply");
	return atol (rbuf + 3);
}

double AzCam3::getDouble (const char *cmd)
{
	callCommand (cmd);
	if (strncmp (rbuf, "OK ", 3) != 0)
		throw rts2core::Error ("invalid reply");
	return atof (rbuf + 3);
}

int AzCam3::startExposure()
{
	if (dataConn)
	{
		removeConnection (dataConn);
		delete dataConn;
	}

	dataConn = new AzCam3DataConn (this, 0);
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
	return callCommand ("exposure.Expose1", getExposure(), imgType[getExpType ()], "RTS2");
}

int AzCam3::stopExposure ()
{
	callCommand ("exposure.Abort()\r\n");
	callCommand ("controller.ReadoutAbort()\r\n");
	return Camera::stopExposure ();
}

long AzCam3::isExposing ()
{
	try
	{
		exposureRemaining->setValueFloat (getDouble ("controller.UpdateExposureTimeRemaining()\r\n"));
		sendValueAll (exposureRemaining);
		if (exposureRemaining->getValueFloat () > 0)
			return exposureRemaining->getValueFloat () * USEC_SEC;
	}
	catch (rts2core::Error &er)
	{
		return -1;
	}
	return -2;
}

int AzCam3::doReadout ()
{
	if (dataConn)
	{
		if (dataConn->getDataSize () == -1)
		{
			try
			{
				pixelsRemaining->setValueLong (getLong ("controller.GetPixelsRemaining()\r\n"));
				sendValueAll (pixelsRemaining);
				return 10000;
			}
			catch (rts2core::Error &er)
			{
				return -1;
			}
		}
		else if (dataConn->getDataSize () > 0)
			return 10;
		removeConnection (dataConn);
		dataConn = NULL;
		fitsDataTransfer ("/tmp/m.fits");
		return -2;
	}
	return -1;
}

int AzCam3::shiftStoreStart (rts2core::Connection *conn, float exptime)
{
	int ret = callCommand ("exposure.SetExposureTime", exptime);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "invalid return from exposure.SetExposureTime call: " << ret << sendLog;
		return -2;
	}
	ret = callCommand ("exposure.Begin()\r\n");
	if (ret)
		return -2;

	ret = callCommand ("exposure.Integrate()\r\n");
	if (ret)
		return -2;

	return Camera::shiftStoreStart (conn, exptime);
}

int AzCam3::shiftStoreShift (rts2core::Connection *conn, int shift, float exptime)
{
	int ret = callCommand ("exposure.SetExposureTime", exptime);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "invalid return from exposure.SetExposureTime call: " << ret << sendLog;
		return -2;
	}
	ret = callCommand ("exposure.Parshift", shift);
	if (ret)
		return -2;
	ret = callCommand ("exposure.Integrate()\r\n");
	return Camera::shiftStoreShift (conn, shift, exptime);
}

int AzCam3::shiftStoreEnd (rts2core::Connection *conn)
{
	int ret = callCommand ("exposure.Readout()\r\n");
	if (ret)
		return -2;
	ret = callCommand ("exposure.End()\r\n");
	if (ret)
		return -2;
	return Camera::shiftStoreEnd (conn);
}

int main (int argc, char **argv)
{
	AzCam3 device (argc, argv);
	device.run ();
}
