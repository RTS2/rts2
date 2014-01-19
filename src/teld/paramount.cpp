/* 
 * Driver for Paramount. You need ParaLib to make that work, please ask petr@kubanek.net
 * for details.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2013 Martin Jelinek <mates@iaa.es>
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

#define DEBUG_EXTRA

#include "gem.h"
#include "telmodel.h"

#include "libnova_cpp.h"

#include <libmks3.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <fcntl.h>

#define TEL_SLEW            0x00
#define TEL_FORCED_HOMING0  0x01
#define TEL_FORCED_HOMING1  0x02

#define RA_TICKS        11520000
#define DEC_TICKS       7500000

// park positions
#define PARK_AXIS0      0
#define PARK_AXIS1      0

typedef enum { T16, T32 } para_t;

/**
 * Holds paramount constant values.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class ParaVal
{
	public:
		ParaVal (const char *in_name, para_t in_type, int in_id)
		{
			name = in_name;
			type = in_type;
			id = in_id;
		}

		bool isName (std::string in_name)
		{
			return (name == in_name);
		}

		int writeAxis (std::ostream & _os, const MKS3Id & axis);
		int readAxis (std::istream & _is, const MKS3Id & axis);

	private:
		const char *name;
		para_t type;
		int id;
};

int ParaVal::writeAxis (std::ostream & _os, const MKS3Id & axis)
{
	CWORD16 val16;
	CWORD32 val32;
	int ret;
	logStream (MESSAGE_DEBUG) << "ParaVal::writeAxis" << sendLog;
	_os << name << ": ";
	switch (type)
	{
		case T16:
			logStream (MESSAGE_DEBUG) << "_MKS3DoGetVal16 axis" << sendLog;
			ret = _MKS3DoGetVal16 (axis, id, &val16);
			if (ret)
			{
				_os << "error" << std::endl;
				return -1;
			}
			_os << val16 << std::endl;
			break;
		case T32:
			logStream (MESSAGE_DEBUG) << "_MKS3DoGetVal32 axis" << sendLog;
			ret = _MKS3DoGetVal32 (axis, id, &val32);
			if (ret)
			{
				_os << "error" << std::endl;
				return -1;
			}
			_os << val32 << std::endl;
			break;
	}
	return ret;
}

int ParaVal::readAxis (std::istream & _is, const MKS3Id & axis)
{
	CWORD16 val16;
	CWORD32 val32;
	int ret;
	logStream (MESSAGE_DEBUG) << "ParaVal::readAxis" << sendLog;
	switch (type)
	{
		case T16:
			_is >> val16;
			if (_is.fail ())
				return -1;
			logStream (MESSAGE_DEBUG) << "_MKS3DoSetVal16 axis" << sendLog;
			ret = _MKS3DoSetVal16 (axis, id, val16);
			break;
		case T32:
			_is >> val32;
			if (_is.fail ())
				return -1;
			logStream (MESSAGE_DEBUG) << "_MKS3DoSetVal32 axis" << sendLog;
			ret = _MKS3DoSetVal32 (axis, id, val32);
			break;
	}
	return ret;
}


namespace rts2teld
{

class Paramount:public GEM
{
	public:
		Paramount (int in_argc, char **in_argv);
		virtual ~ Paramount (void);

		virtual int idle ();

	protected:
		virtual int initHardware ();
		virtual int initValues ();

		virtual int processOption (int in_opt);

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual int isMoving ();
		virtual int isParking ();

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int updateLimits ();

		virtual void updateTrack ();

		// returns actual home offset
		virtual int getHomeOffset (int32_t & off);

		virtual int startResync ();
		virtual int stopMove ();

		virtual int startPark ();
		virtual int endPark ();

		// save and load (ehm) gemini information
		virtual int saveModel ();
		virtual int loadModel ();

		virtual void setDiffTrack (double dra, double ddec);

	private:
		MKS3Id axis0;
		MKS3Id axis1;
		const char *device_name;
		const char *paramount_cfg;

		bool setIndices;

		int ret0;
		int ret1;

		rts2core::ValueInteger *statusRa;
		rts2core::ValueInteger *statusDec;

		rts2core::ValueInteger *motorStatusRa;
		rts2core::ValueInteger *motorStatusDec;

		rts2core::ValueLong *speedRa;
		rts2core::ValueLong *speedDec;
		rts2core::ValueLong *accRa;
		rts2core::ValueLong *accDec;

		rts2core::ValueLong *homevelRaHi;
		rts2core::ValueLong *homevelDecHi;

		rts2core::ValueLong *homevelRaMed;
		rts2core::ValueLong *homevelDecMed;

		rts2core::ValueLong *homevelRaLow;
		rts2core::ValueLong *homevelDecLow;

		rts2core::ValueLong *hourRa;
		rts2core::ValueLong *baseRa;
		rts2core::ValueLong *baseDec;

		rts2core::ValueLong *creepRa;
		rts2core::ValueLong *creepDec;

		rts2core::ValueLong *relRa;
		rts2core::ValueLong *relDec;

		rts2core::ValueBool *tracking;

		rts2core::ValueBool *motorRa;
		rts2core::ValueBool *motorDec;

		CWORD32 park_axis[2];

		CWORD16 ostat0,ostat1;
		double getUsec();
		int doPara();
		int doInfo();
		int basicInfo();
		double canTouch;
		int wasTimeout;
		int forceHoming;
		int doSlew;
		int doAbort;
		int doPark;
		int offNotMoving;

		int checkRetAxis (const MKS3Id & axis, int reta);
		int checkRet ();
		int updateStatus ();

		int loadModelFromFile ();

		int saveFlash ();

		std::vector < ParaVal > paramountValues;

		rts2core::ValueLong *axRa;
		rts2core::ValueLong *axDec;

		rts2core::ValueLong *homeOffsetRa;
		rts2core::ValueLong *homeOffsetDec;

		rts2core::ValueLong *encoderRa;
		rts2core::ValueLong *encoderDec;

		int saveAxis (std::ostream & os, const MKS3Id & axis);

		int moveState;
		char *statusStr(CWORD16 status, char *buf);

		// variables for tracking
		struct timeval track_start_time;
		struct timeval track_recalculate;
		struct timeval track_next;
		MKS3ObjTrackInfo *track0;
		MKS3ObjTrackInfo *track1;

		// timeout for move command
		double move_timeout;

		int getHomeOffsetAxis (MKS3Id axis, int32_t & off);

		// return RA and DEC value pair
		int getParamountValue32 (int id, rts2core::ValueLong *vRa, rts2core::ValueLong *vDec);
		int setParamountValue32 (int id, rts2core::Value *vRa, rts2core::Value *vDec);
};

};

using namespace rts2teld;

char * Paramount::statusStr (CWORD16 status, char *str)
{
	int a=0;
	*str=0; 

	if (status & MOTOR_HOMING) a+=sprintf (str, "HOMING ");
	//  if (status & MOTOR_SERVO) printf ("\033[%dmSRV\033[m ", 30 + color);
	if (status & MOTOR_INDEXING) a+=sprintf (str+a, "INDEXING ");
	if (status & MOTOR_SLEWING) a+=sprintf (str+a, "SLEWING ");
	if (!(status & MOTOR_HOMED)) a+=sprintf (str+a, "!HOMED ");
	if (status & MOTOR_HOMED) a+=sprintf (str+a, "READY ");
	if (status & MOTOR_JOYSTICKING) a+=sprintf (str+a, "JOYSTICKING ");
	if (status & MOTOR_OFF) a+=sprintf (str+a, "OFF ");
	return str;
}


int Paramount::checkRetAxis (const MKS3Id & axis, int reta)
{
	const char *msg = NULL;
	messageType_t msg_type = MESSAGE_ERROR;
	switch (reta)
	{
		case COMM_OKPACKET:		msg = "comm packet OK"; msg_type = MESSAGE_DEBUG; break;
		case COMM_NOPACKET:		msg = "comm no packet"; break;
		case COMM_TIMEOUT:		msg = "comm timeout (check cable!)"; break;
		case COMM_COMMERROR:		msg = "comm error"; break;
		case COMM_BADCHAR:		msg = "bad char at comm"; break;
		case COMM_OVERRUN:		msg = "comm overrun"; break;
		case COMM_BADCHECKSUM:		msg = "comm bad checksum"; break;
		case COMM_BADLEN:		msg = "comm bad message len"; break;
		case COMM_BADCOMMAND:		msg = "comm bad command"; break;
		case COMM_INITFAIL:		msg = "comm init fail"; break;
		case COMM_NACK:			msg = "comm not acknowledged"; break;
		case COMM_BADID:		msg = "comm bad id"; break;
		case COMM_BADSEQ:		msg = "comm bad sequence"; break;
		case COMM_BADVALCODE:		msg = "comm bad value code"; break;

		case MAIN_WRONG_UNIT:		msg = "main wrong unit"; break;
		case MAIN_BADMOTORINIT:		msg = "main bad motor init"; break;
		case MAIN_BADMOTORSTATE:	msg = "main bad motor state"; break;
		case MAIN_BADSERVOSTATE:	msg = "main bad servo state"; break;
		case MAIN_SERVOBUSY:		msg = "main servo busy"; break;
		case MAIN_BAD_PEC_LENGTH:	msg = "main bad pec lenght"; break;
		case MAIN_AT_LIMIT:		msg = "main at limit"; break;
		case MAIN_NOT_HOMED:		msg = "main not homed"; break;
		case MAIN_BAD_POINT_ADD:	msg = "main bad point add"; break;

		case FLASH_PROGERR:		msg = "flash program error"; break;
		// case FLASH_ERASEERR:		msg = "flash erase error"; break; 
		case FLASH_TIMEOUT:		msg = "flash timeout"; break;
		case FLASH_CANT_OPEN_FILE:	msg = "flash cannot open file"; break;
		case FLASH_BAD_FILE:		msg = "flash bad file"; break;
		case FLASH_FILE_READ_ERR:	msg = "flash file read err"; break;
		case FLASH_BADVALID:		msg = "flash bad value id"; break;

		case MOTOR_OK:			msg = "motor ok"; msg_type = MESSAGE_DEBUG; break;
		case MOTOR_OVERCURRENT:		msg = "motor over current"; break;
		case MOTOR_POSERRORLIM:		msg = "motor maximum position error exceeded"; break;
		case MOTOR_STILL_ON:		msg = "motor is on but command needs it at off state"; break;
		case MOTOR_NOT_ON:		msg = "motor off"; break;
		case MOTOR_STILL_MOVING:	msg = "motor still slewing but command needs it stopped"; break;
	}
	if (msg && msg_type != MESSAGE_DEBUG)
		logStream (msg_type) << "Axis:" << axis.axisId << " " << msg << sendLog;
	return reta;
}

int Paramount::checkRet ()
{
	int reta0, reta1;
	reta0 = checkRetAxis (axis0, ret0);
	reta1 = checkRetAxis (axis1, ret1);
	if (reta0 != MKS_OK || reta1 != MKS_OK) return -1;
	return 0;
}

int Paramount::updateStatus ()
{
	CWORD16 new_status;
	char strstat[512];	
	
	// logStream (MESSAGE_DEBUG) << "Paramount::updateStatus" << sendLog;

	ret0 = MKS3StatusGet (axis0, &new_status);
	logStream (MESSAGE_DEBUG) << "MKS3StatusGet axis0: ret0=" << ret0 << "(" << (ret0?"err.":"-OK-") << ") status=" << new_status << " (" << statusStr(new_status, strstat) << ")" << sendLog;

	if (statusRa->getValueInteger () != new_status)
	{
		logStream (MESSAGE_DEBUG) << "changed axis 0 state from " << std::hex << statusRa->getValueInteger () << " to " << new_status << sendLog;
		statusRa->setValueInteger (new_status);
	}

	ret1 = MKS3StatusGet (axis1, &new_status);
	logStream (MESSAGE_DEBUG) << "MKS3StatusGet axis1: ret1=" << ret1 << "(" << (ret1?"err.":"-OK-") << ") status=" << new_status << " (" << statusStr(new_status, strstat) << ")" << sendLog;

	if (statusDec->getValueInteger () != new_status)
	{
		logStream (MESSAGE_DEBUG) << "changed axis 1 state from " << std::hex << statusDec->getValueInteger () << " to " << new_status << sendLog;
		statusDec->setValueInteger (new_status);
	}

	if (checkRet ())
		return -1;

	// set / unset HW error
	if (
		(statusRa->getValueInteger ()  & MOTOR_ERR ) || 
		(statusDec->getValueInteger () & MOTOR_ERR ) || 
		statusRa->getValueInteger ()  == (MOTOR_HOMING | MOTOR_INDEXING) || 
		statusDec->getValueInteger () == (MOTOR_HOMING | MOTOR_INDEXING) || 
		statusRa->getValueInteger ()  == (MOTOR_HOMING | MOTOR_INDEXING | MOTOR_SLEWING) || 
		statusDec->getValueInteger () == (MOTOR_HOMING | MOTOR_INDEXING | MOTOR_SLEWING) || 
		statusRa->getValueInteger ()  == (               MOTOR_INDEXING | MOTOR_SLEWING) || 
		statusDec->getValueInteger () == (               MOTOR_INDEXING | MOTOR_SLEWING)
	)

		maskState (DEVICE_ERROR_HW, DEVICE_ERROR_HW, "set error - axis failed");
	else
		maskState (DEVICE_ERROR_HW, 0, "cleared error conditions");

	// motor status..
	ret0 = MKS3MotorStatusGet (axis0, &new_status);


	if (getDebug ())
		logStream (MESSAGE_DEBUG) << "MKS3MotorStatusGet axis0: ret0=" << ret0 << " status=" << new_status << sendLog;

	if (motorStatusRa->getValueInteger () != new_status)
	{
		logStream (MESSAGE_DEBUG) << "changed motor 0 state from " << std::hex << motorStatusRa->getValueInteger () << " to " << new_status << sendLog;
		motorStatusRa->setValueInteger (new_status);
	}

	ret1 = MKS3MotorStatusGet (axis1, &new_status);
	logStream (MESSAGE_DEBUG) << "MKS3MotorStatusGet axis1: ret1=" << ret1 << " status=" << new_status << sendLog;

	if (motorStatusDec->getValueInteger () != new_status)
	{
		logStream (MESSAGE_DEBUG) << "changed motor 1 state from " << std::hex << motorStatusDec->getValueInteger () << " to " << new_status << sendLog;
		motorStatusDec->setValueInteger (new_status);
	}

	return checkRet ();
}

int Paramount::saveAxis (std::ostream & os, const MKS3Id & axis)
{
	int ret;
	CWORD16 pMajor, pMinor, pBuild;

	logStream (MESSAGE_DEBUG) << "Paramount::saveAxis" << sendLog;
	logStream (MESSAGE_DEBUG) << "MKS3VersionGet" << sendLog;
	ret = MKS3VersionGet (axis, &pMajor, &pMinor, &pBuild);
	if (ret)
		return -1;

	os << "*********************** Axis "
		<< axis.axisId << (axis.axisId ? " (DEC)" : " (RA)")
		<< " Control version " << pMajor << "." << pMinor << "." << pBuild
		<< " ********************" << std::endl;

	for (std::vector < ParaVal >::iterator iter = paramountValues.begin ();
		iter != paramountValues.end (); iter++)
	{
		ret = (*iter).writeAxis (os, axis);
		if (ret)
			return -1;
	}
	return 0;
}

int Paramount::getHomeOffsetAxis (MKS3Id axis, int32_t & off)
{
	int ret;
	CWORD32 en0, pos0;

//	logStream (MESSAGE_DEBUG) << "Paramount::getHomeOffsetAxis" << sendLog;
//	logStream (MESSAGE_DEBUG) << "MKS3PosEncoderGet" << sendLog;
	ret = MKS3PosEncoderGet (axis, &en0);
	if (ret != MKS_OK)
		return ret;
//	logStream (MESSAGE_DEBUG) << "MKS3PosCurGet" << sendLog;
	ret = MKS3PosCurGet (axis, &pos0);
	if (ret != MKS_OK)
		return ret;
	off = en0 - pos0;
	return 0;
}

int Paramount::getHomeOffset (int32_t & off)
{
//	logStream (MESSAGE_DEBUG) << "Paramount::getHomeOffset" << sendLog;
	return getHomeOffsetAxis (axis0, off);
}

int Paramount::updateLimits ()
{
	int ret;
	logStream (MESSAGE_DEBUG) << "Paramount::UpdateLimits" << sendLog;
	logStream (MESSAGE_DEBUG) << "MKS3ConstsLimMinGet axis0" << sendLog;
	ret = MKS3ConstsLimMinGet (axis0, &acMin);
	if (ret)
		return -1;
	logStream (MESSAGE_DEBUG) << "MKS3ConstsLimMinGet axis0" << sendLog;
	ret = MKS3ConstsLimMaxGet (axis0, &acMax);
	if (ret)
		return -1;

	logStream (MESSAGE_DEBUG) << "MKS3ConstsLimMinGet axis1" << sendLog;
	ret = MKS3ConstsLimMinGet (axis1, &dcMin);
	if (ret)
		return -1;
	logStream (MESSAGE_DEBUG) << "MKS3ConstsLimMaxGet axis1" << sendLog;
	ret = MKS3ConstsLimMaxGet (axis1, &dcMax);
	if (ret)
		return -1;
	return 0;
}

Paramount::Paramount (int in_argc, char **in_argv):GEM (in_argc, in_argv, true)
{
	logStream (MESSAGE_DEBUG) << "Paramount::Paramount" << sendLog;
	createValue (tracking, "tracking", "if RA worm is enabled", false, RTS2_VALUE_WRITABLE);
	tracking->setValueBool (true);

	createValue (axRa, "AXRA", "RA axis count", true);
	createValue (axDec, "AXDEC", "DEC axis count", true);

	createValue (homeOffsetRa, "HOMEAXRA", "RA home offset", false);
	createValue (homeOffsetDec, "HOMEAXDEC", "DEC home offset", false);

	createValue (encoderRa, "ENCODER_RA", "RA encoder value", false);
	createValue (encoderDec, "ENCODER_DEC", "DEC encoder value", false);

	createValue (statusRa, "status_ra", "RA axis status", false, RTS2_DT_HEX);
	createValue (statusDec, "status_dec", "DEC axis status", false, RTS2_DT_HEX);

	createValue (motorStatusRa, "motor_status_ra", "RA motor status", false, RTS2_DT_HEX);
	createValue (motorStatusDec, "motor_status_dec", "DEC motor status", false, RTS2_DT_HEX);

	createValue (speedRa, "SPEED_RA", "[???] RA axis maximal speed", false, RTS2_VALUE_WRITABLE);
	createValue (speedDec, "SPEED_DEC", "[???] DEC axis maximal speed", false, RTS2_VALUE_WRITABLE);
	createValue (accRa, "ACC_RA", "[???] RA axis acceleration", false, RTS2_VALUE_WRITABLE);
	createValue (accDec, "ACC_DEC", "[???] DEC axis acceleration", false, RTS2_VALUE_WRITABLE);

	createValue (homevelRaHi, "HOMEVEL_RA_HI", "[???] RA high home velocity", false, RTS2_VALUE_WRITABLE);
	createValue (homevelDecHi, "HOMEVEL_DEC_HI", "[???] DEC high home velocity", false, RTS2_VALUE_WRITABLE);

	createValue (homevelRaMed, "HOMEVEL_RA_MED", "[???] RA medium home velocity", false, RTS2_VALUE_WRITABLE);
	createValue (homevelDecMed, "HOMEVEL_DEC_MED", "[???] DEC medium home velocity", false, RTS2_VALUE_WRITABLE);

	createValue (homevelRaLow, "HOMEVEL_RA_LOW", "[???] RA low home velocity", false, RTS2_VALUE_WRITABLE);
	createValue (homevelDecLow, "HOMEVEL_DEC_LOW", "[???] DEC low home velocity", false, RTS2_VALUE_WRITABLE);

	createValue (hourRa, "HR_RA", "[???] RA axis tracking rate", true);
	hourRa->setValueLong (-43067265);
	createValue (baseRa, "BR_RA", "[???] RA axis base rate", true, RTS2_VALUE_WRITABLE);
	baseRa->setValueLong (hourRa->getValueLong ());
	createValue (baseDec, "BR_DEC", "[???] DEC axis base rate", true, RTS2_VALUE_WRITABLE);
	baseDec->setValueLong (0);

	createValue (creepRa, "CREEP_RA", "[???] RA creep value", false, RTS2_VALUE_WRITABLE);
	creepRa->setValueLong (0);

	createValue (creepDec, "CREEP_DEC", "[???] DEC creep value", false, RTS2_VALUE_WRITABLE);
	creepDec->setValueLong (0);

	createValue (relRa, "REL_RA", "[???] RA relative position", false, RTS2_VALUE_WRITABLE);
	relRa->setValueLong (0);

	createValue (relDec, "REL_DEC", "[???] DEC relative position", false, RTS2_VALUE_WRITABLE);
	relDec->setValueLong (0);

	createValue (motorRa, "motor_RA", "RA motor state", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (motorDec, "motor_DEC", "DEC motor state", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);

	axis0.unitId = 0x64;
	axis0.axisId = 0;

	axis1.unitId = 0x64;
	axis1.axisId = 1;

	park_axis[0] = PARK_AXIS0;
	park_axis[1] = PARK_AXIS1;

	moveState = TEL_SLEW;

	move_timeout = 0;

	ra_ticks = RA_TICKS;
	dec_ticks = DEC_TICKS;

	setIndices = false;

	device_name = "/dev/ttyS0";
	paramount_cfg = "/etc/rts2/paramount.cfg";
	addOption ('f', "device_name", 1, "device file (default /dev/ttyS0");
	addOption ('P', "paramount_cfg", 1, "paramount config file (default /etc/rts2/paramount.cfg");
	addOption ('R', "recalculate", 1, "track update interval in sec; < 0 to disable track updates; defaults to 1 sec");
	addOption ('t', "set indices", 0, "if we need to load indices as first operation");
//	addOption ('G', "dec_zero", 0, "Declination Zero position");
//	addOption ('H', "ha_zero", 0, "Hour Angle Zero position");

	addOption ('D', "dec_park", 1, "DEC park position");
	// in degrees! 30 for south, -30 for north hemisphere; S swap is done later
	// it's (lst - ra(home))
	// haZero and haCpd S swap are handled in ::init, after we get latitude from config file

// this is bootes-1a weirdness
//	haZero = -31.82333;
	//decZero = -10.31777;
	//haZero = -28.17667;
//	decZero = +10.31777;

// this is normal
	haZero = -30.0;
	decZero = 0.0;

	// how many counts per degree
	haCpd = -32000.0;	 // - for N hemisphere, + for S hemisphere; S swap is done later
	decCpd = -20883.33333333333;

	acMargin = 10000;

	timerclear (&track_start_time);
	timerclear (&track_recalculate);
	timerclear (&track_next);

	track_recalculate.tv_sec = 1;
	track_recalculate.tv_usec = 0;

	track0 = NULL;
	track1 = NULL;

	// apply all correction for paramount
	setCorrections (true, true, true);

	// int paramout values
	paramountValues.push_back (ParaVal ("Index angle", T16, CMD_VAL16_INDEX_ANGLE));
	paramountValues.push_back (ParaVal ("Base rate", T32, CMD_VAL32_BASERATE));
	paramountValues.push_back (ParaVal ("Maximum speed", T32, CMD_VAL32_SLEW_VEL));
	paramountValues.push_back (ParaVal ("Acceleration", T32, CMD_VAL32_SQRT_ACCEL));
	//  paramountValues.push_back (ParaVal ("Non-sidereal tracking rate" ));
	paramountValues.push_back (ParaVal ("Minimum position limit", T32, CMD_VAL32_MIN_POS_LIM));
	paramountValues.push_back (ParaVal ("Maximum position limit", T32, CMD_VAL32_MAX_POS_LIM));
	paramountValues.push_back (ParaVal ("Sensor (from sync)", T16, CMD_VAL16_HOME_SENSORS));
	paramountValues.push_back (ParaVal ("Guide", T32, CMD_VAL32_CONSTS_VEL_GUIDE));
	//  paramountValues.push_back (ParaVal ("Tics per revolution",
	paramountValues.push_back (ParaVal ("PEC ratio", T16, CMD_VAL16_PEC_RATIO));
	paramountValues.push_back (ParaVal ("Maximum position error", T16, CMD_VAL16_ERROR_LIMIT));
	//  paramountValues.push_back (ParaVal ("Unit Id.",
	paramountValues.push_back (ParaVal ("EMF constants", T16, CMD_VAL16_EMF_FACTOR));
	paramountValues.push_back (ParaVal ("Home velocity high", T32, CMD_VAL32_HOMEVEL_HI));
	paramountValues.push_back (ParaVal ("Home velocity medium", T32, CMD_VAL32_HOMEVEL_MED));
	paramountValues.push_back (ParaVal ("Home velocity low", T32, CMD_VAL32_HOMEVEL_LO));
	paramountValues.push_back (ParaVal ("Home direction, sense", T16, CMD_VAL16_HOMEDIR));
	paramountValues.push_back (ParaVal ("Home mode, required, Joystick, In-out-in", T16, CMD_VAL16_HOME_MODE));
	paramountValues.push_back (ParaVal ("Home index offset", T16, CMD_VAL16_HOME2INDEX));
	paramountValues.push_back (ParaVal ("PEC cutoff speed", T32, CMD_VAL32_PEC_CUT_SPEED));
	paramountValues.push_back (ParaVal ("Maximum voltage", T16, CMD_VAL16_MOTOR_VOLT_MAX));
	paramountValues.push_back (ParaVal ("Maximum gain", T16, CMD_VAL16_MOTOR_PROPORT_MAX));
	paramountValues.push_back (ParaVal ("Home sense 1", T16, CMD_VAL16_HOMESENSE1));
	paramountValues.push_back (ParaVal ("Home sense 2", T16, CMD_VAL16_HOMESENSE2));
	paramountValues.push_back (ParaVal ("Cur pos", T32, CMD_VAL32_CURPOS));
}

Paramount::~Paramount (void)
{
	delete track0;
	delete track1;
	MKS3Free ();
}

int Paramount::processOption (int in_opt)
{
	double rec_sec;
	logStream (MESSAGE_DEBUG) << "Paramount::processOption" << sendLog;
	switch (in_opt)
	{
		case 'f':
			device_name = optarg;
			break;
		case 'P':
			paramount_cfg = optarg;
			break;
		case 'R':
			rec_sec = atof (optarg);
			track_recalculate.tv_sec = (int) rec_sec;
			track_recalculate.tv_usec =
				(int) ((rec_sec - track_recalculate.tv_sec) * USEC_SEC);
			break;
		case 't':
			setIndices = true;
			break;
		case 'G':
			//decZero->setValueDouble( atof (optarg) );
			break;
		case 'H':
			//haZero->setValueDouble( atof (optarg) );
			break;
		case 'D':
			park_axis[1] = atoi (optarg);
			break;
		default:
			return GEM::processOption (in_opt);
	}
	return 0;
}

int Paramount::initHardware ()
{
	int ret;

	logStream (MESSAGE_DEBUG) << "Paramount::initHardware" << sendLog;
	rts2core::Configuration *config = rts2core::Configuration::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	telLongitude->setValueDouble (config->getObserver ()->lng);
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
								 // south hemispehere
	if (telLatitude->getValueDouble () < 0)
	{
		// swap values which are opposite for south hemispehere
		haZero *= -1.0;
		haCpd *= -1.0;
		hourRa->setValueLong (-1 * hourRa->getValueLong ());
	}

	logStream (MESSAGE_DEBUG) << "MKS3Init" << sendLog;
	ret = MKS3Init (device_name);
	if (ret)
		return -1;

	// these states are only 3, SLEW, PARK1 and PARK2, so the SLEW is good
	ostat0=65535; ostat1=65535;
	moveState = TEL_SLEW; 
	canTouch=0;
	wasTimeout = 0;
	forceHoming = 0;
	doSlew = 0;
	doAbort = 0;
	doPark = 0;
	offNotMoving = 1;

#ifdef TERM_OUT
	fprintf(stdout, "\033[2J"); // clear screen
	fflush(stdout);
#endif

	basicInfo();

	return 0;
}

int Paramount::basicInfo()
{
	int ret;
	
	updateLimits ();

	if (setIndices)
	{
		ret = loadModel ();
		if (ret)
			return -1;
	}
	
	CWORD16 pMajor, pMinor, pBuild;
	logStream (MESSAGE_DEBUG) << "MKS3VersionGet axis0" << sendLog;
	ret = MKS3VersionGet (axis0, &pMajor, &pMinor, &pBuild);
	if (ret)
		return ret;
	snprintf (telType, 64, "Paramount %i %i %i", pMajor, pMinor, pBuild);

  	ret = getParamountValue32 (CMD_VAL32_SLEW_VEL, speedRa, speedDec);
	if (ret)
		return ret;

	ret = getParamountValue32 (CMD_VAL32_SQRT_ACCEL, accRa, accDec);
	if (ret)
		return ret;

	ret = getParamountValue32 (CMD_VAL32_HOMEVEL_HI, homevelRaHi, homevelDecHi);
	if (ret)
		return ret;

	ret = getParamountValue32 (CMD_VAL32_HOMEVEL_MED, homevelRaMed, homevelDecMed);
	if (ret)
		return ret;

	ret = getParamountValue32 (CMD_VAL32_HOMEVEL_LO, homevelRaLow, homevelDecLow);
	if (ret)
		return ret;

	return ret;
}

int Paramount::commandAuthorized (rts2core::Connection *conn)
{
	logStream (MESSAGE_DEBUG) << "Paramount::commandAuthorized" << sendLog;
	if (conn->isCommand ("sleep"))
	{
		if (!conn->paramEnd ())
			return -2;
		sleep (120);
		return 0;
	}
	else if (conn->isCommand ("save_flash"))
	{
		logStream (MESSAGE_DEBUG) << "MKS3ConstsStore axis0" << sendLog;
		ret0 = MKS3ConstsStore (axis0);
		logStream (MESSAGE_DEBUG) << "MKS3ConstsStore axis1" << sendLog;
		ret1 = MKS3ConstsStore (axis1);
		int ret = checkRet ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot store constants");
			return -1;
		}

		logStream (MESSAGE_DEBUG) << "MKS3ConstsReload axis0" << sendLog;
		ret0 = MKS3ConstsReload (axis0);
		logStream (MESSAGE_DEBUG) << "MKS3ConstsReload axis1" << sendLog;
		ret1 = MKS3ConstsReload (axis1);
		ret = checkRet ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot reload constants");
			return -1;
		}

		return 0;
	}
	return Telescope::commandAuthorized (conn);
}

int Paramount::initValues ()
{
	// ignore corrections bellow 5 arcsec
	setIgnoreCorrection (5/3600);
	return Telescope::initValues ();
}

void Paramount::updateTrack ()
{
	double JD;
	struct ln_equ_posn corr_pos;
	return;

	gettimeofday (&track_next, NULL);
	if (track_recalculate.tv_sec < 0)
		return;
	timeradd (&track_next, &track_recalculate, &track_next);

	if (!track0 || !track1)
	{
		JD = ln_get_julian_from_sys ();

		getTarget (&corr_pos);
		startResync ();
		return;
	}
	double track_delta;
	CWORD32 ac, dc;
	MKS3ObjTrackStat stat0, stat1;

	track_delta =
		track_next.tv_sec - track_start_time.tv_sec + (track_next.tv_usec -
		track_start_time.tv_usec) /
		USEC_SEC;
	JD = ln_get_julian_from_timet (&track_next.tv_sec);
	JD += track_next.tv_usec / USEC_SEC / 86400.0;
	getTarget (&corr_pos);
	// calculate position at track_next time
	sky2counts (&corr_pos, ac, dc, JD, 0);

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Track ac " << ac << " dc " << dc << " " << track_delta << sendLog;
	#endif						 /* DEBUG_EXTRA */

	ret0 = MKS3ObjTrackPointAdd (axis0, track0, track_delta + track0->prevPointTimeTicks / track0->sampleFreq, (CWORD32) (ac + (track_delta * 10000)), &stat0);
	ret1 = 	MKS3ObjTrackPointAdd (axis1, track1, track_delta + track1->prevPointTimeTicks / track1->sampleFreq, (CWORD32) (dc + (track_delta * 10000)), &stat1);
	checkRet ();
}

