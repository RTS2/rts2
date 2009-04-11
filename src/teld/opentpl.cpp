/* 
 * Driver for OpenTPL mounts.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

#include <sstream>
#include <fstream>

#include "../utils/rts2config.h"

#include "telescope.h"
#include "connopentpl.h"

#define BLIND_SIZE            1.0
#define OPT_CHECK_POWER       OPT_LOCAL + 1
#define OPT_ROTATOR_OFFSET    OPT_LOCAL + 2
#define OPT_OPENTPL_SERVER    OPT_LOCAL + 3

namespace rts2teld
{

/**
 * Base class for OpenTPL telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OpenTPL:public Telescope
{
	private:
		HostString *openTPLServer;

		enum { OPENED, OPENING, CLOSING, CLOSED } cover_state;

		void checkErrors ();
		void checkCover ();
		void checkPower ();

		bool doCheckPower;

		void getCover ();
		void initCoverState ();

		std::string errorList;

		Rts2ValueBool *cabinetPower;
		Rts2ValueFloat *cabinetPowerState;

		Rts2ValueDouble *derotatorCurrpos;

		Rts2ValueBool *derotatorPower;

		Rts2ValueDouble *targetDist;
		Rts2ValueDouble *targetTime;

		Rts2ValueDouble *cover;

		Rts2ValueInteger *mountTrack;

		Rts2ValueAltAz *parkPos;

		// model values
		Rts2ValueString *model_dumpFile;
		std::vector <Rts2ValueDouble *> modelParams;

		int infoModel ();

		struct ln_equ_posn target;
		int irTracking;

		Rts2ValueDouble *modelQuality;
		Rts2ValueDouble *goodSep;

		double derOff;

		int startMoveReal (double ra, double dec);

	protected:
		rts2core::OpenTpl *opentplConn;

		Rts2ValueInteger *model_recordcount;

		time_t timeout;

		virtual int processOption (int in_opt);

		virtual int initIrDevice ();
		virtual int init ();
		virtual int initValues ();
		virtual int idle ();

		Rts2ValueDouble *derotatorOffset;

		int coverClose ();
		int coverOpen ();

		int domeOpen ();
		int domeClose ();

		int setTelescopeTrack (int new_track);

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

		bool getDerotatorPower ()
		{
			return derotatorPower->getValueBool ();
		}
	public:
		OpenTPL (int argc, char **argv);
		virtual ~ OpenTPL (void);
		virtual int ready ();

		virtual void getAltAz ();

		virtual int info ();
		virtual int saveModel ();
		virtual int loadModel ();
		virtual int resetMount ();

		virtual int startMove ();
		virtual int isMoving ();
		virtual int stopMove ();

		virtual int stopWorm ();
		virtual int startWorm ();

		virtual int startPark ();
		int moveCheck (bool park);
		virtual int isParking ();
		virtual int endPark ();

		virtual int changeMasterState (int new_state);
};

};

using namespace rts2teld;

#define DEBUG_EXTRA

int
OpenTPL::coverClose ()
{
	if (cover == NULL)
		return 0;
	int status = TPL_OK;
	double targetPos;
	if (cover_state == CLOSED)
		return 0;
	status = opentplConn->tpl_set ("COVER.TARGETPOS", 0, &status);
	status = opentplConn->tpl_set ("COVER.POWER", 1, &status);
	status = opentplConn->tpl_get ("COVER.TARGETPOS", targetPos, &status);
	cover_state = CLOSING;
	logStream (MESSAGE_INFO) << "closing cover, status" << status <<
		" target position " << targetPos << sendLog;
	return status;
}


int
OpenTPL::coverOpen ()
{
	if (cover == NULL)
		return 0;
	int status = TPL_OK;
	if (cover_state == OPENED)
		return 0;
	status = opentplConn->tpl_set ("COVER.TARGETPOS", 1, &status);
	status = opentplConn->tpl_set ("COVER.POWER", 1, &status);
	cover_state = OPENING;
	logStream (MESSAGE_INFO) << "opening cover, status" << status << sendLog;
	return status;
}


int
OpenTPL::setTelescopeTrack (int new_track)
{
	int status = TPL_OK;
	int old_track;
	status = opentplConn->tpl_get ("POINTING.TRACK", old_track, &status);
	if (status != TPL_OK)
		return -1;
	status = opentplConn->tpl_setww ("POINTING.TRACK", new_track | (old_track & 128), &status);
	if (status != TPL_OK)
	{
		logStream (MESSAGE_ERROR) << "Cannot setTelescopeTrack" << sendLog;
	}
	return status;
}


int
OpenTPL::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	int status = TPL_OK;
	if (old_value == cabinetPower)
	{
		status =
			opentplConn->tpl_set ("CABINET.POWER",
			((Rts2ValueBool *) new_value)->getValueBool ()? 1 : 0,
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorOffset)
	{
		status =
			opentplConn->tpl_set ("DEROTATOR[3].OFFSET", -1 * new_value->getValueDouble (),
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorCurrpos)
	{
		status =
			opentplConn->tpl_set ("DEROTATOR[3].TARGETPOS", new_value->getValueDouble (),
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorPower)
	{
		status =
			opentplConn->tpl_set ("DEROTATOR[3].POWER",
			((Rts2ValueBool *) new_value)->getValueBool ()? 1 : 0,
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == cover)
	{
		switch (new_value->getValueInteger ())
		{
			case 1:
				status = coverOpen ();
				break;
			case 0:
				status = coverClose ();
				break;
			default:
				return -2;
		}
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == mountTrack)
	{
		status = setTelescopeTrack (new_value->getValueInteger ());
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	// find model parameters..
	for (std::vector <Rts2ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		if (old_value == *iter)
		{
			// construct name..
			std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
			status = opentplConn->tpl_set (pn.c_str (), new_value->getValueDouble (), &status);
			if (status != TPL_OK)
				return -2;
			return 0;
		}
	}
	if (old_value == model_recordcount)
	{
		status = opentplConn->tpl_set ("POINTING.POINTINGPARAMS.RECORDCOUNT", new_value->getValueInteger (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == goodSep)
		return 0;

	return Telescope::setValue (old_value, new_value);
}


OpenTPL::OpenTPL (int in_argc, char **in_argv)
:Telescope (in_argc, in_argv)
{
	openTPLServer = NULL;

	opentplConn = NULL;

	doCheckPower = false;

	irTracking = 4;

	derOff = 0;
	derotatorOffset = NULL;
	derotatorCurrpos = NULL;
	derotatorPower = NULL;

	cover = NULL;

	createValue (cabinetPower, "cabinet_power", "power of cabinet", false);
	createValue (cabinetPowerState, "cabinet_power_state", "power state of cabinet", false);

	createValue (targetDist, "target_dist", "distance in degrees to target", false, RTS2_DT_DEG_DIST);
	createValue (targetTime, "target_time", "reach target time in seconds", false);

	createValue (mountTrack, "TRACK", "mount track");

	strcpy (telType, "BOOTES_IR");

	createValue (modelQuality, "model_quality", "quality of model data", false);
	createValue (goodSep, "good_sep", "targetdistance bellow this value is on target", false, RTS2_DT_DEG_DIST);
	// 2.7 arcsec
	goodSep->setValueDouble (0.00075);

	createValue (parkPos, "park_position", "mount park position", false);
	parkPos->setValueAltAz (70, 0);

	addOption (OPT_OPENTPL_SERVER, "mount-host", 1, "OpenTPL server TCP/IP address and port (separated by :)");
	addOption (OPT_CHECK_POWER, "check power", 0, "whenever to check for power state != 0 (currently depreciated)");

	addOption (OPT_ROTATOR_OFFSET, "rotator_offset", 1, "rotator offset, default to 0");
	addOption ('t', "ir_tracking", 1,
		"IR tracking (1, 2, 3 or 4 - read OpenTCI doc; default 4");

	cover_state = CLOSED;
}


OpenTPL::~OpenTPL (void)
{
	delete opentplConn;
}


int
OpenTPL::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_OPENTPL_SERVER:
			openTPLServer = new HostString (optarg);
			break;
		case OPT_CHECK_POWER:
			doCheckPower = true;
			break;
		case 't':
			irTracking = atoi (optarg);
			break;
		case OPT_ROTATOR_OFFSET:
			derOff = atof (optarg);
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}


int
OpenTPL::initIrDevice ()
{
	std::string ir_ip;
	int ir_port = 0;
	if (openTPLServer)
	{
		ir_ip = openTPLServer->getHostname ();
		ir_port = openTPLServer->getPort ();
	}
	else
	{
		Rts2Config *config = Rts2Config::instance ();
		config->loadFile (NULL);
		// try to get default from config file
		config->getString ("ir", "ip", ir_ip);
		config->getInteger ("ir", "port", ir_port);
	}

	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	opentplConn = new rts2core::OpenTpl (this, ir_ip, ir_port);

	try
	{	
		opentplConn->init ();
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
	}

	return 0;
}


int
OpenTPL::init ()
{
	int ret;
	ret = Telescope::init ();
	if (ret)
		return ret;

	ret = initIrDevice ();
	if (ret)
		return ret;

	return 0;
}


int
OpenTPL::initValues ()
{
	int status = TPL_OK;
	std::string serial;

	std::string config_mount;

	status = opentplConn->tpl_get ("CONFIG.MOUNT", config_mount, &status);
	if (status != TPL_OK)
		return -1;

	createValue (model_dumpFile, "dump_file", "model dump file", false);

	Rts2ValueDouble *modelP;

	// switch mount type
	if (config_mount == "RA-DEC")
	{
		setPointingModel (0);
		createValue (modelP, "doff", "model hour angle encoder offset", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "hoff", "model declination encoder offset", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "me", "model polar axis misalignment in elevation", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "ma", "model polar axis misalignment in azimuth", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "nphd", "model HA and DEC axis not perpendicularity", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "ch", "model east-west colimation error", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "flex", "model flex parameter", false,	RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
	}
	else if (config_mount == "AZ-ZD")
	{
/**
 * OpenTCI/Bootes IR - POINTING.POINTINGPARAMS.xx:
 * AOFF, ZOFF, AE, AN, NPAE, CA, FLEX
 * AOFF, ZOFF = az / zd offset
 * AE = azimut tilt east
 * AN = az tilt north
 * NPAE = az / zd not perpendicular
 * CA = M1 tilt with respect to optical axis
 * FLEX = sagging of tube
 */
		setPointingModel (1);
		createValue (modelP, "aoff", "model azimuth offset", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "zoff", "model zenith offset", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "ae", "azimuth equator? offset", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "an", "azimuth nadir? offset", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "npae", "not polar adjusted equator?", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "ca", "model ca parameter", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
		createValue (modelP, "flex", "model flex parameter", false, RTS2_DT_DEG_DIST);
		modelParams.push_back (modelP);
	}
	else
	{
		logStream (MESSAGE_ERROR) << "Unsupported pointing model: '" << config_mount << "'" << sendLog;
		return -1;
	}

	createValue (model_recordcount, "RECORDCOUNT", "number of observations in model", true);

	status = opentplConn->getValueDouble ("LOCAL.LATITUDE", telLatitude, &status);
	status = opentplConn->getValueDouble ("LOCAL.LONGITUDE", telLongitude, &status);
	status = opentplConn->getValueDouble ("LOCAL.HEIGHT", telAltitude, &status);

	if (status != TPL_OK)
	{
		return -1;
	}

	opentplConn->tpl_get ("CABINET.SETUP.HW_ID", serial, &status);
	addConstValue ("IR_HWID", "serial number", serial.c_str ());

	if (opentplConn->haveModule ("COVER"))
	{
		createValue (cover, "cover", "cover state (1 = opened)", false);
		initCoverState ();
	}

	// nasmith derotator
	if (opentplConn->haveModule ("DEROTATOR[3]"))
	{
		createValue (derotatorOffset, "DER_OFF", "derotator offset", true, RTS2_DT_ROTANG, 0, true);
		createValue (derotatorCurrpos, "DER_CUR", "derotator current position", true, RTS2_DT_DEGREES);
		createValue (derotatorPower, "derotatorPower", "derotator power setting", false);
	}

	// infoModel
	status = infoModel ();
	if (status)
		return status;

	if (derotatorOffset)
		derotatorOffset->setValueDouble (derOff);

	return Telescope::initValues ();
}


