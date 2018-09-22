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
		int getRecvs() {return recvs;}
		bool imgWasRecvd() {return wasRecvd;}

	private:
		ssize_t dataSize;
		size_t headerSize;
		char header[257];
		int outFile;
		int recvs;
		bool wasRecvd;
};

AzCam3DataConn::AzCam3DataConn (rts2core::Block *_master, int _port):ConnTCP (_master, _port)
{
	dataSize = -1;
	headerSize = 0;
	memset (header, 0, sizeof (header));
	outFile = 0;
	recvs = 0;
	wasRecvd = false;
}

int AzCam3DataConn::receive (rts2core::Block *block)
{
	recvs++;
	if (isConnState (CONN_DELETE))
		return -1;
	if (sock >= 0 && block->isForRead (sock))
	{
		int rec;
		if (isConnState (CONN_CONNECTING))
		{
			return acceptConn ();
		}

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
				headerSize = 256+1;
			}
			return 0;
		}

		static char rbuf[2048*4];

		rec = recv (sock, rbuf, 2048*4, 0);
		
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
				wasRecvd = true;
				return rec;
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
		virtual int shiftStoreEnd (rts2core::Connection *conn, int shift, float exptime);

	private:
		rts2core::ConnTCP *commandConn;
		AzCam3DataConn *dataConn;

		rts2core::ValueFloat *exposureRemaining;
		rts2core::ValueLong *pixelsRemaining;
		rts2core::ValueString *lastImagePath;
		

		rts2core::ValueBool *parShiftFocus;
		rts2core::ValueLong *parShiftNexposures;
		rts2core::ValueLong *parShiftFocusSteps;
		rts2core::ValueLong *parShiftDetShifts;
		rts2core::ValueFloat *parShiftExposureTime;

		char rbuf[200];

		int callCommand (const char *cmd);
		int callCommand (const char *cmd, int p1);
		int callCommand (const char *cmd, double p1);
		int callCommand (const char *cmd, const char *p1);
		int callCommand (const char *cmd, double p1, const char *p2, const char *p3);
		int callExposure (const char *cmd, double p1, const char *p2);
		int callCommand (const char *cmd, int p1, int p2, int p3, int p4, int p5, int p6);
		int callShiftExposure( );
		int setBinning(int binx, int biny);
		int setBinning();

		int callArg (const char *cmd);

		int setCamera (const char *name, const char *value);
		int setCamera (const char *name, int value);

		long getLong (const char *cmd);
		double getDouble (const char *cmd);

		int setupDataConnection ();

		HostString *azcamHost;
		const char *hostname;
		float lastShiftExpTime;

	protected:
		virtual void valueChanged(rts2core::Value *);
};

}

using namespace rts2camd;

