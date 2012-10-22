/* 
 * Driver for Paramount. You need ParaLib to make that work, please ask petr@kubanek.net
 * for details.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
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
	_os << name << ": ";
	switch (type)
	{
		case T16:
			ret = _MKS3DoGetVal16 (axis, id, &val16);
			if (ret)
			{
				_os << "error" << std::endl;
				return -1;
			}
			_os << val16 << std::endl;
			break;
		case T32:
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
	switch (type)
	{
		case T16:
			_is >> val16;
			if (_is.fail ())
				return -1;
			ret = _MKS3DoSetVal16 (axis, id, val16);
			break;
		case T32:
			_is >> val32;
			if (_is.fail ())
				return -1;
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

		virtual int info ();

	protected:
		virtual int init ();
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
		virtual int endMove ();
		virtual int stopMove ();

		virtual int startPark ();
		virtual int endPark ();

		// save and load gemini informations
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

		CWORD16 status0;
		CWORD16 status1;

		rts2core::ValueInteger *statusRa;
		rts2core::ValueInteger *statusDec;

		rts2core::ValueLong *speedRa;
		rts2core::ValueLong *speedDec;
		rts2core::ValueLong *accRa;
		rts2core::ValueLong *accDec;

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

		// variables for tracking
		struct timeval track_start_time;
		struct timeval track_recalculate;
		struct timeval track_next;
		MKS3ObjTrackInfo *track0;
		MKS3ObjTrackInfo *track1;

		int getHomeOffsetAxis (MKS3Id axis, int32_t & off);

		// return RA and DEC value pair
		int getParamountValue32 (int id, rts2core::ValueLong *vRa, rts2core::ValueLong *vDec);
		int setParamountValue32 (int id, rts2core::Value *vRa, rts2core::Value *vDec);
};

};

using namespace rts2teld;

int Paramount::checkRetAxis (const MKS3Id & axis, int reta)
{
	const char *msg = NULL;
	messageType_t msg_type = MESSAGE_ERROR;
	switch (reta)
	{
		case COMM_OKPACKET:
			msg = "comm packet OK";
			msg_type = MESSAGE_DEBUG;
			break;
		case COMM_NOPACKET:
			msg = "comm no packet";
			break;
		case COMM_TIMEOUT:
			msg = "comm timeout (check cable!)";
			break;
		case COMM_COMMERROR:
			msg = "comm error";
			break;
		case COMM_BADCHAR:
			msg = "bad char at comm";
			break;
		case COMM_OVERRUN:
			msg = "comm overrun";
			break;
		case COMM_BADCHECKSUM:
			msg = "comm bad checksum";
			break;
		case COMM_BADLEN:
			msg = "comm bad message len";
			break;
		case COMM_BADCOMMAND:
			msg = "comm bad command";
			break;
		case COMM_INITFAIL:
			msg = "comm init fail";
			break;
		case COMM_NACK:
			msg = "comm not acknowledged";
			break;
		case COMM_BADID:
			msg = "comm bad id";
			break;
		case COMM_BADSEQ:
			msg = "comm bad sequence";
			break;
		case COMM_BADVALCODE:
			msg = "comm bad value code";
			break;

		case MAIN_WRONG_UNIT:
			msg = "main wrong unit";
			break;
		case MAIN_BADMOTORINIT:
			msg = "main bad motor init";
			break;
		case MAIN_BADMOTORSTATE:
			msg = "main bad motor state";
			break;
		case MAIN_BADSERVOSTATE:
			msg = "main bad servo state";
			break;
		case MAIN_SERVOBUSY:
			msg = "main servo busy";
			break;
		case MAIN_BAD_PEC_LENGTH:
			msg = "main bad pec lenght";
			break;
		case MAIN_AT_LIMIT:
			msg = "main at limit";
			break;
		case MAIN_NOT_HOMED:
			msg = "main not homed";
			break;
		case MAIN_BAD_POINT_ADD:
			msg = "main bad point add";
			break;

		case FLASH_PROGERR:
			msg = "flash program error";
			break;
			/*    case FLASH_ERASEERR:
				  msg = "flash erase error";
				  break; */
		case FLASH_TIMEOUT:
			msg = "flash timeout";
			break;
		case FLASH_CANT_OPEN_FILE:
			msg = "flash cannot open file";
			break;
		case FLASH_BAD_FILE:
			msg = "flash bad file";
			break;
		case FLASH_FILE_READ_ERR:
			msg = "flash file read err";
			break;
		case FLASH_BADVALID:
			msg = "flash bad value id";
			break;

		case MOTOR_OK:
			msg = "motor ok";
			msg_type = MESSAGE_DEBUG;
			break;
		case MOTOR_OVERCURRENT:
			msg = "motor over current";
			break;
		case MOTOR_POSERRORLIM:
			msg = "motor maximum position error exceeded";
			break;
		case MOTOR_STILL_ON:
			msg = "motor is on but command needs it at off state";
			break;
		case MOTOR_NOT_ON:
			msg = "motor off";
			break;
		case MOTOR_STILL_MOVING:
			msg = "motor still slewing but command needs it stopped";
			break;
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
	if (reta0 != MKS_OK || reta1 != MKS_OK)
		return -1;
	return 0;
}