void
OpenTPL::checkErrors ()
{
	int status = TPL_OK;
	std::string list;

	status = opentplConn->tpl_get ("CABINET.STATUS.LIST", list, &status);
	if (status == 0 && list.length () > 2 && list != errorList)
	{
		// print errors to log & ends..
		logStream (MESSAGE_ERROR) << "IR checkErrors Telescope errors " <<
			list << sendLog;
		errorList = list;
		opentplConn->tpl_set ("CABINET.STATUS.CLEAR", 1, &status);
		if (list == "\"ERR_Soft_Limit_max:4:ZD\"")
		{
			double zd;
			status = opentplConn->tpl_get ("ZD.CURRPOS", zd, &status);
			if (!status)
			{
				if (zd < -80)
					zd = -85;
				if (zd > 80)
					zd = 85;
				status = setTelescopeTrack (0);
				logStream (MESSAGE_DEBUG) <<
					"IR checkErrors set pointing status " << status << sendLog;
				sleep (1);
				status = TPL_OK;
				status = opentplConn->tpl_set ("ZD.TARGETPOS", zd, &status);
				logStream (MESSAGE_ERROR) <<
					"IR checkErrors zd soft limit reset " <<
					zd << " (" << status << ")" << sendLog;
			}
		}
	}
}


void
OpenTPL::checkCover ()
{
	int status = TPL_OK;
	switch (cover_state)
	{
		case OPENING:
			getCover ();
			if (cover->getValueDouble () == 1.0)
			{
				opentplConn->tpl_set ("COVER.POWER", 0, &status);
			#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "IR checkCover opened " << status <<
					sendLog;
			#endif
				cover_state = OPENED;
				break;
			}
			setTimeout (USEC_SEC);
			break;
		case CLOSING:
			getCover ();
			if (cover->getValueDouble () == 0.0)
			{
				opentplConn->tpl_set ("COVER.POWER", 0, &status);
			#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "IR checkCover closed " << status <<
					sendLog;
			#endif
				cover_state = CLOSED;
				break;
			}
			setTimeout (USEC_SEC);
			break;
		case OPENED:
		case CLOSED:
			break;
	}
}