double Paramount::getUsec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	return tv.tv_sec+tv.tv_usec/1000000.;
} 
	
int Paramount::doPara()
{
	CWORD16 stat0,stat1;
	char strstat[512];
	double usec;
	usec=getUsec();

	while ( true )
	{

	// There are cases we should not touch the mount until some time is elapsed
	if (canTouch > usec) 
		{
//		logStream (MESSAGE_DEBUG) << "not touching" << sendLog;
		return 0;
		}		

	// ask for basic status
	ret0 = MKS3StatusGet (axis0, &stat0);
	ret1 = MKS3StatusGet (axis1, &stat1);

	// If something seems wrong with the mount, set up time to have a look again and return
	if (ret0 || ret1)
	{
		logStream (MESSAGE_DEBUG) << "StatusGet returns non-zero, will wait, ret0=" << 
			ret0 << " ret1=" << ret1 << sendLog;
		checkRet();
		maskState (DEVICE_ERROR_HW, DEVICE_ERROR_HW, "set error - axis failed");
		canTouch = getUsec()+1.0;
		wasTimeout = 1;
		return 0;
	}

#ifdef TERM_OUT
	static int progresscnt=0;

	fprintf(stdout, "\033[2;1H%c", ".oOo"[progresscnt]);
	progresscnt=(progresscnt+1)%4; 

	fprintf(stdout, "\033[2;4Hstatus0: ret0=0x%04x (%s) stat0=0x%04x (%s)          \n", 
		ret0, ret0?"err.":"-OK-",  stat0, statusStr(stat0,strstat));
	fprintf(stdout, "\033[3;4Hstatus1: ret1=0x%04x (%s) stat1=0x%04x (%s)          \n\n", 
		ret1, ret1?"err.":"-OK-",  stat1, statusStr(stat1,strstat));
	fflush(stdout);
#endif
	// This floods the log so we write it only if change...
	if( (ostat0 != stat0) || (ostat1!=stat1) )	
		logStream (MESSAGE_DEBUG) << "MKS3StatusGet "
			<< "ret0=" << ret0 << "[" << (ret0?"err.":"-OK-") << "] " 
			<< "stat=" << stat0 << "[" << statusStr(stat0, strstat) << "] "
			<< "ret1=" << ret1 << "[" << (ret1?"err.":"-OK-") << "] "
			<< "status=" << stat1 << "[" << statusStr(stat1, strstat) << "]" 
			<< sendLog;
	ostat1=stat1; ostat0=stat0;
	
	statusRa->setValueInteger (stat0);
	statusDec->setValueInteger (stat1);


	// even if the mount appears after a timeout, do not touch it for another 10s
	if ( wasTimeout )
	{
		logStream (MESSAGE_DEBUG) << "Now StatusGet gets a reply, will wait another 10s" 
			<< sendLog;

		// I may do a real sleep10 in this place and do a proper initHw here
		// instead of the beginning, updateLimits, loadModel etc... that would cause
		//  a power cycle on the mount do a driver reinitialization without further action
		canTouch = getUsec()+10.0;
		//basicInfo();

		offNotMoving = 1;
		wasTimeout = 0;
		maskState (DEVICE_ERROR_HW, 0, "cleared error conditions");
		return 0;
	}

	// updateInfo (should be an extra function)
	doInfo();

	// if the motors are homing we should wait for them to finish and do nothing
	if (stat0 & MOTOR_HOMING || stat1 & MOTOR_HOMING)
		{
		logStream (MESSAGE_DEBUG) 
			<< ((stat0 & MOTOR_HOMING) ? "Axis0 is still homing":"Axis0 is homed" )
			<< ((stat0 & MOTOR_HOMING) ? "Axis1 is still homing":"Axis1 is homed" )
			<< sendLog;
		return 0;
		}

	// need to do homing of at least one axis (this switches the motors on)
	if(forceHoming || (!(stat0 & MOTOR_HOMED)) || (!(stat1 & MOTOR_HOMED)) )
	{
		if (forceHoming || (!(stat0 & MOTOR_HOMED)) )
			{
			if(stat0 & MOTOR_OFF) ret0 = MKS3MotorOn (axis0);
			logStream (MESSAGE_DEBUG) << "Homing Axis0" << sendLog;
			ret0 = MKS3Home (axis0, 0);
			}
	
		if (forceHoming || (!(stat1 & MOTOR_HOMED)) )
			{
			if(stat1 & MOTOR_OFF) ret1 = MKS3MotorOn (axis1);
			logStream (MESSAGE_DEBUG) << "Homing Axis1" << sendLog;
			ret0 = MKS3Home (axis1, 0);
			}
	
		forceHoming=0;

		// the mount is going to be homing for a while no reason to stay here
		return 0;	
	}

	// this is slew+stop, I do not yet know how would I do it precisely
	if(doPark) {
		offNotMoving=1;
		// Ask Slew to the parking pos	
		// set target=ln_ra-dec(parkpos_altaz)
		// doSlew=1;
		doPark=0;
	}

	if(doSlew)
	{
		offNotMoving=0;
		doPark=0;
		doAbort=0;
		// lets assume this does not happen for now
		//if(stat1 & MOTOR_SLEWING) // will have to abort the move

		// if the slew is possible: (homed, not slewing, not indexing, not joysticking...)
		if( (stat0 & MOTOR_HOMED) &&  (!(stat0 & MOTOR_HOMING)) 
			&&  (!(stat0 & MOTOR_INDEXING)) &&  (!(stat0 & MOTOR_SLEWING)) 
			&&  (!(stat0 & MOTOR_JOYSTICKING)) 
			&&  (stat1 & MOTOR_HOMED) &&  (!(stat1 & MOTOR_HOMING))
			&&  (!(stat1 & MOTOR_INDEXING)) &&  (!(stat1 & MOTOR_SLEWING)) 
			&&  (!(stat1 & MOTOR_JOYSTICKING)) )
		{
			CWORD32 ac = 0;
			CWORD32 dc = 0;
			int ret;

			// Switch the motor on if necessary
			if(stat0 & MOTOR_OFF) ret0 = MKS3MotorOn (axis0);
			if(stat1 & MOTOR_OFF) ret1 = MKS3MotorOn (axis1);
			
			// issue the actual move
			ret = sky2counts (ac, dc);
			if (ret) return -1;

			//logStream (MESSAGE_DEBUG) << "MKS3PosRelativeSet axis1" << sendLog;
			//ret1 = MKS3PosRelativeSet (axis1, 0);
			//logStream (MESSAGE_DEBUG) << "_MKS3DoSetVal32 axis1" << sendLog;
			//ret1 = _MKS3DoSetVal32 (axis1, CMD_VAL32_ENCODER_POS, 0);

			logStream (MESSAGE_DEBUG) << "MKS3PosTargetSet axis0" << sendLog;
			ret0 = MKS3PosTargetSet (axis0, (long) ac);
			logStream (MESSAGE_DEBUG) << "MKS3PosTargetSet axis1" << sendLog;
			ret1 = MKS3PosTargetSet (axis1, (long) dc);

			// if that's too far..home us
			if ((ret0 == MAIN_AT_LIMIT) || (ret1 == MAIN_AT_LIMIT))
			{

				if (ret0 == MAIN_AT_LIMIT)
				{
					logStream (MESSAGE_DEBUG) << "MKS3Home axis0" << sendLog;
					ret0 = MKS3Home (axis0, 0);
					moveState |= TEL_FORCED_HOMING0;
					setParkTimeNow ();
				}
				if (ret1 == MAIN_AT_LIMIT)
				{
					logStream (MESSAGE_DEBUG) << "MKS3Home axis1" << sendLog;
					ret1 = MKS3Home (axis1, 0);
					moveState |= TEL_FORCED_HOMING1;
					setParkTimeNow ();
				}
				ret = checkRet ();
				return ret;
			}

			doSlew=0;

			return 0;
		}

		// if the slew is not possible because the mount is already slewing:
		if( (stat0 & MOTOR_HOMED) && (stat1 & MOTOR_HOMED) 
				&&  (!(stat0 & MOTOR_HOMING)) && (!(stat1 & MOTOR_HOMING))
				&&  (!(stat0 & MOTOR_INDEXING)) && (!(stat1 & MOTOR_INDEXING)) 
				&&  (!(stat0 & MOTOR_JOYSTICKING)) && (!(stat1 & MOTOR_JOYSTICKING)) 
				&&  ( ((stat0 & MOTOR_SLEWING)) ||  ((stat1 & MOTOR_SLEWING)) )
		  )
		{
			logStream (MESSAGE_DEBUG) << "Move over move, will have to abort" << sendLog;
			doAbort=1;
		}
	}

	// kill the move
	if(doAbort && (stat0 & MOTOR_HOMED) && (stat1 & MOTOR_HOMED) 
			&&  (!(stat0 & MOTOR_HOMING)) && (!(stat1 & MOTOR_HOMING))
			&&  (!(stat0 & MOTOR_INDEXING)) && (!(stat1 & MOTOR_INDEXING)) 
			&&  (!(stat0 & MOTOR_JOYSTICKING)) && (!(stat1 & MOTOR_JOYSTICKING)) 
			&&  ( ((stat0 & MOTOR_SLEWING)) ||  ((stat1 & MOTOR_SLEWING)) )
	  )
	{	
		logStream (MESSAGE_DEBUG) << "Aborting move" << sendLog;
		if (stat0 & MOTOR_SLEWING ) ret0 = MKS3PosAbort (axis0);
		if (stat1 & MOTOR_SLEWING ) ret1 = MKS3PosAbort (axis1);

		checkRet ();
		sleep(2);

		doAbort=0;
		continue; // do not exit, run doPara again
	} 

	// switch off the motors if not being used
	if( offNotMoving && (stat0 & MOTOR_HOMED) &&  (!(stat0 & MOTOR_HOMING)) 
			&&  (!(stat0 & MOTOR_INDEXING)) &&  (!(stat0 & MOTOR_JOYSTICKING))
			&&  (!(stat0 & MOTOR_SLEWING)) &&  (!(stat0 & MOTOR_OFF)) 
	  )
		MKS3MotorOff (axis0);

	if( offNotMoving && (stat1 & MOTOR_HOMED) &&  (!(stat1 & MOTOR_HOMING)) 
		&&  (!(stat1 & MOTOR_INDEXING)) &&  (!(stat1 & MOTOR_JOYSTICKING))
		&&  (!(stat1 & MOTOR_SLEWING)) &&  (!(stat1 & MOTOR_OFF)) 
		)
		MKS3MotorOff (axis1);

	return 0;
	}
}