int Paramount::updateStatus ()
{
	CWORD16 old_status = status0;
	ret0 = MKS3StatusGet (axis0, &status0);
	if (status0 != old_status)
		logStream (MESSAGE_DEBUG) << "changed axis 0 state from " << std::hex << old_status << " to " << status0 << sendLog;
	statusRa->setValueInteger (status0);
	old_status = status1;
	ret1 = MKS3StatusGet (axis1, &status1);
	if (status1 != old_status)
		logStream (MESSAGE_DEBUG) << "changed axis 1 state from " << std::hex << old_status << " to " << status1 << sendLog;
	statusDec->setValueInteger (status1);

	// set / unset HW error
	if ((status0 & 0x02) || (status1 & 0x02) || status0 == 0x500 || status1 == 0x500 || status0 == 0xd00 || status1 == 0xd00 || status0 == 0xc00 || status1 == 0xc00)
		maskState (DEVICE_ERROR_HW, DEVICE_ERROR_HW, "set error - axis failed");
	else
		maskState (DEVICE_ERROR_HW, 0, "cleared error conditions");

	return checkRet ();
}

int Paramount::saveAxis (std::ostream & os, const MKS3Id & axis)
{
	int ret;
	CWORD16 pMajor, pMinor, pBuild;

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

	ret = MKS3PosEncoderGet (axis, &en0);
	if (ret != MKS_OK)
		return ret;
	ret = MKS3PosCurGet (axis, &pos0);
	if (ret != MKS_OK)
		return ret;
	off = en0 - pos0;
	return 0;
}

int Paramount::getHomeOffset (int32_t & off)
{
	return getHomeOffsetAxis (axis0, off);
}

int Paramount::updateLimits ()
{
	int ret;
	ret = MKS3ConstsLimMinGet (axis0, &acMin);
	if (ret)
		return -1;
	ret = MKS3ConstsLimMaxGet (axis0, &acMax);
	if (ret)
		return -1;

	ret = MKS3ConstsLimMinGet (axis1, &dcMin);
	if (ret)
		return -1;
	ret = MKS3ConstsLimMaxGet (axis1, &dcMax);
	if (ret)
		return -1;
	return 0;
}

Paramount::Paramount (int in_argc, char **in_argv):GEM (in_argc, in_argv, true)
{
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

	createValue (speedRa, "SPEED_RA", "[???] RA axis maximal speed", false, RTS2_VALUE_WRITABLE);
	createValue (speedDec, "SPEED_DEC", "[???] DEC axis maximal speed", false, RTS2_VALUE_WRITABLE);
	createValue (accRa, "ACC_RA", "[???] RA axis acceleration", false, RTS2_VALUE_WRITABLE);
	createValue (accDec, "ACC_DEC", "[???] DEC axis acceleration", false, RTS2_VALUE_WRITABLE);

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

	ra_ticks = RA_TICKS;
	dec_ticks = DEC_TICKS;

	setIndices = false;

	device_name = "/dev/ttyS0";
	paramount_cfg = "/etc/rts2/paramount.cfg";
	addOption ('f', "device_name", 1, "device file (default /dev/ttyS0");
	addOption ('P', "paramount_cfg", 1, "paramount config file (default /etc/rts2/paramount.cfg");
	addOption ('R', "recalculate", 1, "track update interval in sec; < 0 to disable track updates; defaults to 1 sec");
	addOption ('t', "set indices", 0, "if we need to load indices as first operation");

	addOption ('D', "dec_park", 1, "DEC park position");
	// in degrees! 30 for south, -30 for north hemisphere; S swap is done later
	// it's (lst - ra(home))
	// haZero and haCpd S swap are handled in ::init, after we get latitude from config file
	haZero = -30.0;
	decZero = 0;

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
		case 'D':
			park_axis[1] = atoi (optarg);
			break;
		default:
			return GEM::processOption (in_opt);
	}
	return 0;
}