AzCam3::AzCam3 (int argc, char **argv): Camera (argc, argv)
{
	azcamHost = NULL;
	hostname = NULL;

	lastShiftExpTime = NAN;

	createExpType ();
	createShiftStore ();

	createValue (exposureRemaining, "exposure_rem", "[s] AZCam remaining exposure time", false);
	createValue (pixelsRemaining, "pixels_rem", "AZCam remaining readout pixels", false);
	createValue (lastImagePath, "last_img_path", "Path to most recent image", false, RTS2_VALUE_WRITABLE);
	



	//par shift focus variables
	createValue (parShiftFocus, "shiftfocus", "Do an azcam focus run", false, RTS2_VALUE_WRITABLE);
	createValue (parShiftNexposures, "SHIFT_N", "Number of exposure to take during shift focus", false, RTS2_VALUE_WRITABLE);
	createValue (parShiftFocusSteps, "SHIFT_FO", "Number of steps to move in focus during shift focus", false, RTS2_VALUE_WRITABLE);
	createValue (parShiftDetShifts, "SHIFT_PX", "Number of pixels to shift during shift focus", false, RTS2_VALUE_WRITABLE);
	createValue (parShiftExposureTime, "SHIFT_EX", "Time in seconds of each exposure during shift focus. ", false, RTS2_VALUE_WRITABLE);

	//handy defaults
	parShiftFocus->setValueBool(false);
	parShiftNexposures->setValueLong( 7 );
	parShiftFocusSteps->setValueLong( 45 );
	parShiftDetShifts->setValueLong( 50  );
	parShiftExposureTime->setValueDouble( 10.0 );
	//end par shift

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

void AzCam3::valueChanged ( rts2core::Value *changed_value )
{
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

	int ret = callCommand ("abort\r\n");
	if (ret)
		return ret;

	callCommand ("readout_abort\r\n");

	initCameraChip (101, 101, 0, 0);
	//Default binning should be in the config. 
	addBinning2D(3, 3);

	return initChips ();
}

int AzCam3::callCommand (const char *cmd)
{
	// end character \r, 20 second wtime
	// AzCam now uses rts2. syntax for our commands
	// 8/28/2018
	char rts2cmd[50];
	sprintf(rts2cmd, "rts2.%s", cmd );
	int ret = commandConn->writeRead (rts2cmd, strlen(rts2cmd), rbuf, 200, '\r', 20, false);
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
	snprintf (buf, 200, "%s %d\r\n", cmd, p1);
	return callCommand (buf);
}

int AzCam3::callCommand (const char *cmd, double p1)
{
	char buf[200];
	snprintf (buf, 200, "%s %f\r\n", cmd, p1);
	return callCommand (buf);
}

int AzCam3::callCommand (const char *cmd, const char *p1)
{
	char buf[200];
	snprintf (buf, 200, "%s '%s'\r\n", cmd, p1);
	return callCommand (buf);
}

int AzCam3::callCommand (const char *cmd, double p1, const char *p2, const char *p3)
{
	char buf[200];
	snprintf (buf, 200, "%s %f '%s' '%s' \r\n", cmd, p1, p2, p3);
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


int AzCam3::setBinning(int binx, int biny)
{
	char buf[200];

	snprintf(buf, 200, "binning %d %d\r\n", binx, biny );
	return callCommand(buf);
}

int AzCam3::setBinning()
{
	char buf[200];

	snprintf(buf, 200, "binning %d %d\r\n", binningHorizontal(), binningVertical() );
	return callCommand(buf);
}

int AzCam3::callShiftExposure (  )
{

	logStream (MESSAGE_INFO) << "Shift Exposure called. " << sendLog;
	char buf[200];
	snprintf (buf, 200, "focuser_set_pars %f %ld %ld %ld \r\n", 
		parShiftExposureTime->getValueFloat(), 
		parShiftNexposures->getValueLong(), 
		parShiftFocusSteps->getValueLong(), 
		parShiftDetShifts->getValueLong() );
	callCommand(buf);

	try
	{
		
		logStream (MESSAGE_INFO) << "Sending focus.run right ... now " << sendLog;
		commandConn->sendData ("rts2.focus.run\r\n");
		return 0;
	}
	catch (rts2core::ConnError err)
	{
		logStream (MESSAGE_ERROR) << "cannot start shift store exposure " << err << sendLog;
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
	snprintf (buf, 8 + len, "set %s %s\r\n", name, value);
	
	logStream (MESSAGE_INFO) << "sending " <<  buf << sendLog;
	return callArg (buf);
}

int AzCam3::setCamera (const char *name, int value)
{
	size_t len = strlen (name) + 20;
	char buf[8 + len];
	snprintf (buf, 8 + len, "set %s %d\r\n", name, value);
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

int AzCam3::setupDataConnection ()
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

	int ret = setCamera ("remoteimageserverflag", 1);
	if (ret)
		return ret;
	
	ret = setCamera ("remoteimageserverhost", hostname);
	if (ret)
		return ret;

	ret = setCamera ("remoteimageserverport", dataConn->getPort ());
	return ret;
}

int AzCam3::startExposure()
{
	int ret = setupDataConnection ();
	if (ret)
		return ret;

	//ret = callCommand ("exposure.set_roi", getUsedX (), getUsedX () + getUsedWidth () - 1, getUsedY (), getUsedY () + getUsedHeight () - 1, binningHorizontal (), binningVertical ());
	ret = setBinning( binningHorizontal(), binningVertical() );
	if (ret)
		return ret;

	const char *imgType[3] = {"object", "dark", "zero"};

	if( parShiftFocus->getValueBool() )
	{
		callShiftExposure( );
		ret = 0;
	}
	else
	{
		ret = callCommand ( "expose", getExposure(), imgType[getExpType ()], objectName->getValue() );
	}

	if ( ret )
		return ret;
	sleep ( 5 );
	return 0;

}

int AzCam3::stopExposure ()
{
	callCommand ("abort\r\n");
	callCommand ("controller.readout_abort\r\n");
	return Camera::stopExposure ();
}

long AzCam3::isExposing ()
{


	//logStream (MESSAGE_ERROR) << "isExposing called"<<sendLog;
	try
	{
		if ( parShiftFocus->getValueBool())
		{
			exposureRemaining->setValueFloat (getDouble ("timeleft\r\n"));
			sendValueAll (exposureRemaining);
			if (exposureRemaining->getValueFloat () > 0)
				return exposureRemaining->getValueFloat () * USEC_SEC;

			// else exposureRemaining is 0..or negative in case of AzCam bug..
			long camExp = Camera::isExposing ();
			if (camExp > USEC_SEC)
				return camExp;
		}
		else
		{
			if( dataConn->imgWasRecvd() )
			{
				
				return -2;
			}
			else
			{

				//logStream (MESSAGE_ERROR) << "datasize " << dataConn->getDataSize() << " recvs: "<< dataConn->getRecvs()<<sendLog;
				//check again in a second
				return USEC_SEC;
			}

		}
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << "error: " << er <<sendLog;

		return -1;
	}
	if (getState () & CAM_SHIFT)
		return -5;
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
				if (!(getState () & CAM_SHIFT))
				{
					if (!parShiftFocus->getValueBool()) // normal exposure
					{
						pixelsRemaining->setValueLong (getLong ("pixels_remaining\r\n"));


						sendValueAll (pixelsRemaining);
						
						//logStream (MESSAGE_ERROR) << "pixels remaining:"<< pixelsRemaining->getValueLong() <<sendLog;
					}
					else
					{//Shift detector charge for focus expsoure.
						removeConnection (dataConn);
						dataConn = NULL;
				                fitsDataTransfer ("/tmp/m.fits");
						//Don't assume par shift for the next
						//exposure. 
						parShiftFocus->setValueBool(false);
						sendValueAll(parShiftFocus);
						return -2;
					}
				}
				
				return USEC_SEC;
			}
			catch (rts2core::Error &er)
			{
				logStream (MESSAGE_ERROR) << "doReadout " << er <<sendLog;
				return -1;
			}
		}
		else if (dataConn->getDataSize () > 0)
		{
			return 10;
		}
		if (getState () & CAM_SHIFT)
		{
			std::istringstream *is;
			try
			{
				commandConn->receiveData (&is, 20, '\r');
				if (is->str ().substr (0, 2) != "OK")
				{
					logStream (MESSAGE_ERROR) << "invalid response to readout command " << is->str () << sendLog;
					delete is;
					return -1;
				}
				delete is;
			}
			catch (rts2core::ConnTimeoutError &ctoe)
			{
				logStream (MESSAGE_ERROR) << "cannot receive reply to readout command" << sendLog;
			}
		}
		removeConnection (dataConn);
		dataConn = NULL;
		fitsDataTransfer ("/tmp/m.fits");
		return -2;
	}
	return -1;
}

