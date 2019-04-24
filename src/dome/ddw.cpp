/* 
 * Driver for Digital DomeWorks controller.
 * Copyright (C) 2018 Petr Kubanek
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

#include "cupola.h"
#include "connection/serial.h"
#include "math.h"

using namespace rts2dome;

namespace rts2dome
{

typedef enum {IDLE, CLOSING, OPENING, MOVING, HOMING} cmdType;

class DDW:public Cupola
{
	public:
		DDW (int argc, char **argv);
		virtual ~DDW ();

		virtual int idle ();

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

		virtual int moveStart ();
		virtual long isMoving ();
		virtual int moveEnd ();

		virtual double getSlitWidth (double alt);

		long getAzDomeOffset(double az);
		long getDomeAzFromTargetAz(double targetAz);
		long getTargetAzFromDomeAz(double domeAz);
		float * getOffsetCoeff();

		/// remove this once the mount is implemented
		long getTargetAlt() { return 15; }


	private:
		rts2core::ConnSerial *sconn;
		const char *devFile;

		int parseInfo (bool V = true);
		int executeCmd (const char *cmd, cmdType newCmd);
		long inProgress (bool opening);
		cmdType cmdInProgress;

		rts2core::ValueInteger *z;
		rts2core::ValueInteger *shutter;
		rts2core::ValueInteger *dticks;

		//void setAzimuthTicks(int adaz) { setCurrentAz(getTargetAzFromDomeAz(359*(double)(adaz)/(double)(dticks->getValueInteger())), true); }

		long AzDomeOffsetCoeff[2][3];
		
};

}

DDW::DDW (int argc, char **argv):Cupola (argc, argv)
{
	sconn = NULL;
	devFile = "/dev/DDW";
	cmdInProgress = IDLE;

	addOption('f', NULL, 1, "path to device, usually /dev/DDW");

	setIdleInfoInterval(5);

	createValue(z, "Z", "Z progress value", false);
	createValue(shutter, "shutter", "DDW reported shutter state", false);
	shutter->setValueInteger(0);

	createValue(dticks, "dticks", "number of azimuth ticks", false);

	// Azimuth Dome offset coefficients derived for Lowell TiMo
	// using offset(az) = [1]*sin(az+[2])+[3]
	AzDomeOffsetCoeff[1][1] = 4.2772;
	AzDomeOffsetCoeff[1][2] = -21.510;
	AzDomeOffsetCoeff[1][3] = 11.053;	
	AzDomeOffsetCoeff[2][1] = 8.247;
	AzDomeOffsetCoeff[2][2] = -4.908;
	AzDomeOffsetCoeff[2][3] = 20.234;
	
}

DDW::~DDW ()
{
	delete sconn;
}

int DDW::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			devFile = optarg;
			break;
		default:
			return Cupola::processOption(opt);
	}
	return 0;
}

int DDW::initHardware ()
{
	sconn = new rts2core::ConnSerial(devFile, this, rts2core::BS9600,
									 rts2core::C8, rts2core::NONE, 25);
	sconn->setDebug(getDebug ());
	sconn->init();

	sconn->flushPortIO();

	info();

	sconn->flushPortIO();

	info();

	sconn->flushPortIO();

	info();

	return 0;
}

int DDW::info ()
{
	if (cmdInProgress != IDLE)
		return 0;

	sconn->writePort("GINF", 4);
	parseInfo ();
	return Cupola::info();
}

int DDW::idle ()
{
	if (cmdInProgress == HOMING)
	{
		if (inProgress(false) < 0)
			setIdleInfoInterval(5);
	}
	return Cupola::idle();
}

int DDW::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("stop"))
	{
		sconn->writePort("GINF", 4);
		return 0;	
	}
	else if (conn->isCommand ("home"))
	{
		if (cmdInProgress != IDLE && cmdInProgress != OPENING && cmdInProgress != MOVING)
			return -1;
		int rete = executeCmd("GHOM", HOMING);
		if (rete < 0)
		{
			cmdInProgress = IDLE;
			return -1;
		}
		setIdleInfoInterval(0.05);
		return 0;
	}	
	return Cupola::commandAuthorized (conn);
}

int DDW::startOpen ()
{
	if (shutter->getValueInteger() == 2)
		return 0;
	if (cmdInProgress != IDLE)
	{
		sconn->writePort("GINF", 4);
		parseInfo ();
		sconn->flushPortIO();
		char buf[200];
		sconn->readPort(buf, 200);

    	logStream(MESSAGE_WARNING) << "trying to open but " << buf << sendLog;
	}
	int rete = executeCmd ("GOPN", OPENING);
	// when homing is needed, R or L is returned
	switch (rete)
	{
		case 'O':
			return 0;
		case 'R':
			return 0;
		case 'L':
			return 0;
			//shutter->setValueInteger(0);
			//return 0;
		default:
			cmdInProgress = IDLE;
			return DEVDEM_E_HW;
	}
}

long DDW::isOpened ()
{
	if (cmdInProgress == MOVING)
	{
		return shutter->getValueInteger () == 2 ? -2 : 0;
	}
	if (cmdInProgress != OPENING)
	{
		if (info ())
			return -1;
		return shutter->getValueInteger () == 2 ? -2 : 0;
	}
	return inProgress (false);
}

int DDW::endOpen ()
{
	return 0;
}

int DDW::startClose ()
{
	if (cmdInProgress == CLOSING |
		(cmdInProgress != OPENING && shutter->getValueInteger() == 1))
		{
		//logStream(MESSAGE_WARNING) << "shutter already closing or closed; sutther state" << shutter << sendLog;
		return 0;
		}
		
	// stop any command in progress
	if (cmdInProgress != IDLE)
	{

		//logStream(MESSAGE_WARNING) << "aborting command in progress " << cmdInProgress << sendLog;
		sconn->writePort("GINF", 4);
		parseInfo ();
		sconn->flushPortIO();
		char buf[200];
		sconn->readPort(buf, 200);
	}

	int rete = executeCmd ("GCLS", CLOSING);
	// when homing is needed, R or L is returned
	//logStream(MESSAGE_WARNING) << "integer value received upon calling close command " << rete << sendLog;
		
	switch (rete)
	{
		case 'C':
   		   return 100;
		case 'R':
		   return 100;
		case 'L':
		   return 100;
		case 'Z':
		   return 100;
		case 'S':
		   return 100;
		//shutter->setValueInteger(0);
		//return 0;
		default:
			cmdInProgress = IDLE;
		    return DEVDEM_E_HW;
	}
}

long DDW::isClosed ()
{
	if (cmdInProgress != CLOSING)
	{
		if (info ())
			return -1;
		return shutter->getValueInteger () == 1 ? -2 : 0;
	}

	return inProgress (false);
}

int DDW::endClose ()
{
	return 0;
}

int DDW::moveStart ()
{
	// stop any command in progress
	if (cmdInProgress == MOVING)
	{
		sconn->writePort("GINF", 4);
		parseInfo ();
		sconn->flushPortIO();
		char buf[200];
		sconn->readPort(buf, 200);
	}
	if (cmdInProgress == OPENING || cmdInProgress == CLOSING)
	{
		logStream(MESSAGE_ERROR) << "dome busy; ignoring move command " << cmdInProgress << sendLog;
		return DEVDEM_E_HW;
	}
	
	char azbuf[5];
	sconn->flushPortIO();

	 if (getDomeAzFromTargetAz(getTargetAz()) < 0) {
		snprintf(azbuf, 5, "G%03d",
				 (int) round(360+getDomeAzFromTargetAz(getTargetAz()))); 

		//logStream(MESSAGE_WARNING) << "Azimut after transformation:" << (int) round(360+getDomeAzFromTargetAz(getTargetAz())) << sendLog;
}
	else {
		snprintf(azbuf, 5, "G%03d",
				 (int) round(getDomeAzFromTargetAz(getTargetAz())));
		//logStream(MESSAGE_WARNING) << "Azimut after transformation:" << (int) round(getDomeAzFromTargetAz(getTargetAz())) << sendLog;
}

/*	snprintf(azbuf, 5, "G%03d", (int) round(getTargetAz())); */
	
	int azret = executeCmd(azbuf, MOVING);
	if (azret == 'R' || azret == 'L')
	{
		return Cupola::moveStart();
	}
	if (azret == 'V')
	{
		parseInfo();
		cmdInProgress = IDLE;
		return 0;
	}
	logStream(MESSAGE_WARNING) << "unknown azimuth character " << (char) azret << sendLog;
	cmdInProgress = IDLE;
	return DEVDEM_E_HW;
}