int Paramount::idle ()
{
	doPara();

	return GEM::idle ();
}

int Paramount::doInfo ()
{
//	this should update non-critical info "for monitor", and otherwise do nothing
	
	int32_t ac = 0, dc = 0;
	int ret;
	CWORD16 new_status;

	// status should have been done recently (//doPara)
	motorRa->setValueBool (!(statusRa->getValueInteger () & MOTOR_OFF));
	motorDec->setValueBool (!(statusDec->getValueInteger () & MOTOR_OFF));
	
	// motor status..
	ret0 = MKS3MotorStatusGet (axis0, &new_status);
	logStream (MESSAGE_DEBUG) << "MKS3MotorStatusGet axis0: ret0=" << ret0 << " status=" << new_status << sendLog;

	if (motorStatusRa->getValueInteger () != new_status)
	{
		logStream (MESSAGE_DEBUG) << "changed motor 0 state from " << std::hex << motorStatusRa->getValueInteger () << " to " << new_status << sendLog;
		motorStatusRa->setValueInteger (new_status);
	}

	ret1 = MKS3MotorStatusGet (axis1, &new_status);
	logStream (MESSAGE_DEBUG) << "MKS3MotorStatusGet axis1: ret1=" << ret1 << " status=" << new_status << sendLog;

	if (motorStatusDec->getValueInteger () != new_status)
	{
		logStream (MESSAGE_DEBUG) << "changed motor 1 state from " << std::hex << motorStatusDec->getValueInteger () << " to " << new_status << sendLog;
		motorStatusDec->setValueInteger (new_status);
	}

	ret0 = MKS3PosCurGet (axis0, &ac);
	ret1 = MKS3PosCurGet (axis1, &dc);
	ret = checkRet ();
	if (ret) return ret;

	double t_telRa;
	double t_telDec;
	double ut_telDec;

	ret = counts2sky (ac, dc, t_telRa, t_telDec, ut_telDec);

	setTelRa (t_telRa);
	setTelDec (ut_telDec);
	axRa->setValueLong (ac);
	axDec->setValueLong (dc);

	ret = getHomeOffsetAxis (axis0, ac);
	if (ret) return ret;
	ret = getHomeOffsetAxis (axis1, dc);
	if (ret) return ret;

	homeOffsetRa->setValueLong (ac);
	homeOffsetDec->setValueLong (dc);

	ret0 = MKS3PosEncoderGet (axis0, &ac);
	ret1 = MKS3PosEncoderGet (axis1, &dc);
	ret = checkRet ();
	if (ret) return ret;

	encoderRa->setValueLong (ac);
	encoderDec->setValueLong (dc);

  	ret = getParamountValue32 (CMD_VAL32_BASERATE, baseRa, baseDec);
	if (ret) return ret;

	ret = getParamountValue32 (CMD_VAL32_CREEP_VEL, creepRa, creepDec);
	if (ret) return ret;

	return 0;
}

