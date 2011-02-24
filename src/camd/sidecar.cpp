/* 
 * Teledyne SIDECAR driver. Needs IDL server to connect to SIDECAR hardware.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "camd.h"
#include "../utils/utilsfunc.h"
#include "../utils/conntcp.h"

namespace rts2camd
{


/**
 * Sidecar connection class. Utility class to handle connections to the
 * sidecar. Please note that you can easily create two instances (objects)
 * inside a single Camera, if you need it for synchronizing the commanding.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SidecarConn:public rts2core::ConnTCP
{
	public:
		SidecarConn (rts2core::Block * _master, std::string _hostname, int _port):ConnTCP (_master, _hostname.c_str (), _port) {}
		virtual ~SidecarConn () {}

		/**
		 * Send command over TCP/IP, get response.
		 *
		 * @param cmd    command to be send
		 * @param _is    reply from the device
		 * @param wtime  time to wait for reply in seconds
		 */
		void sendCommand (const char *cmd, std::istringstream **_is, int wtime = 10);

		void sendCommand (const char *cmd, double p1, std::istringstream **_is, int wtime = 10);
};

/**
 * Class for sidecar camera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Sidecar:public Camera
{
	public:
		Sidecar (int in_argc, char **in_argv);
		virtual ~Sidecar ();

		/**
		 * Process -t option (Teledyne server IP).
		 */
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initChips ();
		virtual int info ();

		virtual int startExposure ();
		virtual int doReadout ();
	
	private:
		int width;
		int height;

		HostString *sidecarServer;
		SidecarConn *sidecarConn;

		// variables holders
		rts2core::ValueSelection *fsMode;

		int parseConfig (std::istringstream *is);
};

};

using namespace rts2camd;

void SidecarConn::sendCommand (const char *cmd, std::istringstream **_is, int wtime)
{
	sendData (cmd);
	sendData ("\r\n");
	receiveData (_is, wtime, '\n');
}

void SidecarConn::sendCommand (const char *cmd, double p1, std::istringstream **_is, int wtime)
{
	std::ostringstream os;
	os << cmd << " " << p1;
	sendCommand (os.str ().c_str (), _is, wtime);
}

Sidecar::Sidecar (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	sidecarServer = NULL;
	sidecarConn = NULL;

	createTempCCD ();
	createExpType ();

	// name for fsMode, as shown in rts2-mon, will be fs_mode, "mode of.. " is the comment.
	// false means that it will not be recorded to FITS
	// RTS2_VALUE_WRITABLE means you can change value from monitor
	// CAM_WORKING means the value can only be set if camera is not exposing or reading the image
	createValue (fsMode, "fs_mode", "mode of the chip exposure", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	fsMode->addSelVal ("0 option");
	fsMode->addSelVal ("1 option");

	width = 200;
	height = 100;

	addOption ('t', NULL, 1, "Teledyne host name and double colon separated port (port defaults to 5000)");
}

Sidecar::~Sidecar ()
{
}

int Sidecar::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 't':
			sidecarServer = new HostString (optarg, "5000");
			break;
		default:
			return Camera::processOption (in_opt);
	}
	return 0;
}

int Sidecar::init ()
{
	int ret;
	ret = Camera::init ();
	if (ret)
		return ret;

	strcpy (ccdType, "SIDECAR");
	strcpy (serialNumber, "1");

	if (sidecarServer == NULL)
	{
		std::cerr << "Invalid port or IP address of Teledyne server" << std::endl;
		return -1;
	}

	try
	{
		sidecarConn = new SidecarConn (this, sidecarServer->getHostname (), sidecarServer->getPort ());
		sidecarConn->init ();
		sidecarConn->setDebug ();
		// initialize system..
		std::istringstream *is;
		sidecarConn->sendCommand ("Ping", &is);
		sidecarConn->sendCommand ("Initialize1", &is);
		delete is;
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	return initChips ();
}

int Sidecar::initChips ()
{
	initCameraChip (width, height, 0, 0);
	return Camera::initChips ();
}

int Sidecar::parseConfig (std::istringstream *is)
{
	while (!is->eof ())
	{
		std::string var;
		*is >> var;
		logStream (MESSAGE_DEBUG) << "got " << var << sendLog;
	}
	return 0;
}

int Sidecar::info ()
{
	std::istringstream *is;
	sidecarConn->sendCommand ("Ping", &is);
	delete is;
	sidecarConn->sendCommand ("GetConfig", &is);
	parseConfig (is);
	delete is;
	sidecarConn->sendCommand ("GetTelemetry", &is, 120);
	delete is;
	return Camera::info ();
}

int Sidecar::startExposure ()
{
	std::istringstream *is;
	sidecarConn->sendCommand ("Exposure ", getExposure (), &is);
	return 0;
}

int Sidecar::doReadout ()
{
	int ret;
	long usedSize = dataBufferSize;
	if (usedSize > getWriteBinaryDataSize ())
		usedSize = getWriteBinaryDataSize ();
	for (int i = 0; i < usedSize; i += 2)
	{
		uint16_t *d = (uint16_t* ) (dataBuffer + i);
		*d = i;
	}
	ret = sendReadoutData (dataBuffer, usedSize);
	if (ret < 0)
		return ret;

	if (getWriteBinaryDataSize () == 0)
		return -2;				 // no more data..
	return 0;					 // imediately send new data
}

int main (int argc, char **argv)
{
	Sidecar device = Sidecar (argc, argv);
	return device.run ();
}