void
OpenTPL::checkPower ()
{
	int status = TPL_OK;
	double power_state;
	double referenced;
	status = opentplConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "IR checkPower tpl_ret " << status <<
			sendLog;
		return;
	}

	if (!doCheckPower)
		return;

	if (power_state == 0)
	{
		status = opentplConn->tpl_set ("CABINET.POWER", 1, &status);
		status = opentplConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "IR checkPower set power ot 1 ret " <<
				status << sendLog;
			return;
		}
		while (power_state == 0.5)
		{
			logStream (MESSAGE_DEBUG) << "IR checkPower waiting for power up" <<
				sendLog;
			sleep (5);
			status = opentplConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "IR checkPower power_state ret " <<
					status << sendLog;
				return;
			}
		}
	}
	while (true)
	{
		status = opentplConn->tpl_get ("CABINET.REFERENCED", referenced, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "IR checkPower get referenced " <<
				status << sendLog;
			return;
		}
		if (referenced == 1)
			break;
		logStream (MESSAGE_DEBUG) << "IR checkPower referenced " << referenced
			<< sendLog;
		if (referenced == 0)
		{
			status = opentplConn->tpl_set ("CABINET.REINIT", 1, &status);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "IR checkPower reinit " <<
					status << sendLog;
				return;
			}
		}
		sleep (1);
	}
	if (cover)
	{
		// force close of cover..
		initCoverState ();
		switch (getMasterState ())
		{
			case SERVERD_DUSK:
			case SERVERD_NIGHT:
			case SERVERD_DAWN:
				coverOpen ();
				break;
			default:
				coverClose ();
				break;
		}
	}
}