int AzCam3::shiftStoreStart (rts2core::Connection *conn, float exptime)
{
	int ret = setupDataConnection ();
	if (ret)
		return ret;
	
	ret = callCommand ("exposure.set_roi", getUsedX (), getUsedX () + getUsedWidth () - 1, getUsedY (), getUsedY () + getUsedHeight () - 1, binningHorizontal (), binningVertical ());
	if (ret)
		return ret;

	
	//ret = callCommand ("exposure.expose1", getExposure(), imgType[getExpType ()], objectName->getValue());
	//ret = callCommand("focus.run\r\n");
	commandConn->sendData("focus.run\r\n");

	if (ret)
		return ret;
	sleep (5);
	return 0;
}


int AzCam3::shiftStoreShift (rts2core::Connection *conn, int shift, float exptime)
{
	//int ret;
	removeConnection( dataConn );
	dataConn = NULL;
	fitsDataTransfer("/tmp/m.fits");
	return Camera::shiftStoreShift (conn, shift, exptime);
}

int AzCam3::shiftStoreEnd (rts2core::Connection *conn, int shift, float exptime)
{
	//int ret;
	return Camera::shiftStoreEnd (conn, shift, exptime);
}


int main (int argc, char **argv)
{
	AzCam3 device (argc, argv);
	device.run ();
}
