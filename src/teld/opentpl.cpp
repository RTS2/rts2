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

#include "configuration.h"

#include "teld.h"

#include "connection/opentpl.h"

#define BLIND_SIZE            1.0
#define OPT_ROTATOR_OFFSET    OPT_LOCAL + 2
#define OPT_POWEOFF_STANDBY   OPT_LOCAL + 5
#define OPT_GOOD_SEP          OPT_LOCAL + 6

namespace rts2teld
{

/**
 * Base class for OpenTPL telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OpenTPL:public Telescope
{
	public:
		OpenTPL (int argc, char **argv);
		virtual ~ OpenTPL (void);

		virtual int info ();
		virtual int scriptEnds ();

		virtual int saveModel ();
		virtual int loadModel ();
		virtual int resetMount ();

		virtual int startResync ();
		virtual int isMoving ();
		virtual int stopMove ();

		virtual int setTracking (int track, bool addTrackingTimer = false, bool send = true, const char *stopMsg = "tracking stopped");
		int stopWorm ();
		int startWorm ();

		virtual int startPark ();
		int moveCheck (bool park);
		virtual int isParking ();
		virtual int endPark ();

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

	protected:
		rts2core::OpenTpl *opentplConn;

		rts2core::ValueInteger *model_recordcount;

		time_t timeout;

		virtual int processOption (int in_opt);

		virtual int initHardware ();
		virtual int initValues ();
		virtual int idle ();

		rts2core::ValueDouble *derotatorOffset;

		void powerOn ();
		void powerOff ();

		int coverClose ();
		int coverOpen ();

		int domeOpen ();
		int domeClose ();

		int setTelescopeTrack (int new_track);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		bool getDerotatorPower () { return derotatorPower->getValueBool (); }
		virtual void getTelAltAz (struct ln_hrz_posn *hrz);

		virtual void setDiffTrack (double dra, double ddec);

	private:
		HostString *openTPLServer;

		enum { OPENED, OPENING, CLOSING, CLOSED } cover_state;

		void checkErrors ();
		void checkCover ();
		void checkPower ();

		void getCover ();
		void initCoverState ();

		std::string errorList;

		rts2core::ValueBool *debugConn;
		rts2core::ValueBool *noRepower;

		rts2core::ValueBool *cabinetPower;
		rts2core::ValueFloat *cabinetPowerState;

		rts2core::ValueDouble *derotatorCurrpos;

		rts2core::ValueBool *derotatorPower;

		rts2core::ValueDouble *targetDist;
		rts2core::ValueDouble *targetDist1;
		rts2core::ValueDouble *targetDist2;
		rts2core::ValueDouble *targetTime;

		rts2core::ValueDouble *cover;

		rts2core::ValueInteger *mountTrack;

		// model values
		rts2core::ValueString *model_dumpFile;
		std::vector <rts2core::ValueDouble *> modelParams;

		int infoModel ();

		int irTracking;

		rts2core::ValueDouble *modelQuality;
		rts2core::ValueDouble *goodSep;

		// modeling offsets
		rts2core::ValueRaDec *om_offsets;

		rts2core::ValueBool *standbyPoweroff;

		double derOff;

		int startMoveReal (double ra, double dec);
};

}

using namespace rts2teld;

#define DEBUG_EXTRA

void OpenTPL::powerOn ()
{
	checkPower ();
}

void OpenTPL::powerOff ()
{
	int status = TPL_OK;
	opentplConn->setDebug ();
	opentplConn->set ("CABINET.POWER", 0, &status);
	// check that it powered off..
	double power_state;
	logStream (MESSAGE_DEBUG) << "powerOff: waiting for power state off";
	for (int i = 0; i < 60; i++)
	{
		status = opentplConn->get ("CABINET.POWER_STATE", power_state, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "powerOff: power_state " << status << sendLog;
			opentplConn->setDebug (debugConn->getValueBool ());
			return;
		}
		sleep (3);
		if (power_state == 0)
		{
			logStream (MESSAGE_DEBUG) << "powerOff: power_state is 0" << sendLog;
			cabinetPower->setValueBool (false);
			cabinetPowerState->setValueFloat (power_state);
			sendValueAll (cabinetPower);
			sendValueAll (cabinetPowerState);
			opentplConn->setDebug (debugConn->getValueBool ());
			return;
		}
	}
	opentplConn->setDebug (debugConn->getValueBool ());
	cabinetPowerState->setValueFloat (power_state);
	sendValueAll (cabinetPowerState);
	logStream (MESSAGE_ERROR) << "powerOff: timeouted waiting for sucessfull poweroff" << sendLog;
}

int OpenTPL::coverClose ()
{
	if (cover == NULL)
		return 0;
	int status = TPL_OK;
	double targetPos;
	if (cover_state == CLOSED)
		return 0;
	status = opentplConn->set ("COVER.TARGETPOS", 0, &status);
	status = opentplConn->set ("COVER.POWER", 1, &status);
	status = opentplConn->get ("COVER.TARGETPOS", targetPos, &status);
	cover_state = CLOSING;
	logStream (MESSAGE_INFO) << "closing cover, status" << status <<
		" target position " << targetPos << sendLog;
	return status;
}

int OpenTPL::coverOpen ()
{
	if (cover == NULL)
		return 0;
	int status = TPL_OK;
	if (cover_state == OPENED)
		return 0;
	status = opentplConn->set ("COVER.TARGETPOS", 1, &status);
	status = opentplConn->set ("COVER.POWER", 1, &status);
	cover_state = OPENING;
	logStream (MESSAGE_INFO) << "opening cover, status" << status << sendLog;
	return status;
}

int OpenTPL::setTelescopeTrack (int new_track)
{
	int status = TPL_OK;
	int old_track;
	status = opentplConn->get ("POINTING.TRACK", old_track, &status);
	if (status != TPL_OK)
		return -1;
	status = opentplConn->setww ("POINTING.TRACK", new_track | (old_track & 128), &status);
	if (status != TPL_OK)
	{
		logStream (MESSAGE_ERROR) << "Cannot setTelescopeTrack" << sendLog;
	}
	return status;
}

int OpenTPL::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	int status = TPL_OK;
	if (old_value == debugConn)
	{
		opentplConn->setDebug (((rts2core::ValueBool *) new_value)->getValueBool ());
		return 0;
	}
	if (old_value == cabinetPower)
	{
	  	opentplConn->setDebug ();
		status = opentplConn->set ("CABINET.POWER", ((rts2core::ValueBool *) new_value)->getValueBool ()? 1 : 0, &status);
	  	opentplConn->setDebug (debugConn->getValueBool ());
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorOffset)
	{
		status = opentplConn->set ("DEROTATOR[3].OFFSET", -1 * new_value->getValueDouble (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorCurrpos)
	{
		status = opentplConn->set ("DEROTATOR[3].TARGETPOS", new_value->getValueDouble (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorPower)
	{
		status = opentplConn->set ("DEROTATOR[3].POWER", ((rts2core::ValueBool *) new_value)->getValueBool ()? 1 : 0, &status);
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
	for (std::vector <rts2core::ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		if (old_value == *iter)
		{
			// construct name..
			std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
			status = opentplConn->set (pn.c_str (), new_value->getValueDouble (), &status);
			if (status != TPL_OK)
				return -2;
			return 0;
		}
	}
	if (old_value == model_recordcount)
	{
		status = opentplConn->set ("POINTING.POINTINGPARAMS.RECORDCOUNT", new_value->getValueInteger (), &status);
		if (status != TPL_OK)
			return -2;
		status = infoModel ();
		if (status)
			return -2;
		return 0;
	}

	return Telescope::setValue (old_value, new_value);
}

OpenTPL::OpenTPL (int in_argc, char **in_argv):Telescope (in_argc, in_argv, true, true)
{
	openTPLServer = NULL;

	opentplConn = NULL;

	irTracking = -1;

	derOff = 0;
	derotatorOffset = NULL;
	derotatorCurrpos = NULL;
	derotatorPower = NULL;

	cover = NULL;

	createValue (debugConn, "debug_connection", "debug TCP/IP connection to NTM", false, RTS2_VALUE_WRITABLE);
	debugConn->setValueBool (false);

	createValue (noRepower, "no_repower", "when set to true, do not attempt to repower once power == 0 is detected", false, RTS2_VALUE_WRITABLE);
	noRepower->setValueBool (false);

	createValue (cabinetPower, "cabinet_power", "power of cabinet", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	createValue (cabinetPowerState, "cabinet_power_state", "power state of cabinet", false);

	createValue (targetDist, "target_dist", "distance in degrees to target", false, RTS2_DT_DEG_DIST);
	createValue (targetTime, "target_time", "reach target time in seconds", false);

	createValue (mountTrack, "TRACK", "mount track", true, RTS2_VALUE_WRITABLE);

	createValue (modelQuality, "model_quality", "quality of model data", false);
	createValue (goodSep, "good_sep", "targetdistance bellow this value is on target", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
	// 2.7 arcsec
	goodSep->setValueDouble (0.00075);

	createValue (standbyPoweroff, "standby_poweroff", "power off at standby (power on at ready night, dusk or dawn)", false, RTS2_VALUE_WRITABLE);
	standbyPoweroff->setValueBool (false);

	addOption (OPT_OPENTPL_SERVER, "opentpl", 1, "OpenTPL server TCP/IP address and port (separated by :)");

	addOption (OPT_ROTATOR_OFFSET, "rotator_offset", 1, "rotator offset, default to 0");
	addOption ('t', NULL, 1, "tracking (1, 2, 3 or 4 - read OpenTCI doc; default 4");

	addParkPosOption ();
	createParkPos (70, 0, 0);

	addOption (OPT_POWEOFF_STANDBY, "standby-poweroff", 0, "poweroff at standby");
	addOption (OPT_GOOD_SEP, "good-sep", 1, "minimal good separation (in degrees)");

	cover_state = CLOSED;
}

OpenTPL::~OpenTPL (void)
{
	delete opentplConn;
}

int OpenTPL::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_OPENTPL_SERVER:
			openTPLServer = new HostString (optarg, "65432");
			break;
		case OPT_ROTATOR_OFFSET:
			derOff = atof (optarg);
			break;
		case 't':
			irTracking = atoi (optarg);
			break;
		case OPT_POWEOFF_STANDBY:
			standbyPoweroff->setValueBool (true);
			break;
		case OPT_GOOD_SEP:
			goodSep->setValueCharArr (optarg);
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int OpenTPL::initHardware ()
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
		rts2core::Configuration *config = rts2core::Configuration::instance ();
		config->loadFile (NULL);
		// try to get default from config file
		config->getString ("opentpl", "ip", ir_ip);
		config->getInteger ("opentpl", "port", ir_port);
	}

	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	try
	{	
		opentplConn = new rts2core::OpenTpl (this, ir_ip, ir_port);
		opentplConn->init ();
	}
	catch (rts2core::ConnError er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}

	return 0;
}

int OpenTPL::initValues ()
{
	int status = TPL_OK;
	std::string serial;

	std::string config_mount;

	status = opentplConn->get ("CONFIG.MOUNT", config_mount, &status);
	if (status != TPL_OK)
		return -1;

	createValue (model_dumpFile, "dump_file", "model dump file", false);

	rts2core::ValueDouble *modelP;

	// switch mount type
	if (config_mount == "RA-DEC")
	{
		setPointingModel (POINTING_RADEC);

		createValue (targetDist1, "target_dist_ha", "[deg] target distance in HA axis", false, RTS2_DT_DEGREES);
		createValue (targetDist2, "target_dist_dec", "[deg] target distance in DEC axis", false, RTS2_DT_DEGREES);

		createValue (om_offsets, "MO", "[deg] target pointing correction", true, RTS2_DT_DEGREES);

		createValue (modelP, "doff", "[deg] model hour angle encoder offset", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "hoff", "[deg] model declination encoder offset", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "me", "[deg] model polar axis misalignment in elevation", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "ma", "[deg] model polar axis misalignment in azimuth", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "nphd", "[deg] model HA and DEC axis not perpendicularity", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "ch", "[deg] model east-west colimation error", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "flex", "[deg] model flex parameter", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		if (irTracking < 0)
			irTracking = 2;
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
		setPointingModel (POINTING_ALTAZ);

		createValue (targetDist1, "target_dist_az", "[deg] target distance in azimuth axis", false, RTS2_DT_DEGREES);
		createValue (targetDist2, "target_dist_alt", "[deg] target distance in altitude axis", false, RTS2_DT_DEGREES);

		createValue (om_offsets, "MO", "[deg] target pointing correction", true, RTS2_DT_DEGREES);

		createValue (modelP, "aoff", "[deg] model azimuth offset", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "zoff", "[deg] model zenith offset", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "ae", "[deg] azimuth equator? offset", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "an", "[deg] azimuth nadir? offset", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "npae", "[deg] not polar adjusted equator?", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "ca", "[deg] model ca parameter", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		createValue (modelP, "flex", "[deg] model flex parameter", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
		modelParams.push_back (modelP);
		if (irTracking < 0)
			irTracking = 4;
	}
	else
	{
		logStream (MESSAGE_ERROR) << "Unsupported pointing model: '" << config_mount << "'" << sendLog;
		return -1;
	}

	createValue (model_recordcount, "RECORDCOUNT", "number of observations in model", true, RTS2_VALUE_WRITABLE);

	status = opentplConn->getValueDouble ("LOCAL.LATITUDE", telLatitude, &status);
	status = opentplConn->getValueDouble ("LOCAL.LONGITUDE", telLongitude, &status);
	status = opentplConn->getValueDouble ("LOCAL.HEIGHT", telAltitude, &status);

	if (status != TPL_OK)
	{
		return -1;
	}

	opentplConn->get ("CABINET.SETUP.HW_ID", serial, &status);
	addConstValue ("HWID", "serial number", serial.c_str ());

	if (opentplConn->haveModule ("COVER"))
	{
		createValue (cover, "cover", "cover state (1 = opened)", false, RTS2_VALUE_WRITABLE);
		initCoverState ();
	}

	// nasmith derotator
	if (opentplConn->haveModule ("DEROTATOR[3]"))
	{
		createValue (derotatorOffset, "DER_OFF", "derotator offset", true, RTS2_DT_WCS_ROTANG | RTS2_VALUE_WRITABLE, 0);
		createValue (derotatorCurrpos, "DER_CUR", "derotator current position", true, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
		createValue (derotatorPower, "derotatorPower", "derotator power setting", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	}

	// infoModel
	status = infoModel ();
	if (status)
		return status;

	if (derotatorOffset)
		derotatorOffset->setValueDouble (derOff);

	return Telescope::initValues ();
}

void OpenTPL::checkErrors ()
{
	if (!opentplConn->isConnState (CONN_CONNECTED))
		return;
	int status = TPL_OK;
	std::string list;

	status = opentplConn->get ("CABINET.STATUS.LIST", list, &status);
	if (status == 0 && list.length () > 2 && list != errorList)
	{
		// print errors to log & ends..
		logStream (MESSAGE_ERROR) << "checkErrors Telescope errors " <<
			list << sendLog;
		errorList = list;
		opentplConn->set ("CABINET.STATUS.CLEAR", 1, &status);
		if (list == "\"ERR_Soft_Limit_max:4:ZD\"")
		{
			double zd;
			status = opentplConn->get ("ZD.CURRPOS", zd, &status);
			if (!status)
			{
				if (zd < -80)
					zd = -85;
				if (zd > 80)
					zd = 85;
				status = setTelescopeTrack (0);
				logStream (MESSAGE_DEBUG) <<
					"checkErrors set pointing status " << status << sendLog;
				sleep (1);
				status = TPL_OK;
				status = opentplConn->set ("ZD.TARGETPOS", zd, &status);
				logStream (MESSAGE_ERROR) <<
					"checkErrors zd soft limit reset " <<
					zd << " (" << status << ")" << sendLog;
			}
		}
		if (status == TPL_OK)
			errorList = "";
	}
}

void OpenTPL::checkCover ()
{
	int status = TPL_OK;
	switch (cover_state)
	{
		case OPENING:
			getCover ();
			if (cover->getValueDouble () == 1.0)
			{
				opentplConn->set ("COVER.POWER", 0, &status);
			#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "checkCover opened " << status << sendLog;
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
				opentplConn->set ("COVER.POWER", 0, &status);
			#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "checkCover closed " << status << sendLog;
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

void OpenTPL::checkPower ()
{
	int status = TPL_OK;
	double power_state;
	double referenced;
	opentplConn->setDebug ();
	status = opentplConn->get ("CABINET.POWER_STATE", power_state, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "checkPower tpl_ret " << status << sendLog;
		opentplConn->setDebug (debugConn->getValueBool ());
		return;
	}

	if (power_state == 1 || noRepower->getValueBool () == true)
	{
		opentplConn->setDebug (debugConn->getValueBool ());
		return;
	}

	int cycle = 0;
#define WAIT_POWER_SETTLE   24
#define WAIT_POWER_UP       36
 	while (cycle < WAIT_POWER_SETTLE && !(power_state == 0 || power_state == 1))
	{
		sleep (5);
		status = opentplConn->get ("CABINET.POWER_STATE", power_state, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "checkPower tpl_ret " << status << sendLog;
			opentplConn->setDebug (debugConn->getValueBool ());
			return;
		}
		cycle++;
	}
	if (cycle == WAIT_POWER_SETTLE)
	{
		logStream (CRITICAL_TELESCOPE_FAILURE | MESSAGE_ERROR) << "cannot settle down to power_state 0 or 1 - this is significant problem!" << sendLog;
		cabinetPowerState->setValueInteger (power_state);
		sendValueAll (cabinetPowerState);
		opentplConn->setDebug (debugConn->getValueBool ());
		maskState (DEVICE_ERROR_MASK, DEVICE_ERROR_HW, "cannot power down");
		return;
	}

	maskState (DEVICE_ERROR_HW, 0, "preinitialization completed");
	// sometime we can reach 1..
	if (power_state == 0)
	{
		status = opentplConn->set ("CABINET.POWER", 1, &status);
		sleep (5);
		cycle = 0;
		do
		{
			status = opentplConn->get ("CABINET.POWER_STATE", power_state, &status);
			logStream (MESSAGE_DEBUG) << "checkPower waiting for power up" << sendLog;
			sleep (5);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "checkPower power_state ret " << status << sendLog;
				opentplConn->setDebug (debugConn->getValueBool ());
				return;
			}
			cycle++;
		}
		while (power_state < 1 && cycle < WAIT_POWER_UP);
		if (cycle == WAIT_POWER_UP)
		{
			logStream (CRITICAL_TELESCOPE_FAILURE | MESSAGE_ERROR) << "cannot finish power up - this is significant problem!" << sendLog;
			cabinetPowerState->setValueInteger (power_state);
			sendValueAll (cabinetPowerState);
			opentplConn->setDebug (debugConn->getValueBool ());
			maskState (DEVICE_ERROR_MASK, DEVICE_ERROR_HW, "cannot power up");
			return;
		}
	}

	cycle = 0;

	while (cycle < WAIT_POWER_UP)
	{
		status = opentplConn->get ("CABINET.REFERENCED", referenced, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "checkPower get referenced " << status << sendLog;
			opentplConn->setDebug (debugConn->getValueBool ());
			return;
		}
		logStream (MESSAGE_DEBUG) << "checkPower referenced " << referenced << sendLog;
		sleep (5);
		if (referenced == 1)
			break;
/*		if (referenced == 0)
		{
			status = opentplConn->set ("CABINET.REINIT", 1, &status);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "checkPower reinit " << status << sendLog;
				return;
			}
		} */
		cycle++;
	}
	opentplConn->setDebug (debugConn->getValueBool ());
	if (cycle == WAIT_POWER_UP)
	{
		logStream (CRITICAL_TELESCOPE_FAILURE | MESSAGE_ERROR) << "cannot reference device" << sendLog;
		maskState (DEVICE_ERROR_MASK, DEVICE_ERROR_HW, "cannot power up");
		return;
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

void OpenTPL::getCover ()
{
	double cor_tmp;
	int status = TPL_OK;
	opentplConn->get ("COVER.REALPOS", cor_tmp, &status);
	if (status)
		return;
	cover->setValueDouble (cor_tmp);
}

void OpenTPL::initCoverState ()
{
	getCover ();
	if (cover->getValueDouble () == 0)
		cover_state = CLOSED;
	else if (cover->getValueDouble () == 1)
		cover_state = OPENED;
	else
		cover_state = CLOSING;
}

int OpenTPL::infoModel ()
{
	int status = TPL_OK;

	std::string dumpfile;
	int recordcount;

	status = opentplConn->get ("POINTING.POINTINGPARAMS.DUMPFILE", dumpfile, &status);

	for (std::vector <rts2core::ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		double pv;
		std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
		status = opentplConn->get (pn.c_str (), pv, &status);
		if (status != TPL_OK)
			return -1;
		(*iter)->setValueDouble (pv);
	}

	status = opentplConn->get ("POINTING.POINTINGPARAMS.RECORDCOUNT", recordcount, &status);

	if (status != TPL_OK)
		return -1;

	model_dumpFile->setValueString (dumpfile);
	model_recordcount->setValueInteger (recordcount);
	return 0;
}

int OpenTPL::scriptEnds ()
{
	if (derotatorOffset)
	{
	  	derotatorOffset->setValueDouble (derOff);
		sendValueAll (derotatorOffset);
	}
	return Telescope::scriptEnds ();
}

int OpenTPL::startMoveReal (double ra, double dec)
{
	int status = TPL_OK;
	if (targetChangeFromLastResync ())
	{
		status = opentplConn->set ("POINTING.TARGET.RA", ra / 15.0, &status);
		status = opentplConn->set ("POINTING.TARGET.DEC", dec, &status);	

		status = setTelescopeTrack (irTracking);
	}
	if (derotatorOffset && !getDerotatorPower ())
	{
		status = opentplConn->set ("DEROTATOR[3].POWER", 1, &status);
		// status = opentplConn->set ("CABINET.POWER", 1, &status);
	}

	double offset;
	int track;

	// apply corrections
	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			offset = getCorrRa ();
			// for north, we have to switch offset..
			if (telLatitude->getValueDouble () > 0)
				offset *= -1;
			status = opentplConn->set ("HA.OFFSET", offset, &status);
			status = opentplConn->get ("POINTING.TRACK", track, &status);
			offset = getCorrDec ();
			if (track == 3)
				offset *= -1.0;
			status = opentplConn->set ("DEC.OFFSET", offset, &status);
			break;
		case POINTING_ALTAZ:
			offset = getCorrZd ();
			status = opentplConn->set ("ZD.OFFSET", offset, &status);
			offset = getCorrAz ();
			status = opentplConn->set ("AZ.OFFSET", offset, &status);
			break;

	}

	if (isModelOn () && (getCorrRa () != 0 || getCorrDec () != 0))
	{
		status = opentplConn->set ("POINTING.POINTINGPARAMS.SAMPLE", 1, &status);
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
		status = opentplConn->set ("DEROTATOR[3].OFFSET", -1 * derotatorOffset->getValueDouble () , &status);

		if (status != TPL_OK)
			return status;
	}

	return status;
}