long DDW::isMoving ()
{
	if (cmdInProgress != MOVING)
		return -1;
	return inProgress(false);
}

int DDW::moveEnd ()
{
	return 0;
}

double DDW::getSlitWidth (double alt)
{
	return 5;
}

int DDW::parseInfo (bool V)
{
	shutter->setValueInteger(0);

	char buf[200];
	char *bp = buf;
	memset (buf, 0, sizeof(buf));

	if (sconn->readPort (buf, 200, '\r') == -1)
		return -1;

	int ver;
	int _dticks;
	int home1;
	int coast;
	int adaz;
	int slave;
	int _shutter;
	int dsr_status;
	int home;
	int htick_ccl;
	int htick_clk;
	int upins;
	int weaage;
	int winddir;
	int windspd;
	int temp;
	int humid;
	int wetness;
	int snow;
	int wind_peak;

	int scopeaz;
	int intdz;
	int intoff;

	if (V)
	{
		if (buf[0] != 'V')
			return -1;
		bp++;
	}

	//logStream(MESSAGE_WARNING) << "receive string from dome " << bp << sendLog;

	int sret = sscanf(bp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r",
		&ver, &_dticks, &home1, &coast, &adaz,
		&slave, &_shutter, &dsr_status, &home, &htick_ccl,
		&htick_clk, &upins, &weaage, &winddir, &windspd,
		&temp, &humid, &wetness, &snow, &wind_peak,
		&scopeaz, &intdz, &intoff);

	if (sret != 23)
	{
		logStream(MESSAGE_WARNING) << "cannot parse info string " << sret << sendLog;
		return -1;
	}

	switch (_shutter)
	{
		case 1:
			maskState(DOME_DOME_MASK, DOME_CLOSED, "dome closed");
			break;
		case 2:
			maskState(DOME_DOME_MASK, DOME_OPENED, "dome opened");
			break;
	}

	
	//logStream(MESSAGE_WARNING) << "shutter state " << _shutter << sendLog;
	
	shutter->setValueInteger(_shutter);

	dticks->setValueInteger(_dticks);

	setAzimuthTicks(adaz);

	usleep (USEC_SEC * 1.5);

	return 0;
}