int Paramount::init ()
{
	CWORD16 motorState0, motorState1;
	CWORD32 pos0, pos1;
	int ret;
	int i;

	ret = GEM::init ();
	if (ret)
		return ret;

	rts2core::Configuration *config = rts2core::Configuration::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	telLongitude->setValueDouble (config->getObserver ()->lng);
	telLatitude->setValueDouble (config->getObserver ()->lat);
								 // south hemispehere
	if (telLatitude->getValueDouble () < 0)
	{
		// swap values which are opposite for south hemispehere
		haZero *= -1.0;
		haCpd *= -1.0;
		hourRa->setValueLong (-1 * hourRa->getValueLong ());
	}

	ret = MKS3Init ((char *) device_name);
	if (ret)
		return -1;

	ret = updateLimits ();
	//	if (ret)
	//		return -1;

	if (setIndices)
	{
		ret = loadModel ();
		if (ret)
			return -1;
	}

	moveState = TEL_SLEW;

	ret0 = MKS3MotorOn (axis0);
	ret1 = MKS3MotorOn (axis1);
	checkRet ();

	ret0 = MKS3PosAbort (axis0);
	ret1 = MKS3PosAbort (axis1);
	checkRet ();

	// wait till it stop slew
	for (i = 0; i < 10; i++)
	{
		ret = updateStatus ();
		if (ret)
			return ret;
		ret0 = MKS3MotorStatusGet (axis0, &motorState0);
		ret1 = MKS3MotorStatusGet (axis1, &motorState1);
		ret = checkRet ();
		if (ret)
			return ret;
		if (!((motorState0 & MOTOR_SLEWING) || (motorState1 & MOTOR_SLEWING)))
			break;
		if ((motorState0 & MOTOR_POSERRORLIM)
			|| (motorState1 & MOTOR_POSERRORLIM))
		{
			ret0 = MKS3ConstsMaxPosErrorSet (axis0, 16000);
			ret1 = MKS3ConstsMaxPosErrorSet (axis1, 16000);
			ret = checkRet ();
			if (ret)
				return ret;
			i = 10;
			break;
		}
		sleep (1);
	}

	if (i == 10)
	{
		ret0 = MKS3PosCurGet (axis0, &pos0);
		ret1 = MKS3PosCurGet (axis1, &pos1);
		ret = checkRet ();
		if (ret)
			return ret;
		ret0 = MKS3PosTargetSet (axis0, pos0);
		ret1 = MKS3PosTargetSet (axis1, pos1);
		ret = checkRet ();
		//		if (ret)
		//			return ret;
	}

	ret0 = MKS3MotorOff (axis0);
	ret1 = MKS3MotorOff (axis1);
	ret = checkRet ();
	//	if (ret)
	//return ret;

	CWORD16 pMajor, pMinor, pBuild;
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

	return ret;
}

int Paramount::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("sleep"))
	{
		if (!conn->paramEnd ())
			return -2;
		sleep (120);
		return 0;
	}
	else if (conn->isCommand ("save_flash"))
	{
		ret0 = MKS3ConstsStore (axis0);
		ret1 = MKS3ConstsStore (axis1);
		int ret = checkRet ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot store constants");
			return -1;
		}

		ret0 = MKS3ConstsReload (axis0);
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