int OpenTPL::idle ()
{
	// check for errors..
	checkErrors ();
	if (cover)
		checkCover ();
	return Telescope::idle ();
}

void OpenTPL::getTelAltAz (struct ln_hrz_posn *hrz)
{
	int status = TPL_OK;
	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			Telescope::getTelAltAz (hrz);
			break;
		case POINTING_ALTAZ:
			status = opentplConn->get ("ZD.REALPOS", hrz->alt, &status);
			status = opentplConn->get ("AZ.REALPOS", hrz->az, &status);

			hrz->alt = 90 - fabs (hrz->alt);
			hrz->az = ln_range_degrees (hrz->az + 180);
			break;
	}
}

void OpenTPL::setDiffTrack (double dra, double ddec)
{
	int status = TPL_OK;
	status = opentplConn->set ("POINTING.TARGET.RA_V", dra, &status);
	status = opentplConn->set ("POINTING.TARGET.DEC_V", ddec, &status);

	if (status != TPL_OK)
		logStream (MESSAGE_ERROR) << "cannot set differential tracking " << status << sendLog;
}

int OpenTPL::info ()
{
	if (!opentplConn->isConnState (CONN_CONNECTED))
		return -1;
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

	status = opentplConn->get ("CABINET.POWER", cab_power, &status);
	status = opentplConn->get ("CABINET.POWER_STATE", cab_power_state, &status);
	if (status != TPL_OK)
		return -1;
	cabinetPower->setValueBool (cab_power == 1);
	cabinetPowerState->setValueFloat (cab_power_state);

	status = opentplConn->get ("POINTING.TRACK", track, &status);
	if (status != TPL_OK)
		return -1;

	if (!track && getPointingModel () != POINTING_RADEC)
	{
		// telRA, telDec are invalid: compute them from ZD/AZ
		struct ln_hrz_posn hrz;
		struct ln_lnlat_posn observer;
		struct ln_equ_posn curr;
		switch (getPointingModel ())
		{
			case POINTING_ALTAZ:
				status = opentplConn->get ("AZ.CURRPOS", az, &status);
				status = opentplConn->get ("ZD.CURRPOS", zd, &status);
				if (status != TPL_OK)
					return -1;
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
		status = opentplConn->get ("POINTING.CURRENT.RA", t_telRa, &status);
		t_telRa *= 15.0;
		status = opentplConn->get ("POINTING.CURRENT.DEC", t_telDec, &status);

		telFlip->setValueInteger (track == 3 ? 1 : 0);
	}

	double va1,va2;

	// get offsets..
	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			status = opentplConn->get ("POINTING.CURRENT.DH", va1, &status);
			status = opentplConn->get ("POINTING.CURRENT.DD", va2, &status);

			if (status != TPL_OK)
				return -1;

			om_offsets->setValueRaDec (va1, va2);
			break;
		case POINTING_ALTAZ:
			status = opentplConn->get ("POINTING.CURRENT.DA", va1, &status);
			status = opentplConn->get ("POINTING.CURRENT.DZ", va2, &status);

			if (status != TPL_OK)
				return -1;

			om_offsets->setValueRaDec (va1, va2);
			break;
	}

	if (status != TPL_OK)
		return -1;

	setTelRaDec (t_telRa, t_telDec);

	#ifdef DEBUG_EXTRA
	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			status = opentplConn->get ("HA.CURRSPEED", zd_speed, &status);
			status = opentplConn->get ("DEC.CURRSPEED", az_speed, &status);

			logStream (MESSAGE_DEBUG) << "info ra " << getTelRa ()
				<< " dec " << getTelDec ()
				<< " ha_speed " << az_speed
				<< " dec_speed " << zd_speed
				<< " track " << track
				<< sendLog;
			break;

		case POINTING_ALTAZ:
			status = opentplConn->get ("ZD.CURRSPEED", zd_speed, &status);
			status = opentplConn->get ("AZ.CURRSPEED", az_speed, &status);

			logStream (MESSAGE_DEBUG) << "info ra " << getTelRa ()
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
		opentplConn->get ("DEROTATOR[3].POWER", derPower, &status);
		if (status == TPL_OK)
		{
			derotatorPower->setValueBool (derPower == 1);
		}
	}

	if (cover)
		getCover ();

	mountTrack->setValueInteger (track);

	status = TPL_OK;
	double point_dist, point_dist1, point_dist2;
	double point_time;
	status = opentplConn->get ("POINTING.TARGETDISTANCE", point_dist, &status);
	status = opentplConn->get ("POINTING.SLEWINGTIME", point_time, &status);
	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			status = opentplConn->get ("HA.TARGETDISTANCE", point_dist1, &status);
			status = opentplConn->get ("DEC.TARGETDISTANCE", point_dist2, &status);
			break;
		case POINTING_ALTAZ:
			status = opentplConn->get ("ZD.TARGETDISTANCE", point_dist1, &status);
			status = opentplConn->get ("AZ.TARGETDISTANCE", point_dist2, &status);
			break;
	}
	if (status == TPL_OK)
	{
		targetDist->setValueDouble (point_dist);
		targetTime->setValueDouble (point_time);
		targetDist1->setValueDouble (point_dist1);
		targetDist2->setValueDouble (point_dist2);
	}

	return Telescope::info ();
}