int DDW::executeCmd (const char *cmd, cmdType newCmd)
{
	if (sconn->writePort (cmd, 4) != 0)
		return -1;
	char repl;
	if (sconn->readPort (repl) != 1)
		return -1;
	cmdInProgress = newCmd;

	//logStream(MESSAGE_WARNING) << "executing command " << cmdInProgress << " with reply " << repl << sendLog;
	

	return repl;
}

long DDW::inProgress (bool opening)
{
	char rc;
	if (!sconn->readPort(rc))
		return -1;

	switch (rc)
	{
		case 'S':
		    return 100;
		case 'T':
			return 100;
		case 'O':
			if (cmdInProgress == OPENING)
				return 100;
			break;
		case 'C':
			if (cmdInProgress == CLOSING)
				return 100;
			break;
		case 'P':
		{
		    char pos[5];
			if (sconn->readPort(pos, 4, '\r') == -1)
				return -1;
			setAzimuthTicks(atoi(pos));
			return 100;
		}
		case 'Z':
		{
			char zbuf[4];
			if (sconn->readPort(zbuf, 4, '\r') == -1)
				return -1;
			z->setValueInteger(atoi(zbuf));
			sendValueAll(z);
			return 100;
		}
		case 'V':
		{
			cmdInProgress = IDLE;
			parseInfo (false);
			return -2;
		}
	}
		
	logStream(MESSAGE_WARNING) << "unknown character during command execution: " << (char) rc << sendLog;
	cmdInProgress = IDLE;
	return -1;
}