int Paramount::idle ()
{
//	struct timeval now;
	int32_t homeOff, ac = 0;
	int ret;
	// check if we aren't close to RA limit
	ret = MKS3PosCurGet (axis0, &ac);
	if (ret)
	{
		sleep (10);
		MKS3Free ();
		MKS3Init ((char *) device_name);
		return rts2core::Device::idle ();
	}
	ret = getHomeOffset (homeOff);
	if (ret)
	{
		sleep (10);
		return rts2core::Device::idle ();
	}
	ac += homeOff;
	if (telLatitude->getValueDouble () < 0)
	{
		if ((ac + acMargin) > acMax)
			needStop ();
	}
	else
	{
		if ((ac - acMargin) < acMin)
			needStop ();
	}
	ret = updateStatus ();
	if (ret)
	{
		// give mount time to recover
		sleep (10);
		// don't check for move etc..
		return rts2core::Device::idle ();
	}
	// if we need homing..
/*	if ((!(status0 & MOTOR_HOMED)) && (!(status0 & MOTOR_HOMING)))
	{
		logStream (MESSAGE_INFO) <<
			"Detected not homed axis0, force homing.. (axis0:" << status0 <<
			")" << sendLog;
		//startPark (NULL);
	}
	else if ((!(status1 & MOTOR_HOMED)) && (!(status1 & MOTOR_HOMING)))
	{
		logStream (MESSAGE_INFO) <<
			"Detected not homed axis1, force homing.. (axis1:" << status1 <<
			")" << sendLog;
		//startPark (NULL);
	} */
	// issue new track request..if needed and we aren't homing
	/*  if ((getState () & TEL_MASK_MOVING) == TEL_OBSERVING
		  && !(status0 & MOTOR_HOMING) && !(status0 & MOTOR_SLEWING)
		  && !(status1 & MOTOR_HOMING) && !(status1 & MOTOR_SLEWING))
		{
		  gettimeofday (&now, NULL);
		  if (timercmp (&track_next, &now, <))
		updateTrack ();
		} */
	// check for some critical stuff
	return GEM::idle ();
}

int Paramount::info ()
{
	int32_t ac = 0, dc = 0;
	int ret;
	ret0 = MKS3PosCurGet (axis0, &ac);
	ret1 = MKS3PosCurGet (axis1, &dc);
	ret = checkRet ();
	if (ret)
		return ret;
	double t_telRa;
	double t_telDec;
	double ut_telDec;
	ret = counts2sky (ac, dc, t_telRa, t_telDec, ut_telDec);
	setTelRa (t_telRa);
	setTelDec (ut_telDec);
	axRa->setValueLong (ac);
	axDec->setValueLong (dc);

	ret = getHomeOffsetAxis (axis0, ac);
	if (ret)
		return ret;
	ret = getHomeOffsetAxis (axis1, dc);
	if (ret)
		return ret;
	homeOffsetRa->setValueLong (ac);
	homeOffsetDec->setValueLong (dc);

	ret0 = MKS3PosEncoderGet (axis0, &ac);
	ret1 = MKS3PosEncoderGet (axis1, &dc);
	ret = checkRet ();
	if (ret)
		return ret;
	encoderRa->setValueLong (ac);
	encoderDec->setValueLong (dc);

  	ret = getParamountValue32 (CMD_VAL32_BASERATE, baseRa, baseDec);
	if (ret)
		return ret;

	ret = getParamountValue32 (CMD_VAL32_CREEP_VEL, creepRa, creepDec);
	if (ret)
		return ret;

	ret = updateStatus ();
	if (ret)
		return ret;
	motorRa->setValueBool (!(status0 & MOTOR_OFF));
	motorDec->setValueBool (!(status1 & MOTOR_OFF));

	return GEM::info ();
}