int OpenTPL::saveModel ()
{
	int ret;
	ret = infoModel ();
	if (ret)
		return ret;
	std::ofstream of;
	of.open ("/etc/rts2/ir.model", std::ios_base::out | std::ios_base::trunc);
	of.precision (20);
	for (std::vector <rts2core::ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		double pv;
		int status = TPL_OK;
		std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
		status = opentplConn->get (pn.c_str (), pv, &status);
		if (status != TPL_OK)
			return -1;
		of << (*iter)->getName () << " " << pv << std::endl;
	}
	of.close ();
	if (of.fail ())
	{
		logStream (MESSAGE_ERROR) << "model saving failed" << sendLog;
		return -1;
	}
	return 0;
}

int OpenTPL::loadModel ()
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
			status = opentplConn->set (pnc.c_str (), pv, &status);
			if (status != TPL_OK)
				return -1;
		}
	}
	catch (std::exception & e)
	{
		logStream (MESSAGE_DEBUG) << "loadModel error" << sendLog;
		return -1;
	}
	return infoModel ();
}

int OpenTPL::resetMount ()
{
	int status = TPL_OK;
	int power = 0;
	double power_state;
	status = opentplConn->set ("CABINET.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "resetMount powering off: " << status <<
			sendLog;
		return -1;
	}
	while (true)
	{
		logStream (MESSAGE_DEBUG) << "resetMount waiting for power down" <<
			sendLog;
		sleep (5);
		status = opentplConn->get ("CABINET.POWER_STATE", power_state, &status);
		if (status)
		{
			logStream (MESSAGE_ERROR) << "resetMount power_state ret: " <<
				status << sendLog;
			return -1;
		}
		if (power_state == 0 || power_state == -1)
		{
			logStream (MESSAGE_DEBUG) << "resetMount final power_state: " <<
				power_state << sendLog;
			break;
		}
	}
	return Telescope::resetMount ();
}