//int Paramount::info ()
//{
	// this should do almost nothing,i.e. not even doPara()?
	//doPara();
//	return GEM::info ();
//}

int Paramount::startResync ()
{
	doSlew=1;
	doPara();
	return 0;
}

int Paramount::isMoving ()
{
	doPara();

	if ( (statusRa->getValueInteger () & MOTOR_SLEWING) 
		|| (statusDec->getValueInteger () & MOTOR_SLEWING))
		return USEC_SEC / 10;

	return -2;
}

int Paramount::stopMove ()
{
	// there may be an unfinished slew request
	doSlew=0;
	
	// in any case 
	doAbort=1;

	doPara();

	return 0;
}

int Paramount::startPark ()
{
	forceHoming=1;
	doPark=1;

	doPara();

	return 0;
}

int Paramount::isParking ()
{
	doPara();
	if ((statusRa->getValueInteger () & MOTOR_HOMING) || (statusDec->getValueInteger () & MOTOR_HOMING))
		return USEC_SEC / 10;
	
	return -2;
}

int Paramount::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	logStream (MESSAGE_DEBUG) << "Paramount::setValue" << sendLog;
	if (oldValue == tracking)
	{
		if (((rts2core::ValueBool * ) newValue)->getValueBool () == true)
			{
			logStream (MESSAGE_DEBUG) << "MKS3MotorOn6 axis0" << sendLog;
			MKS3MotorOn (axis0);
			}
		else
			{
			logStream (MESSAGE_DEBUG) << "MKS3MotorOff6 axis0" << sendLog;
			MKS3MotorOff (axis0);
			}
		return 0;

	}
	if (oldValue == speedRa)
		return setParamountValue32 (CMD_VAL32_SLEW_VEL, newValue, speedDec) ? -2 : 0;
	if (oldValue == speedDec)
		return setParamountValue32 (CMD_VAL32_SLEW_VEL, speedRa, newValue) ? -2 : 0;
	if (oldValue == accRa)
		return setParamountValue32 (CMD_VAL32_SQRT_ACCEL, newValue, accDec) ? -2 : 0;
	if (oldValue == accDec)
		return setParamountValue32 (CMD_VAL32_SQRT_ACCEL, accRa, newValue) ? -2 : 0;
	if (oldValue == homevelRaHi)
		return setParamountValue32 (CMD_VAL32_HOMEVEL_HI, newValue, homevelDecHi) ? -2 : 0;
	if (oldValue == homevelDecHi)
		return setParamountValue32 (CMD_VAL32_HOMEVEL_HI, homevelRaHi, newValue) ? -2 : 0;
	if (oldValue == homevelRaMed)
		return setParamountValue32 (CMD_VAL32_HOMEVEL_MED, newValue, homevelDecMed) ? -2 : 0;
	if (oldValue == homevelDecMed)
		return setParamountValue32 (CMD_VAL32_HOMEVEL_MED, homevelRaMed, newValue) ? -2 : 0;
	if (oldValue == homevelRaLow)
		return setParamountValue32 (CMD_VAL32_HOMEVEL_LO, newValue, homevelDecLow) ? -2 : 0;
	if (oldValue == homevelDecLow)
		return setParamountValue32 (CMD_VAL32_HOMEVEL_LO, homevelRaLow, newValue) ? -2 : 0;
	if (oldValue == baseRa)
		return setParamountValue32 (CMD_VAL32_BASERATE, newValue, baseDec) ? -2 : 0;
	if (oldValue == baseDec)
		return setParamountValue32 (CMD_VAL32_BASERATE, baseRa, newValue) ? -2 : 0;
	if (oldValue == creepRa)
		return setParamountValue32 (CMD_VAL32_CREEP_VEL, newValue, creepDec) ? -2 : 0;
	if (oldValue == creepDec)
		return setParamountValue32 (CMD_VAL32_CREEP_VEL, creepRa, newValue) ? -2 : 0;
	if (oldValue == relRa)
		return setParamountValue32 (CMD_VAL32_RELPOS, newValue, relDec) ? -2 : 0;
	if (oldValue == relDec)
		return setParamountValue32 (CMD_VAL32_RELPOS, relRa, newValue) ? -2 : 0;
	if (oldValue == motorRa)
		{
		logStream (MESSAGE_DEBUG) << "MKS3MotorOn/Off axis0" << sendLog;
		return (((rts2core::ValueBool *) newValue)->getValueBool () ? MKS3MotorOn (axis0) : MKS3MotorOff (axis0)) == MKS_OK ? 0 : -2;
		}
	if (oldValue == motorDec)
		{
		logStream (MESSAGE_DEBUG) << "MKS3MotorOn/Off axis1" << sendLog;
		return (((rts2core::ValueBool *) newValue)->getValueBool () ? MKS3MotorOn (axis1) : MKS3MotorOff (axis1)) == MKS_OK ? 0 : -2;
		}
	return Telescope::setValue (oldValue, newValue);
}