int Paramount::startResync ()
{
	int ret;
	CWORD32 ac = 0;
	CWORD32 dc = 0;

	delete track0;
	delete track1;

	track0 = NULL;
	track1 = NULL;

	ret = updateStatus ();
	if (ret)
		return -1;

	ret0 = MKS_OK;
	ret1 = MKS_OK;
	// when we are homing, we will move after home finish
	if (status0 & MOTOR_HOMING)
	{
		moveState = TEL_FORCED_HOMING0;
		return 0;
	}
	if (status1 & MOTOR_HOMING)
	{
		moveState = TEL_FORCED_HOMING1;
		return 0;
	}
	if ((status0 & MOTOR_SLEWING) || (status1 & MOTOR_SLEWING))
	{
		logStream (MESSAGE_DEBUG) << "Aborting move, as mount is slewing" << sendLog;
		if (status0 & MOTOR_SLEWING)
		{
			ret0 = MKS3PosAbort (axis0);
			MKS3MotorOff (axis0);
		}
		if (status1 & MOTOR_SLEWING)
		{
			ret1 = MKS3PosAbort (axis1);
			MKS3MotorOff (axis1);
		}
		sleep (2);
		ret = checkRet ();
		if (ret)
			return -1;
	}
	if ((status0 & MOTOR_OFF) || (status1 & MOTOR_OFF))
	{
		ret0 = MKS3MotorOn (axis0);
		ret1 = MKS3MotorOn (axis1);
		usleep (USEC_SEC / 10);
	}
	if (!(status0 & MOTOR_HOMED))
	{
		MKS3Home (axis0, 0);
		moveState |= TEL_FORCED_HOMING0;
	}
	if (!(status1 & MOTOR_HOMED))
	{
		MKS3Home (axis1, 1);
		moveState |= TEL_FORCED_HOMING1;
	}
	if (moveState & (TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1))
	{
		setParkTimeNow ();
		logStream (MESSAGE_DEBUG) << "homing needed, aborting move command" << sendLog;
		return 0;
	}

	ret = checkRet ();
	if (ret)
	{
		return -1;
	}

	ret = sky2counts (ac, dc);
	if (ret)
	{
		return -1;
	}

	moveState = TEL_SLEW;

	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "Paramount::startResync " << ac << " " << dc << sendLog;
	#endif						 /* DEBUG_EXTRA */

	ret1 = MKS3PosRelativeSet (axis1, 0);

	ret1 = _MKS3DoSetVal32 (axis1, CMD_VAL32_ENCODER_POS, 0);

	ret0 = MKS3PosTargetSet (axis0, (long) ac);
	ret1 = MKS3PosTargetSet (axis1, (long) dc);

	// if that's too far..home us
	if (ret0 == MAIN_AT_LIMIT)
	{
		ret0 = MKS3Home (axis0, 0);
		moveState |= TEL_FORCED_HOMING0;
		setParkTimeNow ();
	}
	if (ret1 == MAIN_AT_LIMIT)
	{
		ret1 = MKS3Home (axis1, 0);
		moveState |= TEL_FORCED_HOMING1;
		setParkTimeNow ();
	}
	ret = checkRet ();
	if (ret)
		return ret;
	return 0;
}

int Paramount::isMoving ()
{
	// we were called from idle loop
	if (moveState & (TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1))
	{
		if ((status0 & MOTOR_HOMING) || (status1 & MOTOR_HOMING))
			return USEC_SEC / 10;
		moveState = TEL_SLEW;
		// re-move
		return startResync ();
	}
	// check axis state..
	if (status0 & SERVO_STATE_UN)
	{
		sleep (10);
		// switch motor off
		MKS3MotorOff (axis0);
		sleep (10);
		MKS3MotorOn (axis0);
		MKS3Home (axis0, 0);
		moveState |= TEL_FORCED_HOMING0;
		setParkTimeNow ();
		return USEC_SEC / 10;
	}
	if (status1 & SERVO_STATE_UN)
	{
		sleep (10);
		// switch motor off
		MKS3MotorOff (axis1);
		sleep (10);
		MKS3MotorOn (axis1);
		MKS3Home (axis1, 0);
		moveState |= TEL_FORCED_HOMING1;
		setParkTimeNow ();
		return USEC_SEC / 10;
	}
	if ((status0 & MOTOR_SLEWING) || (status1 & MOTOR_SLEWING))
		return USEC_SEC / 10;
	// we reached destination
	return -2;
}

int Paramount::endMove ()
{
	int ret;
	//  int ret_track;
	ret0 = MKS3MotorOn (axis0);
	ret1 = MKS3MotorOn (axis1);
	ret = checkRet ();
	// init tracking structures
	gettimeofday (&track_start_time, NULL);
	timerclear (&track_next);
	/*  if (!track0)
		track0 = new MKS3ObjTrackInfo ();
	  if (!track1)
		track1 = new MKS3ObjTrackInfo ();
	  ret0 = MKS3ObjTrackInit (axis0, track0);
	  ret1 = MKS3ObjTrackInit (axis1, track1);
	  ret_track = checkRet ();
	  if (!ret_track)
		updateTrack ();
	#ifdef DEBUG_EXTRA
	  logStream(MESSAGE_DEBUG) << "Track init " << ret0 << " " << ret1 << sendLog;
	#endif // DEBUG_EXTRA */
	// 1 sec sleep to get time to settle down
	sleep (1);
	if (ret)
		return ret;
	ret = setParamountValue32 (CMD_VAL32_BASERATE, baseRa, baseDec);
	if (ret)
	  	return ret;
	return GEM::endMove ();
}