void
OpenTPL::getCover ()
{
	double cor_tmp;
	int status = TPL_OK;
	opentplConn->tpl_get ("COVER.REALPOS", cor_tmp, &status);
	if (status)
		return;
	cover->setValueDouble (cor_tmp);
}


void
OpenTPL::initCoverState ()
{
	getCover ();
	if (cover->getValueDouble () == 0)
		cover_state = CLOSED;
	else if (cover->getValueDouble () == 1)
		cover_state = OPENED;
	else
		cover_state = CLOSING;
}


int
OpenTPL::infoModel ()
{
	int status = TPL_OK;

	std::string dumpfile;
	int recordcount;

	status = opentplConn->tpl_get ("POINTING.POINTINGPARAMS.DUMPFILE", dumpfile, &status);

	for (std::vector <Rts2ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		double pv;
		std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
		status = opentplConn->tpl_get (pn.c_str (), pv, &status);
		if (status != TPL_OK)
			return -1;
		(*iter)->setValueDouble (pv);
	}

	status = opentplConn->tpl_get ("POINTING.POINTINGPARAMS.RECORDCOUNT", recordcount, &status);

	if (status != TPL_OK)
		return -1;

	model_dumpFile->setValueString (dumpfile);
	model_recordcount->setValueInteger (recordcount);
	return 0;
}