int OpenTPL::startResync ()
{
	int status = 0;
	double sep;

	struct ln_equ_posn target;

	if (standbyPoweroff->getValueBool () == true)
		checkPower ();

	getTarget (&target);

	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			if (target.dec < -89.85)
				target.dec = -89.80;
			break;
		case POINTING_ALTAZ:
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
			status = opentplConn->set ("AZ.OFFSET", az_off, &status);
			status = opentplConn->set ("ZD.OFFSET", alt_off, &status);
			if (status)
			{
				logStream (MESSAGE_ERROR) << "startMove cannot zero offset" << sendLog;
				return -1;
			}
			break;
	}

	status = startMoveReal (target.ra, target.dec);
	if (status)
		return -1;

	// wait till we get it processed
	status = opentplConn->get ("POINTING.TARGETDISTANCE", sep, &status);
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

int OpenTPL::isMoving ()
{
	return moveCheck (false);
}

int OpenTPL::stopMove ()
{
	int status = 0;
	double zd;
	info ();
	// ZD check..
	status = opentplConn->get ("ZD.CURRPOS", zd, &status);
	if (status)
	{
		logStream (MESSAGE_DEBUG) << "stopMove cannot get ZD! (" << status << ")" << sendLog;
		return -1;
	}
	if (getPointingModel () == POINTING_ALTAZ && fabs (zd) < 1)
	{
		logStream (MESSAGE_DEBUG) << "stopMove suspicious ZD.. " << zd << " set TRACK to 0" << sendLog;
		status = setTelescopeTrack (0);
		if (status)
		{
			logStream (MESSAGE_DEBUG) << "stopMove cannot set track: " << status << sendLog;
			return -1;
		}
		return 0;
	}
	return 0;
}