int Paramount::endPark ()
{
	return 0;
}

int Paramount::saveFlash ()
{
	logStream (MESSAGE_DEBUG) << "Paramount::saveFlash" << sendLog;
	CWORD16 data[4000];
	int file = open ("/etc/rts2/flash", O_CREAT | O_TRUNC);
	logStream (MESSAGE_DEBUG) << "MKS3UserSpaceRead axis0" << sendLog;
	MKS3UserSpaceRead (axis0, 0, 4000, data);
	write (file, data, 4000 * sizeof (CWORD16));
	close (file);
	return 0;
}

int Paramount::saveModel ()
{
	// dump Paramount stuff
	int ret;
	logStream (MESSAGE_DEBUG) << "Paramount::saveModel" << sendLog;
	std::ofstream os (paramount_cfg);
	if (os.fail ())
		return -1;
	ret = saveAxis (os, axis0);
	if (ret)
		return -1;
	ret = saveAxis (os, axis1);
	if (ret)
		return -1;
	os.close ();
	return 0;
}

int Paramount::loadModelFromFile ()
{
	logStream (MESSAGE_DEBUG) << "Paramount::loadModelFromFile" << sendLog;
	std::string name;
	std::ifstream is (paramount_cfg);
	MKS3Id *used_axe = NULL;
	int axisId;
	int ret;

	if (is.fail ())
		return -1;
	while (!is.eof ())
	{
		is >> name;
		// find a comment, swap axis
		if (name.find ("**") == 0)
		{
			is >> name >> axisId;
			is.ignore (2000, is.widen ('\n'));
			if (is.fail ())
				return -1;
			switch (axisId)
			{
				case 0:
					used_axe = &axis0;
					break;
				case 1:
					used_axe = &axis1;
					break;
				default:
					return -1;
			}
		}
		else
		{
			// read until ":"
			while (true)
			{
				if (name.find (':') != name.npos)
					break;
				std::string name_ap;
				is >> name_ap;
				if (is.fail ())
					return 0;
				name = name + std::string (" ") + name_ap;
			}
			name = name.substr (0, name.size () - 1);
			// find value and load it
			for (std::vector < ParaVal >::iterator iter =
				paramountValues.begin (); iter != paramountValues.end ();
				iter++)
			{
				if ((*iter).isName (name))
				{
					if (!used_axe)
						return -1;
					// found value
					ret = (*iter).readAxis (is, *used_axe);
					if (ret)
					{
						logStream (MESSAGE_ERROR) << "Error setting value for "
							<< name << ", ret: " << ret << sendLog;
						return -1;
					}
					is.ignore (2000, is.widen ('\n'));
				}
			}
		}
	}
	return 0;
}