int
OpenTPL::startMoveReal (double ra, double dec)
{
	int status = TPL_OK;
	status = opentplConn->tpl_set ("POINTING.TARGET.RA", ra / 15.0, &status);
	status = opentplConn->tpl_set ("POINTING.TARGET.DEC", dec, &status);
	if (derotatorOffset && !getDerotatorPower ())
	{
		status = opentplConn->tpl_set ("DEROTATOR[3].POWER", 1, &status);
		status = opentplConn->tpl_set ("CABINET.POWER", 1, &status);
	}

	double offset;
	int track;

	// apply corrections
	switch (getPointingModel ())
	{
		case 0:
			offset = getCorrRa ();
			status = opentplConn->tpl_set ("HA.OFFSET", offset, &status);
			status = opentplConn->tpl_get ("POINTING.TRACK", track, &status);
			offset = getCorrDec ();
			if (track == 3)
				offset *= -1.0;
			status = opentplConn->tpl_set ("DEC.OFFSET", offset, &status);
			break;
		case 1:
			offset = getCorrZd ();
			status = opentplConn->tpl_set ("ZD.OFFSET", offset, &status);
			offset = getCorrAz ();
			status = opentplConn->tpl_set ("AZ.OFFSET", offset, &status);
			break;

	}

	if (isModelOn () && (getCorrRa () != 0 || getCorrDec () != 0))
	{
		status = opentplConn->tpl_set ("POINTING.POINTINGPARAMS.SAMPLE", 1, &status);
		status = opentplConn->getValueDouble ("POINTING.POINTINGPARAMS.CALCULATE", modelQuality, &status);
		status = opentplConn->getValueInteger ("POINTING.POINTINGPARAMS.RECORDCOUNT", model_recordcount, &status);
		logStream (MESSAGE_DEBUG) << "modeling quality parameter: " << modelQuality->getValueDouble ()
			<< " status "<< status
			<< " recordcount " << model_recordcount->getValueInteger () << sendLog;
		sendValueAll (modelQuality);
		sendValueAll (model_recordcount);
	}

	if (derotatorOffset)
	{
		status = opentplConn->tpl_set ("DEROTATOR[3].OFFSET", -1 * derotatorOffset->getValueDouble () , &status);

		if (status != TPL_OK)
			return status;
	}

	status = setTelescopeTrack (irTracking);
	return status;
}


int
OpenTPL::idle ()
{
	// check for power..
	checkPower ();
	// check for errors..
	checkErrors ();
	if (cover)
		checkCover ();
	return Telescope::idle ();
}


int
OpenTPL::ready ()
{
	return 0;
}


void
OpenTPL::getAltAz ()
{
	int status = TPL_OK;
	double zd, az;

	if (getPointingModel () == 0)
	{
		Telescope::getAltAz ();
	}
	else
	{
		status = opentplConn->tpl_get ("ZD.REALPOS", zd, &status);
		status = opentplConn->tpl_get ("AZ.REALPOS", az, &status);

		telAltAz->setValueAltAz (90 - fabs (zd), ln_range_degrees (az + 180));
	}
}


