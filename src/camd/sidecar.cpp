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

		/**
		 * Call method on server.
		 *
		 * @param method   method name
		 * @param p1       first parameter
		 * @param _is      string output (return of the call)
		 * @param wtime    wait time (in seconds)
		 */
		// this does not return anything. It only returns possible output of command in _is. You might want to parse
		// method output to get more, e.g. status of the call
		void callMethod (const char *method, int p1, std::istringstream **_is, int wtime = 10);

		/**
		 * Call method on server with 5 parameters of arbitary type.
		 *
		 * @see callMethod(cmd,p1,_is,wtime)
		 */
		template < typename t1, typename t2, typename t3, typename t4, typename t5 > void callMethod (const char *cmd, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, std::istringstream **_is, int wtime = 10)
		{
			std::ostringstream os;
			os << std::fixed << cmd << "(" << p1 << "," << p2 << "," << p3 << "," << p4 << "," << p5 << ")";
			sendCommand (os.str().c_str (), _is, wtime);
		}
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


	protected:
		/**
		 * Process -t option (Teledyne server IP).
		 */
		virtual int processOption (int in_opt);

		virtual int init ();
		virtual int initChips ();
		virtual int info ();

		// called when variable is changed
		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		virtual int startExposure ();
		virtual int doReadout ();
	
	private:
		int width;
		int height;

		HostString *sidecarServer;
		SidecarConn *sidecarConn;
		SidecarConn *imageRetrievalConn;

		// variables holders
		rts2core::ValueSelection *fsMode;

		rts2core::ValueInteger *nReset;
		rts2core::ValueInteger *nRead;
		rts2core::ValueInteger *nGroups;
		rts2core::ValueInteger *nDropFrames;
		rts2core::ValueInteger *nRamps;

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

void SidecarConn::callMethod (const char *method, int p1, std::istringstream **_is, int wtime)
{
	std::ostringstream os;
	os << method << "(" << p1 << ")";
	sendCommand (os.str ().c_str (), _is, wtime);
}

Sidecar::Sidecar (int in_argc, char **in_argv):Camera (in_argc, in_argv)
{
	sidecarServer = NULL;
	sidecarConn = NULL;
	imageRetrievalConn = NULL;

	createTempCCD ();
	createExpType ();

	// name for fsMode, as shown in rts2-mon, will be fs_mode, "mode of.. " is the comment.
	// false means that it will not be recorded to FITS
	// RTS2_VALUE_WRITABLE means you can change value from monitor
	// CAM_WORKING means the value can only be set if camera is not exposing or reading the image
	createValue (fsMode, "fs_mode", "mode of the chip exposure (Up The Ramp[0] vs Fowler[1])", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	fsMode->addSelVal ("0 option");
	fsMode->addSelVal ("1 option");

	createValue (nReset, "n_reset", "number of reset frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	createValue (nRead, "n_read", "number of read frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	createValue (nGroups, "n_groups", "number of groups", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	createValue (nDropFrames, "n_drop_frames", "number of drop frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	createValue (nRamps, "n_ramps", "number of ramps", false, RTS2_VALUE_WRITABLE, CAM_WORKING);

	width = 2048;
	height = 2048;

	addOption ('t', NULL, 1, "Teledyne host name and double colon separated port (port defaults to 5000)");
}

Sidecar::~Sidecar ()
{
	delete sidecarConn;
	delete imageRetrievalConn;
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
		// Images are scp'd back to linux rts2 machine via a python TCP server running on the windows machine.
		// The port address of this image retrieval server is one higher (5001) than the port of the
		// HxRG Socket Server (5000). 
		imageRetrievalConn = new SidecarConn (this, sidecarServer->getHostname (), sidecarServer->getPort ()+1);
		sidecarConn->init ();
		imageRetrievalConn->init ();
		sidecarConn->setDebug ();
		imageRetrievalConn->setDebug ();
		// initialize system..
		std::istringstream *is;
		sidecarConn->sendCommand ("Ping", &is);
		// sidecarConn->sendCommand ("Initialize1", &is);
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
	// sidecarConn->sendCommand ("GetConfig", &is);
	// parseConfig (is);
	// delete is;
	// sidecarConn->sendCommand ("GetTelemetry", &is, 120);
	// delete is;
	return Camera::info ();
}

int Sidecar::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	std::istringstream *is = NULL;
	if (old_value == fsMode)
	{
		// this will send SetFSMode(0) (or 1..) on sidecar connection
		sidecarConn->callMethod ("SetFSMode",new_value->getValueInteger (), &is);
		// if that will return anything, we shall process output found in is..can be done in callMethod
		delete is;
		return 0;
	}
	else if (old_value == nReset)
	{
		sidecarConn->callMethod ("SetRampParam", new_value->getValueInteger (), nRead->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
		delete is;
		return 0;
	}
	return 0;
}

int Sidecar::startExposure ()
{
	std::istringstream *is;
	sidecarConn->sendCommand ("AcquireRamp", &is);
	// Maybe have image retrieval command sent as part of doReadout below.
	imageRetrievalConn->sendCommand ("cklein@192.168.1.56:/home/cklein/Desktop/HxRG_Data", &is);
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