int Paramount::loadModel ()
{
	int ret;

	logStream (MESSAGE_DEBUG) << "Paramount::loadModel" << sendLog;
	ret = loadModelFromFile ();
	if (ret)
		return ret;

	logStream (MESSAGE_DEBUG) << "MKS3ConstsStore axis0" << sendLog;
	ret0 = MKS3ConstsStore (axis0);
	logStream (MESSAGE_DEBUG) << "MKS3ConstsStore axis1" << sendLog;
	ret1 = MKS3ConstsStore (axis1);
	ret = checkRet ();
	if (ret)
		return -1;

	logStream (MESSAGE_DEBUG) << "MKS3ConstsReload axis0" << sendLog;
	ret0 = MKS3ConstsReload (axis0);
	logStream (MESSAGE_DEBUG) << "MKS3ConstsReload axis1" << sendLog;
	ret1 = MKS3ConstsReload (axis1);
	ret = checkRet ();
	if (ret)
		return -1;
	return 0;
}

void Paramount::setDiffTrack (double dra, double ddec)
{
	// calculate diff track..
	logStream (MESSAGE_DEBUG) << "Paramount::setDiffTrack" << sendLog;
	dra = (-dra * haCpd);
	ddec = (ddec * decCpd);
	if (getFlip ())
		ddec *= -1;
	baseRa->setValueLong (dra + hourRa->getValueLong ());
	baseDec->setValueLong (ddec);
	setParamountValue32 (CMD_VAL32_BASERATE, baseRa, baseDec);
}