int
OpenTPL::info ()
{
	double zd, az;
	#ifdef DEBUG_EXTRA
	double zd_speed, az_speed;
	#endif						 // DEBUG_EXTRA
	double t_telRa, t_telDec;
	int track = 0;
	int derPower = 0;
	int status = TPL_OK;

	int cab_power;
	double cab_power_state;

	status = opentplConn->tpl_get ("CABINET.POWER", cab_power, &status);
	status = opentplConn->tpl_get ("CABINET.POWER_STATE", cab_power_state, &status);
	if (status != TPL_OK)
		return -1;
	cabinetPower->setValueBool (cab_power == 1);
	cabinetPowerState->setValueFloat (cab_power_state);

	status = opentplConn->tpl_get ("POINTING.TRACK", track, &status);
	if (status != TPL_OK)
		return -1;

	if (!track)
	{
		// telRA, telDec are invalid: compute them from ZD/AZ
		struct ln_hrz_posn hrz;
		struct ln_lnlat_posn observer;
		struct ln_equ_posn curr;
		switch (getPointingModel ())
		{
			case 0:
				status = opentplConn->tpl_get ("POINTING.CURRPOS.RA", t_telRa, &status);
				t_telRa *= 15.0;
				status = opentplConn->tpl_get ("POINTING.CURRPOS.DEC", t_telDec, &status);
				if (status != TPL_OK)
				{
					return -1;
				}
				telFlip->setValueInteger (0);
				break;
			case 1:
				hrz.az = az;
				hrz.alt = 90 - fabs (zd);
				observer.lng = telLongitude->getValueDouble ();
				observer.lat = telLatitude->getValueDouble ();
				ln_get_equ_from_hrz (&hrz, &observer, ln_get_julian_from_sys (), &curr);
				t_telRa = curr.ra;
				t_telDec = curr.dec;
				telFlip->setValueInteger (0);
				break;
		}
	}
	else
	{
		status = opentplConn->tpl_get ("POINTING.CURRENT.RA", t_telRa, &status);
		t_telRa *= 15.0;
		status = opentplConn->tpl_get ("POINTING.CURRENT.DEC", t_telDec, &status);

		telFlip->setValueInteger (0);
	}

	if (status != TPL_OK)
		return -1;

	setTelRaDec (t_telRa, t_telDec);

	#ifdef DEBUG_EXTRA
	switch (getPointingModel ())
	{
		case 0:
			status = opentplConn->tpl_get ("HA.CURRSPEED", zd_speed, &status);
			status = opentplConn->tpl_get ("DEC.CURRSPEED", az_speed, &status);

			logStream (MESSAGE_DEBUG) << "IR info ra " << getTelRa ()
				<< " dec " << getTelDec ()
				<< " ha_speed " << az_speed
				<< " dec_speed " << zd_speed
				<< " track " << track
				<< sendLog;
			break;

		case 1:
			status = opentplConn->tpl_get ("ZD.CURRSPEED", zd_speed, &status);
			status = opentplConn->tpl_get ("AZ.CURRSPEED", az_speed, &status);

			logStream (MESSAGE_DEBUG) << "IR info ra " << getTelRa ()
				<< " dec " << getTelDec ()
				<< " az_speed " << az_speed
				<< " zd_speed " << zd_speed
				<< " track " << track
				<< sendLog;
			break;
	}
	#endif						 // DEBUG_EXTRA

	if (status)
		return -1;

	if (derotatorOffset)
	{
		opentplConn->getValueDouble ("DEROTATOR[3].OFFSET", derotatorOffset, &status);
		opentplConn->getValueDouble ("DEROTATOR[3].CURRPOS", derotatorCurrpos, &status);
		opentplConn->tpl_get ("DEROTATOR[3].POWER", derPower, &status);
		if (status == TPL_OK)
		{
			derotatorPower->setValueBool (derPower == 1);
		}
	}

	if (cover)
		getCover ();

	mountTrack->setValueInteger (track);

	status = TPL_OK;
	double point_dist;
	double point_time;
	status = opentplConn->tpl_get ("POINTING.TARGETDISTANCE", point_dist, &status);
	status = opentplConn->tpl_get ("POINTING.SLEWINGTIME", point_time, &status);
	if (status == TPL_OK)
	{
		targetDist->setValueDouble (point_dist);
		targetTime->setValueDouble (point_time);
	}

	return Telescope::info ();
}


int
OpenTPL::saveModel ()
{
	std::ofstream of;
	of.open ("/etc/rts2/ir.model", std::ios_base::out | std::ios_base::trunc);
	of.precision (20);
	for (std::vector <Rts2ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		double pv;
		int status = TPL_OK;
		std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
		status = opentplConn->tpl_get (pn.c_str (), pv, &status);
		if (status != TPL_OK)
			return -1;
		of << pn << " " << pv << std::endl;
	}
	of.close ();
	if (of.fail ())
	{
		logStream (MESSAGE_ERROR) << "model saving failed" << sendLog;
		return -1;
	}
	return 0;
}