int OpenTPL::startPark ()
{
	int status = TPL_OK;

	struct ln_hrz_posn hrzPark;
	struct ln_equ_posn equPark;
	struct ln_lnlat_posn observer;

	if (standbyPoweroff->getValueBool () == true)
		checkPower ();

	switch (getPointingModel ())
	{
		case POINTING_RADEC:
			observer.lng = telLongitude->getValueDouble ();
			observer.lat = telLatitude->getValueDouble ();

			hrzPark.alt = parkPos->getAlt ();
			hrzPark.az = parkPos->getAz ();

			ln_get_equ_from_hrz (&hrzPark, &observer, ln_get_julian_from_sys (), &equPark);

			status = opentplConn->set ("POINTING.TARGET.RA", equPark.ra / 15.0, &status);
			status = opentplConn->set ("POINTING.TARGET.DEC", equPark.dec, &status);
			setTelescopeTrack (irTracking);

			setTarget (equPark.ra, equPark.dec);

			sleep (15);
			break;
		case POINTING_ALTAZ:
			status = opentplConn->set ("AZ.TARGETPOS", parkPos->getAz (), &status);
			status = opentplConn->set ("ZD.TARGETPOS", 90 - parkPos->getAlt (), &status);
			status = opentplConn->set ("DEROTATOR[3].POWER", 0, &status);
			break;
	}
	if (status)
	{
		logStream (MESSAGE_ERROR) << "startPark status " << status << sendLog;
		return -1;
	}
	time (&timeout);
	timeout += 300;
	return 1;
}