float * DDW::getOffsetCoeff()
{
	// Azimuth Dome offset coefficients derived for Lowell TiMo
	// these coefficients assume (N:0, E:90, S:180, W:270)
	// the transformation from rts2 horizontal system is done in
	// getDomeAzFromTargetAz and getTargetAzfromDomeAz

static float AzDomeOffsetCoeff[3];

if (getTargetAlt() > 0 and getTargetAlt() <= 30) {
	AzDomeOffsetCoeff[0] = -0.023*getTargetAlt()-4.291;
	AzDomeOffsetCoeff[1] = -0.015*getTargetAlt()+0.24;
	AzDomeOffsetCoeff[2] = -0.102*getTargetAlt()-11.345;
}

else if (getTargetAlt() > 30 and getTargetAlt() <= 45) {
	AzDomeOffsetCoeff[0] = -0.044*getTargetAlt()-3.663;
	AzDomeOffsetCoeff[1] = -0.015*getTargetAlt()-0.028;
	AzDomeOffsetCoeff[2] = -0.171*getTargetAlt()-9.287;
}

else if (getTargetAlt() > 45 and getTargetAlt() <= 60) {
	AzDomeOffsetCoeff[0] = -0.201*getTargetAlt()+3.402;
	AzDomeOffsetCoeff[1] = 0.0078*getTargetAlt()-0.637;
	AzDomeOffsetCoeff[2] = -0.337*getTargetAlt()-1.835;
}

else if (getTargetAlt() > 60 and getTargetAlt() <= 75) {
	AzDomeOffsetCoeff[0] = -0.337*getTargetAlt()+11.55;
	AzDomeOffsetCoeff[1] = 0.0042*getTargetAlt()-0.421;
	AzDomeOffsetCoeff[2] = -0.909*getTargetAlt()+32.537;
}

else if (getTargetAlt() > 75 and getTargetAlt() <= 90) {
	AzDomeOffsetCoeff[0] = -0.585*getTargetAlt()+30.150;
	AzDomeOffsetCoeff[1] = -0.796*getTargetAlt()+59.614;
	AzDomeOffsetCoeff[2] = -2.345*getTargetAlt()+140.172;
}


return AzDomeOffsetCoeff;
	
}

long DDW::getAzDomeOffset(double az)
{
 	float *coeffarray = getOffsetCoeff();

	return coeffarray[0]*sin(az/180*M_PI+coeffarray[1])+coeffarray[2];
}

long DDW::getDomeAzFromTargetAz(double targetAz)
{
	// add 180 degs to go from rts2 horizontal frame to (N:0, E:90,
	// S:180, W:270) frame in which measurements were done
	targetAz = ((int)targetAz + 180) % 360;
	return targetAz + getAzDomeOffset(targetAz);
}

long DDW::getTargetAzFromDomeAz(double domeAz)
{
	// add 180 degs to go from rts2 horizontal frame to (N:0, E:90,
	// S:180, W:270) frame in which measurements were done
	domeAz = ((int)domeAz - 180 + 360) % 360;
	return domeAz - getAzDomeOffset(domeAz);
}

/* TO BE IMPLEMENTED FOR OTHER TELESCOPE

float * getOffsetCoeffOther(getTargetAlt()) 
{

static float AzDomeOffsetCoeff[3];

if (getTargetAlt() > 0 and getTargetAlt() <= 30) {
	AzDomeOffsetCoeff[0] = 0.032*getTargetAlt()-13.67;
	AzDomeOffsetCoeff[1] = -0.0006*getTargetAlt()-0.089;
	AzDomeOffsetCoeff[2] = 0.0806*getTargetAlt()+5.107;
}

else if (getTargetAlt() > 30 and getTargetAlt() <= 45) {
	AzDomeOffsetCoeff[0] = -0.071*getTargetAlt()-10.565;
	AzDomeOffsetCoeff[1] = -0.00547*getTargetAlt()+0.057;
	AzDomeOffsetCoeff[2] = 0.317*getTargetAlt()-1.993;
}

else if (getTargetAlt() > 45 and getTargetAlt() <= 60) {
	AzDomeOffsetCoeff[0] = -0.096*getTargetAlt()-9.443;
	AzDomeOffsetCoeff[1] = 0.0022*getTargetAlt()-0.288;
	AzDomeOffsetCoeff[2] = 0.630*getTargetAlt()-16.045;
}

else if (getTargetAlt() > 60 and getTargetAlt() <= 90) {
	AzDomeOffsetCoeff[0] = -0.132*getTargetAlt()-7.281;
	AzDomeOffsetCoeff[1] = 0.00026667*getTargetAlt()-0.14;
	AzDomeOffsetCoeff[2] = 0.7107*getTargetAlt()-20.917;
}


return AzDomeOffsetCoeff;

}

long  CLASS::getAzDomeOffsetOtherTelescope(double az)
{
 	float *coeffarray = getOffsetCoeffOtheró();

	return coeffarray[0]*sin(az/180*M_PI+coeffarray[1])+coeffarray[2];

}

*/
int main(int argc, char **argv)
{
	DDW device(argc, argv);
	return device.run();
}
