/* 
 * Driver for OpenTPL mounts.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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
#include "ir.h"

#define DEBUG_EXTRA

int
Rts2TelescopeIr::coverClose ()
{
	if (cover == NULL)
		return 0;
	int status = TPL_OK;
	double targetPos;
	if (cover_state == CLOSED)
		return 0;
	status = irConn->tpl_set ("COVER.TARGETPOS", 0, &status);
	status = irConn->tpl_set ("COVER.POWER", 1, &status);
	status = irConn->tpl_get ("COVER.TARGETPOS", targetPos, &status);
	cover_state = CLOSING;
	logStream (MESSAGE_INFO) << "closing cover, status" << status <<
		" target position " << targetPos << sendLog;
	return status;
}


int
Rts2TelescopeIr::coverOpen ()
{
	if (cover == NULL)
		return 0;
	int status = TPL_OK;
	if (cover_state == OPENED)
		return 0;
	status = irConn->tpl_set ("COVER.TARGETPOS", 1, &status);
	status = irConn->tpl_set ("COVER.POWER", 1, &status);
	cover_state = OPENING;
	logStream (MESSAGE_INFO) << "opening cover, status" << status << sendLog;
	return status;
}


int
Rts2TelescopeIr::setTelescopeTrack (int new_track)
{
	int status = TPL_OK;
	int old_track;
	status = irConn->tpl_get ("POINTING.TRACK", old_track, &status);
	if (status != TPL_OK)
		return -1;
	status = irConn->tpl_setww ("POINTING.TRACK", new_track | (old_track & 128), &status);
	if (status != TPL_OK)
	{
		logStream (MESSAGE_ERROR) << "Cannot setTelescopeTrack" << sendLog;
	}
	return status;
}


int
Rts2TelescopeIr::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	int status = TPL_OK;
	if (old_value == cabinetPower)
	{
		status =
			irConn->tpl_set ("CABINET.POWER",
			((Rts2ValueBool *) new_value)->getValueBool ()? 1 : 0,
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorOffset)
	{
		status =
			irConn->tpl_set ("DEROTATOR[3].OFFSET", -1 * new_value->getValueDouble (),
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorCurrpos)
	{
		status =
			irConn->tpl_set ("DEROTATOR[3].TARGETPOS", new_value->getValueDouble (),
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == derotatorPower)
	{
		status =
			irConn->tpl_set ("DEROTATOR[3].POWER",
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
			status = irConn->tpl_set (pn.c_str (), new_value->getValueDouble (), &status);
			if (status != TPL_OK)
				return -2;
			return 0;
		}
	}
	if (old_value == model_recordcount)
	{
		status = irConn->tpl_set ("POINTING.POINTINGPARAMS.RECORDCOUNT", new_value->getValueInteger (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}

	return Rts2DevTelescope::setValue (old_value, new_value);
}


Rts2TelescopeIr::Rts2TelescopeIr (int in_argc, char **in_argv)
:Rts2DevTelescope (in_argc, in_argv)
{
	ir_port = 0;
	irConn = NULL;

	doCheckPower = false;

	createValue (cabinetPower, "cabinet_power", "power of cabinet", false);
	createValue (cabinetPowerState, "cabinet_power_state", "power state of cabinet", false);

	derotatorOffset = NULL;
	derotatorCurrpos = NULL;
	derotatorPower = NULL;

	createValue (targetDist, "target_dist", "distance in degrees to target", false, RTS2_DT_DEG_DIST);
	createValue (targetTime, "target_time", "reach target time in seconds", false);

	createValue (mountTrack, "TRACK", "mount track");

	cover = NULL;

	addOption ('I', "ir_ip", 1, "IR TCP/IP address");
	addOption ('N', "ir_port", 1, "IR TCP/IP port number");
	addOption ('p', "check power", 0, "whenever to check for power state != 0 (currently depreciated)");

	strcpy (telType, "BOOTES_IR");

	cover_state = CLOSED;
}


Rts2TelescopeIr::~Rts2TelescopeIr (void)
{
	delete irConn;
}


int
Rts2TelescopeIr::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'I':
			ir_ip = std::string (optarg);
			break;
		case 'N':
			ir_port = atoi (optarg);
			break;
		case 'p':
			doCheckPower = true;
			break;
		default:
			return Rts2DevTelescope::processOption (in_opt);
	}
	return 0;
}


int
Rts2TelescopeIr::initIrDevice ()
{
	Rts2Config *config = Rts2Config::instance ();
	config->loadFile (NULL);
	// try to get default from config file
	if (ir_ip.length () == 0)
	{
		config->getString ("ir", "ip", ir_ip);
	}
	if (!ir_port)
	{
		config->getInteger ("ir", "port", ir_port);
	}
	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	irConn = new rts2core::OpenTpl (this, ir_ip, ir_port);
	
	int ret = irConn->init ();
	if (ret)
		return ret;

	// are we connected ?
	if (!irConn->isOK ())
	{
		std::cerr << "Connection to server failed" << std::endl;
		return -1;
	}

	return 0;
}


int
Rts2TelescopeIr::init ()
{
	int ret;
	ret = Rts2DevTelescope::init ();
	if (ret)
		return ret;

	ret = initIrDevice ();
	if (ret)
		return ret;

	return 0;
}


int
Rts2TelescopeIr::initValues ()
{
	int status = TPL_OK;
	std::string serial;

	std::string config_mount;

	status = irConn->tpl_get ("CONFIG.MOUNT", config_mount, &status);
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

	status = irConn->getValueDouble ("LOCAL.LATITUDE", telLatitude, &status);
	status = irConn->getValueDouble ("LOCAL.LONGITUDE", telLongitude, &status);
	status = irConn->getValueDouble ("LOCAL.HEIGHT", telAltitude, &status);

	if (status != TPL_OK)
	{
		return -1;
	}

	irConn->tpl_get ("CABINET.SETUP.HW_ID", serial, &status);
	addConstValue ("IR_HWID", "serial number", serial.c_str ());

	if (irConn->haveModule ("COVER"))
	{
		createValue (cover, "cover", "cover state (1 = opened)", false);
		initCoverState ();
	}

	// nasmith derotator
	if (irConn->haveModule ("DEROTATOR[3]"))
	{
		createValue (derotatorOffset, "DER_OFF", "derotator offset", true, RTS2_DT_ROTANG, 0, true);
		createValue (derotatorCurrpos, "DER_CUR", "derotator current position", true, RTS2_DT_DEGREES);
		createValue (derotatorPower, "derotatorPower", "derotator power setting", false);
	}

	// infoModel
	status = infoModel ();
	if (status)
		return status;

	return Rts2DevTelescope::initValues ();
}


void
Rts2TelescopeIr::checkErrors ()
{
	int status = TPL_OK;
	std::string list;

	status = irConn->tpl_get ("CABINET.STATUS.LIST", list, &status);
	if (status == 0 && list.length () > 2 && list != errorList)
	{
		// print errors to log & ends..
		logStream (MESSAGE_ERROR) << "IR checkErrors Telescope errors " <<
			list << sendLog;
		errorList = list;
		irConn->tpl_set ("CABINET.STATUS.CLEAR", 1, &status);
		if (list == "\"ERR_Soft_Limit_max:4:ZD\"")
		{
			double zd;
			status = irConn->tpl_get ("ZD.CURRPOS", zd, &status);
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
				status = irConn->tpl_set ("ZD.TARGETPOS", zd, &status);
				logStream (MESSAGE_ERROR) <<
					"IR checkErrors zd soft limit reset " <<
					zd << " (" << status << ")" << sendLog;
			}
		}
	}
}


void
Rts2TelescopeIr::checkCover ()
{
	int status = TPL_OK;
	switch (cover_state)
	{
		case OPENING:
			getCover ();
			if (cover->getValueDouble () == 1.0)
			{
				irConn->tpl_set ("COVER.POWER", 0, &status);
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
				irConn->tpl_set ("COVER.POWER", 0, &status);
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
Rts2TelescopeIr::checkPower ()
{
	int status = TPL_OK;
	double power_state;
	double referenced;
	status = irConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
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
		status = irConn->tpl_set ("CABINET.POWER", 1, &status);
		status = irConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
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
			status = irConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
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
		status = irConn->tpl_get ("CABINET.REFERENCED", referenced, &status);
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
			status = irConn->tpl_set ("CABINET.REINIT", 1, &status);
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
Rts2TelescopeIr::getCover ()
{
	double cor_tmp;
	int status = TPL_OK;
	irConn->tpl_get ("COVER.REALPOS", cor_tmp, &status);
	if (status)
		return;
	cover->setValueDouble (cor_tmp);
}


void
Rts2TelescopeIr::initCoverState ()
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
Rts2TelescopeIr::infoModel ()
{
	int status = TPL_OK;

	std::string dumpfile;
	int recordcount;

	status = irConn->tpl_get ("POINTING.POINTINGPARAMS.DUMPFILE", dumpfile, &status);

	for (std::vector <Rts2ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		double pv;
		std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
		status = irConn->tpl_get (pn.c_str (), pv, &status);
		if (status != TPL_OK)
			return -1;
		(*iter)->setValueDouble (pv);
	}

	status = irConn->tpl_get ("POINTING.POINTINGPARAMS.RECORDCOUNT", recordcount, &status);

	if (status != TPL_OK)
		return -1;

	model_dumpFile->setValueString (dumpfile);
	model_recordcount->setValueInteger (recordcount);
	return 0;
}


int
Rts2TelescopeIr::idle ()
{
	// check for power..
	checkPower ();
	// check for errors..
	checkErrors ();
	if (cover)
		checkCover ();
	return Rts2DevTelescope::idle ();
}


int
Rts2TelescopeIr::ready ()
{
	return !irConn->isOK ();
}


int
Rts2TelescopeIr::getAltAz ()
{
	int status = TPL_OK;
	double zd, az;

	if (getPointingModel () == 0)
		return Rts2DevTelescope::getAltAz ();

	status = irConn->tpl_get ("ZD.REALPOS", zd, &status);
	status = irConn->tpl_get ("AZ.REALPOS", az, &status);

	if (status != TPL_OK)
		return -1;

	telAltAz->setValueAltAz (90 - fabs (zd), ln_range_degrees (az + 180));

	return 0;
}


int
Rts2TelescopeIr::info ()
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

	if (!irConn->isOK ())
		return -1;

	status = irConn->tpl_get ("CABINET.POWER", cab_power, &status);
	status = irConn->tpl_get ("CABINET.POWER_STATE", cab_power_state, &status);
	if (status != TPL_OK)
		return -1;
	cabinetPower->setValueBool (cab_power == 1);
	cabinetPowerState->setValueFloat (cab_power_state);

	status = irConn->tpl_get ("POINTING.TRACK", track, &status);
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
				status = irConn->tpl_get ("HA.CURRPOS", t_telRa, &status);
				status = irConn->tpl_get ("DEC.CURRPOS", t_telDec, &status);
				if (status != TPL_OK)
				{
					return -1;
				}
				t_telRa = ln_range_degrees (ln_get_mean_sidereal_time (ln_get_julian_from_sys ()) - t_telRa);
				setTelRaDec (t_telRa, t_telDec);
				break;
			case 1:
				hrz.az = az;
				hrz.alt = 90 - fabs (zd);
				observer.lng = telLongitude->getValueDouble ();
				observer.lat = telLatitude->getValueDouble ();
				ln_get_equ_from_hrz (&hrz, &observer, ln_get_julian_from_sys (), &curr);
				setTelRa (curr.ra);
				setTelDec (curr.dec);
				telFlip->setValueInteger (0);
				break;
		}
	}
	else
	{
		status = irConn->tpl_get ("POINTING.CURRENT.RA", t_telRa, &status);
		t_telRa *= 15.0;
		status = irConn->tpl_get ("POINTING.CURRENT.DEC", t_telDec, &status);

		telFlip->setValueInteger (0);
	}

	if (status != TPL_OK)
		return -1;

	setTelRaDec (t_telRa, t_telDec);

	#ifdef DEBUG_EXTRA
	switch (getPointingModel ())
	{
		case 0:
			status = irConn->tpl_get ("HA.CURRSPEED", zd_speed, &status);
			status = irConn->tpl_get ("DEC.CURRSPEED", az_speed, &status);

			logStream (MESSAGE_DEBUG) << "IR info ra " << getTelRa ()
				<< " dec " << getTelDec ()
				<< " ha_speed " << az_speed
				<< " dec_speed " << zd_speed
				<< " track " << track
				<< sendLog;
			break;

		case 1:
			status = irConn->tpl_get ("ZD.CURRSPEED", zd_speed, &status);
			status = irConn->tpl_get ("AZ.CURRSPEED", az_speed, &status);

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
		irConn->getValueDouble ("DEROTATOR[3].OFFSET", derotatorOffset, &status);
		irConn->getValueDouble ("DEROTATOR[3].CURRPOS", derotatorCurrpos, &status);
		irConn->tpl_get ("DEROTATOR[3].POWER", derPower, &status);
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
	status = irConn->tpl_get ("POINTING.TARGETDISTANCE", point_dist, &status);
	status = irConn->tpl_get ("POINTING.SLEWINGTIME", point_time, &status);
	if (status == TPL_OK)
	{
		targetDist->setValueDouble (point_dist);
		targetTime->setValueDouble (point_time);
	}

	return Rts2DevTelescope::info ();
}


int
Rts2TelescopeIr::saveModel ()
{
	std::ofstream of;
	of.open ("/etc/rts2/ir.model", std::ios_base::out | std::ios_base::trunc);
	of.precision (20);
	for (std::vector <Rts2ValueDouble *>::iterator iter = modelParams.begin (); iter != modelParams.end (); iter++)
	{
		double pv;
		int status = TPL_OK;
		std::string pn = std::string ("POINTING.POINTINGPARAMS.") + (*iter)->getName ();
		status = irConn->tpl_get (pn.c_str (), pv, &status);
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
Rts2TelescopeIr::loadModel ()
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
			status = irConn->tpl_set (pnc.c_str (), pv, &status);
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
Rts2TelescopeIr::resetMount ()
{
	int status = TPL_OK;
	int power = 0;
	double power_state;
	status = irConn->tpl_set ("CABINET.POWER", power, &status);
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
		status = irConn->tpl_get ("CABINET.POWER_STATE", power_state, &status);
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
	return Rts2DevTelescope::resetMount ();
}