int OpenTPL::moveCheck (bool park)
{
	int status = TPL_OK;
	int track;
	double poin_dist;
	time_t now;
	// if parking, check is different for EQU telescope
	if (park && getPointingModel () != POINTING_RADEC)
	{
		struct ln_equ_posn tPos;
		struct ln_equ_posn cPos;

		switch (getPointingModel ())
		{
			case POINTING_ALTAZ:
				status = opentplConn->get ("ZD.TARGETPOS", tPos.ra, &status);
				status = opentplConn->get ("AZ.TARGETPOS", tPos.dec, &status);
				status = opentplConn->get ("ZD.CURRPOS", cPos.ra, &status);
				status = opentplConn->get ("AZ.CURRPOS", cPos.dec, &status);
				break;
		}
		poin_dist = ln_get_angular_separation (&cPos, &tPos);
	}
	else
	{
		status = opentplConn->get ("POINTING.TARGETDISTANCE", poin_dist, &status);
	}
	if (status != TPL_OK)
		return -1;
	time (&now);
	// get track..
	status = opentplConn->get ("POINTING.TRACK", track, &status);
	if (track == 0 && !park)
	{
		logStream (MESSAGE_WARNING) << "Tracking sudently stopped, reenable tracking (track=" << track << " park = " << park << ")" << sendLog;
		setTelescopeTrack (irTracking);
		sleep (10);
		return USEC_SEC / 100;
	}
	if (fabs (poin_dist) <= goodSep->getValueDouble ()) 
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "isMoving target distance " << poin_dist << sendLog;
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

int OpenTPL::isParking ()
{
	return moveCheck (true);
}

int OpenTPL::endPark ()
{
	setTelescopeTrack (0);
	sleep (5);
	if (standbyPoweroff->getValueBool () == true)
		powerOff ();
	return 0;
}

int OpenTPL::setTracking (int track, bool addTrackingTimer, bool send, const char *stopMsg)
{
	int ret;

	if (track)
		ret = startWorm ();
	else
		ret = stopWorm ();

	if (ret)
		return ret;

	return Telescope::setTracking (track, addTrackingTimer, send, stopMsg);
}

int OpenTPL::startWorm ()
{
	int status = TPL_OK;
	status = setTelescopeTrack (irTracking);
	if (status != TPL_OK)
		return -1;
	return 0;
}

int OpenTPL::stopWorm ()
{
	int status = TPL_OK;
	status = setTelescopeTrack (0);
	if (status)
		return -1;
	return 0;
}

void OpenTPL::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_ONOFF_MASK))
	{
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			powerOn ();
			coverOpen ();
			break;
		default:
			coverClose ();
			break;
	}
	Telescope::changeMasterState (old_state, new_state);
}

int main (int argc, char **argv)
{
	OpenTPL device = OpenTPL (argc, argv);
	return device.run ();
}