int
OpenTPL::loadModel ()
{
	std::ifstream ifs;
	int status = TPL_OK;
	try
	{
		std::string pn;
		double pv;
		ifs.open ("/etc/rts2/ir.model");
		while (!ifs.eof ())
		{
			ifs >> pn >> pv;
			std::string pnc = std::string ("POINTING.POINTINGPARAMS.") + pn;
			status = opentplConn->tpl_set (pnc.c_str (), pv, &status);
			if (status != TPL_OK)
				return -1;
		}
	}
	catch (std::exception & e)
	{
		logStream (MESSAGE_DEBUG) << "IR loadModel error" << sendLog;
		return -1;
	}
	return infoModel ();
}


int
OpenTPL::resetMount ()
{
	int status = TPL_OK;
	int power = 0;
	double power_state;
	status = opentplConn->tpl_set ("CABINET.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "IR resetMount powering off: " << status <<
			sendLog;
		return -1;
	}
	while (true)
	{
		logStream (MESSAGE_DEBUG) << "IR resetMount waiting for power down" <<
			sendLog;
		sleep (5);
		status = opentplConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "IR resetMount power_state ret: " <<
				status << sendLog;
			return -1;
		}
		if (power_state == 0 || power_state == -1)
		{
			logStream (MESSAGE_DEBUG) << "IR resetMount final power_state: " <<
				power_state << sendLog;
			break;
		}
	}
	return Telescope::resetMount ();
}


int
OpenTPL::startMove ()
{
	int status = 0;
	double sep;

	getTarget (&target);

	switch (getPointingModel ())
	{
		case 0:
			if (target.dec < -89.85)
				target.dec = -89.80;
			break;
		case 1:
			// move to zenit - move to different dec instead
			if (fabs (target.dec - telLatitude->getValueDouble ()) <= BLIND_SIZE)
			{
				if (fabs (target.ra / 15.0 - getLocSidTime ()) <= BLIND_SIZE / 15.0)
				{
					target.dec = telLatitude->getValueDouble () - BLIND_SIZE;
				}
			}

			double az_off = 0;
			double alt_off = 0;
			status = opentplConn->tpl_set ("AZ.OFFSET", az_off, &status);
			status = opentplConn->tpl_set ("ZD.OFFSET", alt_off, &status);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "IR startMove cannot zero offset" << sendLog;
				return -1;
			}
			break;
	}

	status = startMoveReal (target.ra, target.dec);
	if (status)
		return -1;

	// wait till we get it processed
	status = opentplConn->tpl_get ("POINTING.TARGETDISTANCE", sep, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "cannot get target separation" << sendLog;
		return -1;
	}
	if (sep > 2)
	  	usleep (USEC_SEC / 5);
	else if (sep > 2 / 60.0)
		usleep (USEC_SEC / 100);
	else
		usleep (USEC_SEC / 1000);

	time (&timeout);
	timeout += 30;
	return 0;
}


int
OpenTPL::isMoving ()
{
	return moveCheck (false);
}


int
OpenTPL::stopMove ()
{
	int status = 0;
	double zd;
	info ();
	// ZD check..
	status = opentplConn->tpl_get ("ZD.CURRPOS", zd, &status);
	if (status)
	{
		logStream (MESSAGE_DEBUG) << "IR stopMove cannot get ZD! (" << status <<
			")" << sendLog;
		return -1;
	}
	if (fabs (zd) < 1)
	{
		logStream (MESSAGE_DEBUG) << "IR stopMove suspicious ZD.. " << zd <<
			sendLog;
		status = setTelescopeTrack (0);
		if (status)
		{
			logStream (MESSAGE_DEBUG) << "IR stopMove cannot set track: " <<
				status << sendLog;
			return -1;
		}
		return 0;
	}
	return 0;
}


