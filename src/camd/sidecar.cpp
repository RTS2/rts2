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
        rts2core::ValueInteger *nResets;
		rts2core::ValueInteger *nReads;
		rts2core::ValueInteger *nGroups;
		rts2core::ValueInteger *nDropFrames;
		rts2core::ValueInteger *nRamps;
		rts2core::ValueSelection *gain;
		rts2core::ValueSelection *ktcRemoval;
		rts2core::ValueSelection *warmTest;
		rts2core::ValueSelection *idleMode;
        rts2core::ValueSelection *enhancedClocking;
        
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
	fsMode->addSelVal ("0 Up The Ramp");
	fsMode->addSelVal ("1 Fowler Sampling");
	fsMode->setValueInteger(0);

	createValue (nResets, "n_resets", "number of reset frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	nResets->setValueInteger(0);
	createValue (nReads, "n_reads", "number of read frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	nReads->setValueInteger(1);
	createValue (nGroups, "n_groups", "number of groups", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	nGroups->setValueInteger(1);
	createValue (nDropFrames, "n_drop_frames", "number of drop frames", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	nDropFrames->setValueInteger(1);
	createValue (nRamps, "n_ramps", "number of ramps", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
    nRamps->setValueInteger(1);



    createValue (gain, "gain", "set the gain. 0-15", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	gain->addSelVal ("0   -3dB, small Cin");
	gain->addSelVal ("1    0dB, small Cin");
	gain->addSelVal ("2    3dB, small Cin");
	gain->addSelVal ("3    6dB, small Cin");
	gain->addSelVal ("4    6dB, large Cin");
	gain->addSelVal ("5    9dB, small Cin");
	gain->addSelVal ("6    9dB, large Cin");
	gain->addSelVal ("7   12dB, small Cin");
	gain->addSelVal ("8   12dB, large Cin");
	gain->addSelVal ("9   15dB, small Cin");
	gain->addSelVal ("10  15dB, large Cin");
	gain->addSelVal ("11  18dB, small Cin");
	gain->addSelVal ("12  18dB, large Cin");
	gain->addSelVal ("13  21dB, large Cin");
	gain->addSelVal ("14  24dB, large Cin");
	gain->addSelVal ("15  27dB, large Cin");
	gain->setValueInteger(8);

    createValue (ktcRemoval, "preamp_ktc_removal", "turn on (1) or off (0) the preamp KTC removal", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	ktcRemoval->addSelVal ("0 Off");
	ktcRemoval->addSelVal ("1 On");
	ktcRemoval->setValueInteger(1);
    
    createValue (warmTest, "warm_test", "set for warm (1) or cold (0)", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	warmTest->addSelVal ("0 Cold");
	warmTest->addSelVal ("1 Warm");
	warmTest->setValueInteger(1);
	
	// Not sure why, but changing the IdleModeOption in the rts2-mon does not affect a change
	// in the HxRG Socket Server. The SS does show receiving the TCP command, though.
	createValue (idleMode, "idle_mode", "do nothing (0), continuously reset (1), or continuously read-reset (2)", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	idleMode->addSelVal ("0 Do Nothing");
	idleMode->addSelVal ("1 Continuous Resets");
	idleMode->addSelVal ("2 Continuous Read-Resets");
	idleMode->setValueInteger(1);
	
	createValue (enhancedClocking, "clocking", "normal (0) or enhanced (1)", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	enhancedClocking->addSelVal ("0 Normal");
	enhancedClocking->addSelVal ("1 Enhanced");
	enhancedClocking->setValueInteger(0);
	
	width = 2048;
	height = 2048;

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
		// parse it..
		size_t pos = var.find('=');
		if (pos == std::string::npos)
		{
			logStream (MESSAGE_DEBUG) << "cannot find = in '" << var << "', ignoring this line" << sendLog;
			continue;
		}
		// variable name and variable value
		std::string vn = var.substr (0, pos - 1);
		std::string vv = var.substr (pos + 1);
		rts2core::Value *vp = getOwnValue (vn.c_str ());
		if (vp == NULL)
		{
			logStream (MESSAGE_DEBUG) << "cannot find value with name " << vn << ", creating it" << sendLog;
			rts2core::ValueDouble *vd;
			createValue (vd, vn.c_str (), "created by parseConfing", false);
			updateMetaInformations (vd);
			vp = vd;
		}
		vp->setValueCharArr (vv.c_str ());
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
	
    else if (old_value == nResets)
	{
		sidecarConn->callMethod ("SetRampParam", new_value->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
		delete is;
		return 0;
	}
	
    else if (old_value == nReads)
	{
		sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), new_value->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
		delete is;
		return 0;
	}

    else if (old_value == nGroups)
	{
		sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), nReads->getValueInteger (), new_value->getValueInteger (), nDropFrames->getValueInteger (), nRamps->getValueInteger (), &is);
		delete is;
		return 0;
	}

    else if (old_value == nDropFrames)
	{
		sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), new_value->getValueInteger (), nRamps->getValueInteger (), &is);
		delete is;
		return 0;
	}

    else if (old_value == nRamps)
	{
		sidecarConn->callMethod ("SetRampParam", nResets->getValueInteger (), nReads->getValueInteger (), nGroups->getValueInteger (), nDropFrames->getValueInteger (), new_value->getValueInteger (), &is);
		delete is;
		return 0;
	}

	else if (old_value == gain)
	{
		sidecarConn->callMethod ("SetGain",new_value->getValueInteger (), &is);
		delete is;
		return 0;
	}
	
	else if (old_value == ktcRemoval)
	{
		sidecarConn->callMethod ("SetKTCRemoval",new_value->getValueInteger (), &is);
		delete is;
		return 0;
	}
	
	else if (old_value == warmTest)
	{
		sidecarConn->callMethod ("SetWarmTest",new_value->getValueInteger (), &is);
		delete is;
		return 0;
	}
		
	else if (old_value == idleMode)
	{
		sidecarConn->callMethod ("SetIdleModeOption",new_value->getValueInteger (), &is);
		delete is;
		return 0;
	}

	else if (old_value == enhancedClocking)
	{
		sidecarConn->callMethod ("SetEnhancedClk",new_value->getValueInteger (), &is);
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
//   std::istringstream *is;
//    imageRetrievalConn->sendCommand ("cklein@192.168.1.56:/home/cklein/Desktop/HxRG_Data", &is);
//	int ret;
//	long usedSize = dataBufferSize;
//	if (usedSize > getWriteBinaryDataSize ())
//		usedSize = getWriteBinaryDataSize ();
//	for (int i = 0; i < usedSize; i += 2)
//	{
//		uint16_t *d = (uint16_t* ) (dataBuffer + i);
//		*d = i;
//	}
//	ret = sendReadoutData (dataBuffer, usedSize);
//	if (ret < 0)
//		return ret;

//	if (getWriteBinaryDataSize () == 0)
//		return -2;				 // no more data..
	return 0;					 // imediately send new data
}

int main (int argc, char **argv)
{
	Sidecar device = Sidecar (argc, argv);
	return device.run ();
}