int Paramount::getParamountValue32 (int id, rts2core::ValueLong *vRa, rts2core::ValueLong *vDec)
{
	//logStream (MESSAGE_DEBUG) << "Paramount::getParamountValue32" << sendLog;
  	CWORD32 _ra,_dec;
	//logStream (MESSAGE_DEBUG) << "_MKS3DoGetVal32 axis0" << sendLog;
	ret0 = _MKS3DoGetVal32 (axis0, id, &_ra);
	//logStream (MESSAGE_DEBUG) << "_MKS3DoGetVal32 axis1" << sendLog;
	ret1 = _MKS3DoGetVal32 (axis1, id, &_dec);
	if (checkRet ())
	{
		logStream (MESSAGE_ERROR) << "cannot get 32 bit value with ID " << id << sendLog;
		return -1;
	}
	vRa->setValueLong (_ra);
	vDec->setValueLong (_dec);
	return 0;
}

int Paramount::setParamountValue32 (int id, rts2core::Value *vRa, rts2core::Value *vDec)
{
	logStream (MESSAGE_DEBUG) << "Paramount::setParamountValue32" << sendLog;
	CWORD32 _ra = vRa->getValueLong ();
	CWORD32 _dec = vDec->getValueLong ();
	//logStream (MESSAGE_DEBUG) << "_MKS3DoSetVal32 axis0" << sendLog;
	ret0 = _MKS3DoSetVal32 (axis0, id, _ra);
	//logStream (MESSAGE_DEBUG) << "_MKS3DoSetVal32 axis0" << sendLog;
	ret1 = _MKS3DoSetVal32 (axis1, id, _dec);
	if (checkRet ())
	{
		logStream (MESSAGE_ERROR) << "cannot set 32 bit value with ID " << id << sendLog;
		return -1;
	}
	return 0;
}

int main (int argc, char **argv)
{
	Paramount device (argc, argv);
	return device.run ();
}