int
OpenTPL::startPark ()
{
	int status = TPL_OK;
	// Park to south+zenith
	status = setTelescopeTrack (0);
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "IR startPark tracking status " << status <<
		sendLog;
	#endif
	sleep (1);
	status = TPL_OK;
	double tra;
	double tdec;
	switch (getPointingModel ())
	{
		case 0:
			status = opentplConn->tpl_get ("POINTING.CURRENT.SIDEREAL_TIME", tra, &status);
			if (status != TPL_OK)
				return -1;
			// decide dec based on latitude
			tdec = 90 - fabs (telLatitude->getValueDouble ());
			if (telLatitude->getValueDouble () < 0)
				tdec *= -1.0;

			status = opentplConn->tpl_set ("POINTING.TARGET.RA", tra, &status);
			status = opentplConn->tpl_set ("POINTING.TARGET.DEC", tdec, &status);
			setTelescopeTrack (irTracking);
			break;
		case 1:
			status = opentplConn->tpl_set ("AZ.TARGETPOS", 0, &status);
			status = opentplConn->tpl_set ("ZD.TARGETPOS", 0, &status);
			status = opentplConn->tpl_set ("DEROTATOR[3].POWER", 0, &status);
			break;
	}
	if (status)
	{
		logStream (MESSAGE_ERROR) << "IR startPark status " <<
			status << sendLog;
		return -1;
	}
	time (&timeout);
	timeout += 300;
	return 0;
}


int
OpenTPL::moveCheck (bool park)
{
	int status = TPL_OK;
	int track;
	double poin_dist;
	time_t now;
	// if parking, check is different for EQU telescope
	if (park)
	{
		struct ln_equ_posn tPos;
		struct ln_equ_posn cPos;

		switch (getPointingModel ())
		{
			case 0:
				status = opentplConn->tpl_get ("HA.TARGETPOS", tPos.ra, &status);
				status = opentplConn->tpl_get ("DEC.TARGETPOS", tPos.dec, &status);
				status = opentplConn->tpl_get ("HA.CURRPOS", cPos.ra, &status);
				status = opentplConn->tpl_get ("DEC.CURRPOS", cPos.dec, &status);
				break;
			case 1:
				status = opentplConn->tpl_get ("ZD.TARGETPOS", tPos.ra, &status);
				status = opentplConn->tpl_get ("AZ.TARGETPOS", tPos.dec, &status);
				status = opentplConn->tpl_get ("ZD.CURRPOS", cPos.ra, &status);
				status = opentplConn->tpl_get ("AZ.CURRPOS", cPos.dec, &status);
				break;
		}
		poin_dist = ln_get_angular_separation (&cPos, &tPos);
	}
	else
	{
		status = opentplConn->tpl_get ("POINTING.TARGETDISTANCE", poin_dist, &status);
	}
	if (status != TPL_OK)
		return -1;
	time (&now);
	// get track..
	status = opentplConn->tpl_get ("POINTING.TRACK", track, &status);
	if (track == 0 && !park)
	{
		logStream (MESSAGE_WARNING) << "Tracking sudently stopped, reenable tracking (track=" << track << " park = " << park << ")" << sendLog;
		setTelescopeTrack (irTracking);
		sleep (1);
		return USEC_SEC / 100;
	}
	if (fabs (poin_dist) <= goodSep->getValueDouble ()) 
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "IR isMoving target distance " << poin_dist << sendLog;
		#endif
		return -2;
	}
	// finish due to timeout
	if (timeout < now)
	{
		logStream (MESSAGE_ERROR) << "checkMoves target distance in timeout " << poin_dist << " (" << status << ")" << sendLog;
		return -1;
	}
	return USEC_SEC / 100;
}


int
OpenTPL::isParking ()
{
	return moveCheck (true);
}


int
OpenTPL::endPark ()
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "IR endPark" << sendLog;
	#endif
	return 0;
}


int
OpenTPL::startWorm ()
{
	int status = TPL_OK;
	status = setTelescopeTrack (irTracking);
	if (status != TPL_OK)
		return -1;
	return 0;
}


int
OpenTPL::stopWorm ()
{
	int status = TPL_OK;
	status = setTelescopeTrack (0);
	if (status)
		return -1;
	return 0;
}


int
OpenTPL::changeMasterState (int new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			coverOpen ();
			break;
		default:
			coverClose ();
			break;
	}
	return Telescope::changeMasterState (new_state);
}


int
main (int argc, char **argv)
{
	OpenTPL device = OpenTPL (argc, argv);
	return device.run ();
}