int Paramount::stopMove ()
{
	int ret;
	// if we issue startMove after abort, we will get to position after homing is performed
	if ((moveState & TEL_FORCED_HOMING0) || (moveState & TEL_FORCED_HOMING1))
		return 0;
	// check if we are homing..
	ret = updateStatus ();
	if (ret)
		return -1;
	if ((status0 & MOTOR_HOMING) || (status1 & MOTOR_HOMING))
		return 0;
	ret0 = MKS3PosAbort (axis0);
	ret1 = MKS3PosAbort (axis1);
	return checkRet ();
}

int Paramount::startPark ()
{
	int ret;

	// if parking is currently going on, do not park again
	if (getState () & TEL_PARKING)
		return 0;

	delete track0;
	delete track1;

	track0 = NULL;
	track1 = NULL;

	ret0 = MKS3MotorOn (axis0);
	ret1 = MKS3MotorOn (axis1);
	ret = checkRet ();
	if (ret)
		return -1;

	if ((status0 & MOTOR_SLEWING) || (status1 & MOTOR_SLEWING))
	{
		if (status0 & MOTOR_SLEWING)
		{
			ret0 = MKS3PosAbort (axis0);
		}
		if (status1 & MOTOR_SLEWING)
		{
			ret0 = MKS3PosAbort (axis1);
		}
		if (ret)
			return -1;
	}

	ret0 = MKS3Home (axis0, 0);
	ret1 = MKS3Home (axis1, 0);
	moveState = TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1;
	return checkRet ();
}

int Paramount::isParking ()
{
	int ret;
	if (moveState & (TEL_FORCED_HOMING0 | TEL_FORCED_HOMING1))
	{
		if ((status0 & MOTOR_HOMING) || (status1 & MOTOR_HOMING))
			return USEC_SEC / 10;
		moveState = TEL_SLEW;
		// move to park position
		ret0 = MKS3PosTargetSet (axis0, park_axis[0]);
		ret1 = MKS3PosTargetSet (axis1, park_axis[1]);
		ret = updateStatus ();
		if (ret)
			return -1;
		return USEC_SEC / 10;
	}
	// home finished, only check if we get to proper position
	if ((status0 & MOTOR_SLEWING) || (status1 & MOTOR_SLEWING))
		return USEC_SEC / 10;
	return -2;
}

int Paramount::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == tracking)
	{
		if (((rts2core::ValueBool * ) newValue)->getValueBool () == true)
			MKS3MotorOn (axis0);
		else
			MKS3MotorOff (axis0);
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
		return (((rts2core::ValueBool *) newValue)->getValueBool () ? MKS3MotorOn (axis0) : MKS3MotorOff (axis0)) == MKS_OK ? 0 : -2;
	if (oldValue == motorDec)
		return (((rts2core::ValueBool *) newValue)->getValueBool () ? MKS3MotorOn (axis1) : MKS3MotorOff (axis1)) == MKS_OK ? 0 : -2;

	return Telescope::setValue (oldValue, newValue);
}

int Paramount::endPark ()
{
	int ret;
	ret = updateStatus ();
	if (ret)
		return -1;
	if (!(status0 & MOTOR_HOMED) || !(status1 & MOTOR_HOMED))
		return -1;
	ret0 = MKS3MotorOff (axis0);
	ret1 = MKS3MotorOff (axis1);
	return checkRet ();
}

int Paramount::saveFlash ()
{
	CWORD16 data[4000];
	int file = open ("/etc/rts2/flash", O_CREAT | O_TRUNC);
	MKS3UserSpaceRead (axis0, 0, 4000, data);
	write (file, data, 4000 * sizeof (CWORD16));
	close (file);
	return 0;
}

int Paramount::saveModel ()
{
	// dump Paramount stuff
	int ret;
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

	ret = loadModelFromFile ();
	if (ret)
		return ret;

	ret0 = MKS3ConstsStore (axis0);
	ret1 = MKS3ConstsStore (axis1);
	ret = checkRet ();
	if (ret)
		return -1;

	ret0 = MKS3ConstsReload (axis0);
	ret1 = MKS3ConstsReload (axis1);
	ret = checkRet ();
	if (ret)
		return -1;
	return 0;
}

void Paramount::setDiffTrack (double dra, double ddec)
{
	// calculate diff track..
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
  	CWORD32 _ra,_dec;
	ret0 = _MKS3DoGetVal32 (axis0, id, &_ra);
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
	CWORD32 _ra = vRa->getValueLong ();
	CWORD32 _dec = vDec->getValueLong ();
	ret0 = _MKS3DoSetVal32 (axis0, id, _ra);
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
