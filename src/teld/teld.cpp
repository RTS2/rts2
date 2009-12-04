/* 
 * Telescope control daemon.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <libnova/libnova.h>

#include "../utils/rts2device.h"
#include "../utils/libnova_cpp.h"

#include "teld.h"
#include "rts2devclicop.h"

#include "model/telmodel.h"

#define OPT_BLOCK_ON_STANDBY  OPT_LOCAL + 117
#define OPT_HORIZON           OPT_LOCAL + 118

using namespace rts2teld;

Telescope::Telescope (int in_argc, char **in_argv):Rts2Device (in_argc, in_argv, DEVICE_TYPE_MOUNT, "T0")
{
	for (int i = 0; i < 4; i++)
	{
		timerclear (dir_timeouts + i);
	}

	// object
	createValue (oriRaDec, "ORI", "original position (J2000)", true, RTS2_VALUE_WRITABLE);
	// users offset
	createValue (offsRaDec, "OFFS", "object offset", true, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE, 0);
	offsRaDec->setValueRaDec (0, 0);

	createValue (woffsRaDec, "woffs", "offsets waiting to be applied", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE, 0);
	woffsRaDec->setValueRaDec (0, 0);
	woffsRaDec->resetValueChanged ();

	createValue (objRaDec, "OBJ", "telescope FOV center position (J200) - with offsets applied", true);

	createValue (tarRaDec, "TAR", "target position (J2000)", true);

	createValue (corrRaDec, "CORR_", "correction from closed loop", true, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE, 0);
	corrRaDec->setValueRaDec (0, 0);

	createValue (wcorrRaDec, "wcorr", "corrections which waits for being applied", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE, 0);
	wcorrRaDec->setValueRaDec (0, 0);
	wcorrRaDec->resetValueChanged ();

	createValue (wCorrImgId, "wcorr_img", "Image id waiting for correction", false, RTS2_VALUE_WRITABLE, 0, true);

	// position error
	createValue (posErr, "pos_err", "error in degrees", false, RTS2_DT_DEG_DIST);

	createValue (telTargetRaDec, "tel_target", "target RA DEC telescope coordinates - one feeded to TCS", false);

	createValue (modelRaDec, "MO_RTS2", "[deg] RTS2 model offsets", true, RTS2_DT_DEGREES, 0);
	modelRaDec->setValueRaDec (0, 0);

	// target + model + corrections = sends to tel ... TEL (read from sensors, if possible)
	createValue (telRaDec, "TEL", "mount position (read from sensors)", true);

	createValue (telAltAz, "TEL_", "horizontal telescope coordinates", true);

	createValue (pointingModel, "pointing", "pointing model (equ, alt-az, ...)", false, 0, 0, true);
	pointingModel->addSelVal ("EQU");
	pointingModel->addSelVal ("ALT-AZ");
	pointingModel->addSelVal ("ALT-ALT");

	createConstValue (telLatitude, "LATITUDE", "observatory latitude", true, RTS2_DT_DEGREES);
	createConstValue (telLongitude, "LONGITUD", "observatory longitude", true, RTS2_DT_DEGREES);
	createConstValue (telAltitude, "ALTITUDE", "observatory altitude", true);


	createValue (mountParkTime, "PARKTIME", "Time of last mount park");

	createValue (blockMove, "block_move", "if true, any software movement of the telescope is blocked", false, RTS2_VALUE_WRITABLE);
	blockMove->setValueBool (false);

	createValue (blockOnStandby, "block_on_standby", "Block telescope movement if switched to standby/off mode. Enable it if switched back to on.", false, RTS2_VALUE_WRITABLE);
	blockOnStandby->setValueBool (false);

	createValue (airmass, "AIRMASS", "Airmass of target location");
	createValue (hourAngle, "HA", "Location hour angle", true, RTS2_DT_RA);
	createValue (lst, "LST", "Local Sidereal Time", true, RTS2_DT_RA);

	createValue (targetDistance, "target_distance", "distance to the target in degrees", false, RTS2_DT_DEG_DIST);
	createValue (targetStarted, "move_started", "time when movement was started", false);
	createValue (targetReached, "move_end", "expected time when telescope will reach the destination", false);

	targetDistance->setValueDouble (nan("f"));
	targetStarted->setValueDouble (nan("f"));
	targetReached->setValueDouble (nan("f"));

	createValue (moveNum, "MOVE_NUM", "number of movements performed by the driver; used in corrections for synchronization", true);
	moveNum->setValueInteger (0);

	createValue (corrImgId, "CORR_IMG", "ID of last image used for correction", true, RTS2_VALUE_WRITABLE);
	corrImgId->setValueInteger (0);

	defaultRotang = 0;
	createValue (rotang, "MNT_ROTA", "mount rotang", true, RTS2_DT_ROTANG | RTS2_VALUE_WRITABLE);

	move_connection = NULL;

	createValue (ignoreCorrection, "ignore_correction", "corrections below this value will be ignored", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE, 0, true);
	ignoreCorrection->setValueDouble (0);

	createValue (smallCorrection, "small_correction", "correction bellow this value will be considered as small", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
	smallCorrection->setValueDouble (0);

	createValue (modelLimit, "model_limit", "model separation limit", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
	modelLimit->setValueDouble (5.0);

	createValue (telFov, "telescope_fov", "telescope field of view", false, RTS2_DT_DEG_DIST | RTS2_VALUE_WRITABLE);
	telFov->setValueDouble (180.0);

	createValue (telFlip, "MNT_FLIP", "telescope flip");

	// default is to aply model corrections
	createValue (calAberation, "CAL_ABER", "if aberation is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calAberation->setValueBool (false);

	createValue (calPrecession, "CAL_PREC", "if precession is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calPrecession->setValueBool (false);

	createValue (calRefraction, "CAL_REFR", "if refraction is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calRefraction->setValueBool (false);

	createValue (calModel, "CAL_MODE", "if model calculations are included in target calcuations", false, RTS2_VALUE_WRITABLE);
	calModel->setValueBool (false);

	modelFile = NULL;
	model = NULL;

	standbyPark = false;
	horizonFile = NULL;
	hardHorizon = NULL;

	addOption ('m', NULL, 1, "name of file holding model parameters, calculated by T-Point");
	addOption ('l', NULL, 1, "separation limit (corrections above that number in degrees will be ignored)");
	addOption ('g', NULL, 1, "minimal good separation. Correction above that number will be aplied immediately. Default to 180 deg");

	addOption ('c', NULL, 1, "minimal value for corrections. Corrections bellow that value will be rejected.");

	addOption ('s', NULL, 0, "park when switched to standby");
	addOption (OPT_BLOCK_ON_STANDBY, "block-on-standby", 0, "block telescope movement when switching to standby");

	addOption ('r', NULL, 1, "telescope rotang");
	addOption (OPT_HORIZON, "horizon", 1, "telescope hard horizon");

	// send telescope position every 60 seconds
	setIdleInfoInterval (60);

	moveInfoCount = 0;
	moveInfoMax = 100;
}

Telescope::~Telescope (void)
{
	delete model;
}


double Telescope::getLocSidTime (double JD)
{
	double ret;
	ret = ln_get_apparent_sidereal_time (JD) * 15.0 + telLongitude->getValueDouble ();
	return ln_range_degrees (ret) / 15.0;
}

int Telescope::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'm':
			modelFile = optarg;
			calModel->setValueBool (true);
			break;
		case 'l':
			modelLimit->setValueDouble (atof (optarg));
			break;
		case 'g':
			telFov->setValueDouble (atof (optarg));
			break;
		case 'c':
			ignoreCorrection->setValueDouble (atof (optarg));
			break;
		case OPT_BLOCK_ON_STANDBY:
			blockOnStandby->setValueBool (true);
			break;
		case 's':
			standbyPark = true;
			break;
		case 'r':
			defaultRotang = atof (optarg);
			rotang->setValueDouble (defaultRotang);
			break;
		case OPT_HORIZON:
			horizonFile = optarg;
			break;
		default:
			return Rts2Device::processOption (in_opt);
	}
	return 0;
}

void Telescope::calculateCorrAltAz ()
{
	struct ln_equ_posn equ_target;
	struct ln_equ_posn equ_corr;

	struct ln_lnlat_posn observer;

	double jd = ln_get_julian_from_sys ();

	equ_target.ra = tarRaDec->getRa ();
	equ_target.dec = tarRaDec->getDec ();

	equ_corr.ra = equ_target.ra - corrRaDec->getRa ();
	equ_corr.dec = equ_target.dec - corrRaDec->getDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	ln_get_hrz_from_equ (&equ_target, &observer, jd, &tarAltAz);

	if (corrRaDec->getRa () == 0 && corrRaDec->getDec () == 0)
	{
		corrAltAz.alt = tarAltAz.alt;
		corrAltAz.az = tarAltAz.az;
	}
	else
	{
		ln_get_hrz_from_equ (&equ_corr, &observer, jd, &corrAltAz);
	}
}

double Telescope::getCorrZd ()
{
	if (corrRaDec->getRa () == 0 && corrRaDec->getDec () == 0)
		return 0;

	calculateCorrAltAz ();

	return corrAltAz.alt - tarAltAz.alt;
}

double Telescope::getCorrAz ()
{
	if (corrRaDec->getRa () == 0 && corrRaDec->getDec () == 0)
		return 0;

	calculateCorrAltAz ();

	return tarAltAz.az - corrAltAz.az;
}

double Telescope::getTargetDistance ()
{
	struct ln_equ_posn tar,tel;
	getTelTargetRaDec (&tar);
	getTelRaDec (&tel);

	if (isnan(tar.ra) || isnan(tar.dec) || isnan(tel.ra) || isnan(tel.dec))
		return -1;

	return ln_get_angular_separation (&tel, &tar);
}

void Telescope::getTelTargetAltAz (struct ln_hrz_posn *hrz, double jd)
{
	struct ln_equ_posn tar;
	getTelTargetRaDec (&tar);
	struct ln_lnlat_posn observer;
	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();
	ln_get_hrz_from_equ (&tar, &observer, jd, hrz);
}

double Telescope::getTargetHa ()
{
	return getTargetHa (ln_get_julian_from_sys ());
}

double Telescope::getTargetHa (double jd)
{
	return ln_range_degrees (ln_get_apparent_sidereal_time (jd) - telRaDec->getRa ());
}

double Telescope::getLstDeg (double JD)
{
	return ln_range_degrees (15 * ln_get_apparent_sidereal_time (JD) +
		telLongitude->getValueDouble ());
}

void Telescope::valueChanged (Rts2Value * changed_value)
{
	if (changed_value == woffsRaDec)
	{
		maskState (BOP_EXPOSURE, BOP_EXPOSURE, "blocking exposure for offsets");
	}
	if (changed_value == oriRaDec
		|| changed_value == offsRaDec
		|| changed_value == corrRaDec)
	{
		startResyncMove (NULL, false);
	}
	Rts2Device::valueChanged (changed_value);
}

void Telescope::applyAberation (struct ln_equ_posn *pos, double JD)
{
	ln_get_equ_aber (pos, JD, pos);
}

void Telescope::applyPrecession (struct ln_equ_posn *pos, double JD)
{
	ln_get_equ_prec (pos, JD, pos);
}

void Telescope::applyRefraction (struct ln_equ_posn *pos, double JD)
{
	struct ln_hrz_posn hrz;
	struct ln_lnlat_posn obs;
	double ref;

	obs.lng = telLongitude->getValueDouble ();
	obs.lat = telLatitude->getValueDouble ();

	ln_get_hrz_from_equ (pos, &obs, JD, &hrz);
	ref = ln_get_refraction_adj (hrz.alt, 860, 10);
	hrz.alt += ref;
	ln_get_equ_from_hrz (&hrz, &obs, JD, pos);
}

void Telescope::incMoveNum ()
{
	// reset offsets
	offsRaDec->setValueRaDec (0, 0);
	offsRaDec->resetValueChanged ();

	woffsRaDec->setValueRaDec (0, 0);
	woffsRaDec->resetValueChanged ();

	modelRaDec->setValueRaDec (0, 0);
	modelRaDec->resetValueChanged ();

	corrRaDec->setValueRaDec (0, 0);
	corrRaDec->resetValueChanged ();

	wcorrRaDec->setValueRaDec (0, 0);
	wcorrRaDec->resetValueChanged ();

	moveNum->inc ();

	corrImgId->setValueInteger (0);
	wCorrImgId->setValueInteger (0);
}

void Telescope::applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD)
{
	struct ln_equ_posn hadec;
	double ra;
	double ls;
	if (!model || calModel->getValueBool () == false)
	{
		model_change->ra = -1 * corrRaDec->getRa();
		model_change->dec = -1 * corrRaDec->getDec();

		pos->ra = ln_range_degrees (pos->ra + corrRaDec->getRa ());
		pos->dec = pos->dec + corrRaDec->getDec ();
		telTargetRaDec->setValueRaDec (pos->ra, pos->dec);
		return;
	}
	ls = getLstDeg (JD);
	hadec.ra = ln_range_degrees (ls - pos->ra);
	hadec.dec = pos->dec;

	// change RA and DEC when modeling oposite side and we have only one model
	if (flip && model)
	{
		LibnovaRaDec fhadec (&hadec);

		struct ln_lnlat_posn observer;
		observer.lng = telLongitude->getValueDouble ();
		observer.lat = telLatitude->getValueDouble ();

		fhadec.flip (&observer);

		hadec.ra = fhadec.getRa ();
		hadec.dec = fhadec.getDec ();

		model->reverse (&hadec);

		// now flip back
		fhadec.setRa (hadec.ra);
		fhadec.setDec (hadec.dec);

		fhadec.flip (&observer);

		hadec.ra = fhadec.getRa ();
		hadec.dec = fhadec.getDec ();
	}
	else
	{
		model->reverse (&hadec);
	}

	// get back from model - from HA
	ra = ln_range_degrees (ls - hadec.ra);

	// calculate change
	model_change->ra = ln_range_degrees (pos->ra - ra);
	model_change->dec = pos->dec - hadec.dec;

	if (model_change->ra > 180)
		model_change->ra -= 360.0;

	hadec.ra = ra;

	double sep = ln_get_angular_separation (&hadec, pos);

	// change above modelLimit are strange - reject them
	if (sep > modelLimit->getValueDouble ())
	{
		logStream (MESSAGE_WARNING)
			<< "telescope applyModel big change - rejecting "
			<< model_change->ra << " "
			<< model_change->dec << " sep " << sep << sendLog;
		model_change->ra = 0;
		model_change->dec = 0;
		modelRaDec->setValueRaDec (0, 0);
		pos->ra = ln_range_degrees (pos->ra + corrRaDec->getRa ());
		pos->dec = pos->dec + corrRaDec->getDec ();
		telTargetRaDec->setValueRaDec (pos->ra, pos->dec);
		return;
	}

	logStream (MESSAGE_DEBUG)
		<< "Telescope::applyModel offsets ra: "
		<< model_change->ra << " dec: " << model_change->dec
		<< sendLog;

	modelRaDec->setValueRaDec (model_change->ra, model_change->dec);

	model_change->ra -= corrRaDec->getRa();
	model_change->dec -= corrRaDec->getDec();

	pos->ra = ln_range_degrees (ra + corrRaDec->getRa ());
	pos->dec = hadec.dec + corrRaDec->getDec ();

	telTargetRaDec->setValueRaDec (pos->ra, pos->dec);
}

int Telescope::init ()
{
	int ret;
	ret = Rts2Device::init ();
	if (ret)
		return ret;

	if (modelFile)
	{
		model = new rts2telmodel::Model (this, modelFile);
		ret = model->load ();
		if (ret)
			return ret;
	}

	if (horizonFile)
	{
		hardHorizon = new ObjectCheck (horizonFile);
	}

	return 0;
}

int Telescope::initValues ()
{
	int ret;
	ret = info ();
	if (ret)
		return ret;
	tarRaDec->setFromValue (telRaDec);
	telTargetRaDec->setFromValue (telRaDec);
	oriRaDec->setFromValue (telRaDec);
	objRaDec->setFromValue (telRaDec);
	modelRaDec->setValueRaDec (0, 0);

	return Rts2Device::initValues ();
}

void Telescope::checkMoves ()
{
	int ret;
	if ((getState () & TEL_MASK_MOVING) == TEL_MOVING)
	{
		ret = isMoving ();
		if (ret >= 0)
		{
			setTimeout (ret);
			if (moveInfoCount == moveInfoMax)
			{
				info ();
				if (move_connection)
					sendInfo (move_connection);
				moveInfoCount = 0;
			}
			else
			{
				moveInfoCount++;
			}
		}
		else if (ret == -1)
		{
			if (move_connection)
				sendInfo (move_connection);
			maskState (DEVICE_ERROR_MASK | TEL_MASK_CORRECTING |
				TEL_MASK_MOVING | BOP_EXPOSURE,
				DEVICE_ERROR_HW | TEL_NOT_CORRECTING | TEL_OBSERVING,
				"move finished with error");
			move_connection = NULL;
			setIdleInfoInterval (60);
		}
		else if (ret == -2)
		{
			infoAll ();
			ret = endMove ();
			if (ret)
			{
				maskState (DEVICE_ERROR_MASK | TEL_MASK_CORRECTING |
					TEL_MASK_MOVING | BOP_EXPOSURE,
					DEVICE_ERROR_HW | TEL_NOT_CORRECTING | TEL_OBSERVING,
					"move finished with error");
			}
			else
				maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE,
					TEL_NOT_CORRECTING | TEL_OBSERVING,
					"move finished without error");
			if (move_connection)
			{
				sendInfo (move_connection);
			}
			move_connection = NULL;
			setIdleInfoInterval (60);
		}
	}
	else if ((getState () & TEL_MASK_MOVING) == TEL_PARKING)
	{
		ret = isParking ();
		if (ret >= 0)
		{
			setTimeout (ret);
		}
		if (ret == -1)
		{
			infoAll ();
			maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING | BOP_EXPOSURE,
				DEVICE_ERROR_HW | TEL_PARKED,
				"park command finished with error");
			setIdleInfoInterval (60);
		}
		else if (ret == -2)
		{
			infoAll ();
			ret = endPark ();
			if (ret)
				maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING | BOP_EXPOSURE,
					DEVICE_ERROR_HW | TEL_PARKED,
					"park command finished with error");
			else
				maskState (TEL_MASK_MOVING | BOP_EXPOSURE, TEL_PARKED,
					"park command finished without error");
			if (move_connection)
			{
				sendInfo (move_connection);
			}
			setIdleInfoInterval (60);
		}
	}
}

int Telescope::idle ()
{
	checkMoves ();
	return Rts2Device::idle ();
}

void Telescope::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_COP_SYNCED:
			maskState (TEL_MASK_COP, TEL_NO_WAIT_COP);
			break;
	}
	Rts2Device::postEvent (event);
}

int Telescope::willConnect (Rts2Address * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_COPULA)
		return 1;
	return Rts2Device::willConnect (in_addr);
}

Rts2DevClient *Telescope::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_COPULA:
			return new Rts2DevClientCupolaTeld (conn);
	}
	return Rts2Device::createOtherType (conn, other_device_type);
}

int Telescope::changeMasterState (int new_state)
{
	// park us during day..
	if (((new_state & SERVERD_STATUS_MASK) == SERVERD_DAY)
		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF)
		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF)
		|| ((new_state & SERVERD_STANDBY_MASK) && standbyPark))
	{
		if ((getState () & TEL_MASK_MOVING) == 0)
			startPark (NULL);
	}

	if (blockOnStandby->getValueBool () == true)
	{
		if ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF
		  || (new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF
		  || (new_state & SERVERD_STANDBY_MASK))
			blockMove->setValueBool (true);
		else
			blockMove->setValueBool (false);
	}

	return Rts2Device::changeMasterState (new_state);
}

void Telescope::getTelAltAz (struct ln_hrz_posn *hrz)
{
	struct ln_equ_posn telpos;
	struct ln_lnlat_posn observer;

	telpos.ra = telRaDec->getRa ();
	telpos.dec = telRaDec->getDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	ln_get_hrz_from_equ_sidereal_time (&telpos, &observer, ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()), hrz);
}

double Telescope::estimateTargetTime ()
{
	// most of mounts move at 2 degrees per second.
	return getTargetDistance () / 2.0;
}

int Telescope::info ()
{
	struct ln_hrz_posn hrz;
	// calculate alt+az
	getTelAltAz (&hrz);
	telAltAz->setValueAltAz (hrz.alt, hrz.az);

	if (telFlip->getValueInteger ())
		rotang->setValueDouble (ln_range_degrees (defaultRotang + 180.0));
	else
		rotang->setValueDouble (defaultRotang);

	// fill in airmass, ha and lst
	airmass->setValueDouble (ln_get_airmass (telAltAz->getAlt (), 750));
	lst->setValueDouble (getLstDeg (ln_get_julian_from_sys ()));
	hourAngle->setValueDouble (lst->getValueDouble () - telRaDec->getRa ());
	targetDistance->setValueDouble (getTargetDistance ());

	// check if we aren't bellow hard horizon - if yes, stop worm..
	if (hardHorizon)
	{
		struct ln_hrz_posn hrpos;
		hrpos.alt = telAltAz->getAlt ();
		hrpos.az = telAltAz->getAz ();
		if (!hardHorizon->is_good (&hrpos))
		{
			stopWorm ();
		}
	}

	return Rts2Device::info ();
}

int Telescope::scriptEnds ()
{
	corrImgId->setValueInteger (0);
	woffsRaDec->setValueRaDec (0, 0);
	return Rts2Device::scriptEnds ();
}

void Telescope::applyCorrections (struct ln_equ_posn *pos, double JD)
{
	// apply all posible corrections
	if (calAberation->getValueBool () == true)
		applyAberation (pos, JD);
	if (calPrecession->getValueBool () == true)
		applyPrecession (pos, JD);
	if (calRefraction->getValueBool () == true)
		applyRefraction (pos, JD);
}

void Telescope::applyCorrections (double &tar_ra, double &tar_dec)
{
	struct ln_equ_posn pos;
	pos.ra = tar_ra;
	pos.dec = tar_dec;

	applyCorrections (&pos, ln_get_julian_from_sys ());

	tar_ra = pos.ra;
	tar_dec = pos.dec;
}

int Telescope::endMove ()
{
	LibnovaRaDec l_to (telRaDec->getRa (), telRaDec->getDec ());
	LibnovaRaDec l_tar (tarRaDec->getRa (), tarRaDec->getDec ());
	LibnovaRaDec l_telTar (telTargetRaDec->getRa (), telTargetRaDec->getDec ());

	logStream (MESSAGE_INFO)
		<< "moved to " << l_to
		<< " requested " << l_telTar
		<< " target " << l_tar
		<< sendLog;
	return 0;
}

int Telescope::startResyncMove (Rts2Conn * conn, bool onlyCorrect)
{
	int ret;

	struct ln_equ_posn pos;

	// if object was not specified, do not move
	if (isnan (oriRaDec->getRa ()) || isnan (oriRaDec->getDec ()))
		return -1;

	// object changed from last call to startResyncMove
	if (oriRaDec->wasChanged ())
	{
		incMoveNum ();
	}
	// if some value is waiting to be applied..
	else if (wcorrRaDec->wasChanged ())
	{
		corrRaDec->incValueRaDec (wcorrRaDec->getRa (), wcorrRaDec->getDec ());
		corrImgId->setValueInteger (wCorrImgId->getValueInteger ());
	}

	if (woffsRaDec->wasChanged ())
	{
		offsRaDec->setValueRaDec (woffsRaDec->getRa (), woffsRaDec->getDec ());
	}

	LibnovaRaDec l_obj (oriRaDec->getRa (), oriRaDec->getDec ());

	// first apply offset
	pos.ra = ln_range_degrees (oriRaDec->getRa () + offsRaDec->getRa ());
	pos.dec = oriRaDec->getDec () + offsRaDec->getDec ();

	objRaDec->setValueRaDec (pos.ra, pos.dec);
	sendValueAll (objRaDec);

	// apply corrections
	double JD = ln_get_julian_from_sys ();

	applyCorrections (&pos, JD);

	LibnovaRaDec syncTo (&pos);
	LibnovaRaDec syncFrom (telRaDec->getRa (), telRaDec->getDec ());

	// now we have target position, which can be feeded to telescope
	tarRaDec->setValueRaDec (pos.ra, pos.dec);

	// calculate target after corrections
	pos.ra = ln_range_degrees (pos.ra - corrRaDec->getRa ());
	pos.dec = pos.dec - corrRaDec->getDec ();
	telTargetRaDec->setValueRaDec (pos.ra, pos.dec);
	modelRaDec->setValueRaDec (0, 0);

	moveInfoCount = 0;

	if (hardHorizon)
	{
		struct ln_hrz_posn hrpos;
		getTelTargetAltAz (&hrpos, JD);
		if (!hardHorizon->is_good (&hrpos))
		{
			logStream (MESSAGE_ERROR) << "target is not accesible from this telescope" << sendLog;
			maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE, TEL_NOT_CORRECTING | TEL_OBSERVING, "cannot perform move");
			if (conn)
				conn->sendCommandEnd (DEVDEM_E_HW, "unaccesible target");
			return -1;
		}
	}

	if (blockMove->getValueBool () == true)
	{
		logStream (MESSAGE_ERROR) << "Telescope move blocked" << sendLog;
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "telescope move blocked");
		return -1;
	}

	ret = startResync ();
	if (ret)
	{
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot move to location");
		return ret;
	}

	targetStarted->setNow ();
	targetReached->setValueDouble (targetStarted->getValueDouble () + estimateTargetTime ());

	infoAll ();

	tarRaDec->resetValueChanged ();
	telTargetRaDec->resetValueChanged ();
	oriRaDec->resetValueChanged ();
	offsRaDec->resetValueChanged ();
	corrRaDec->resetValueChanged ();

	if (woffsRaDec->wasChanged ())
	{
		woffsRaDec->resetValueChanged ();
	}

	if (wcorrRaDec->wasChanged ())
	{
		wcorrRaDec->setValueRaDec (0, 0);
		wcorrRaDec->resetValueChanged ();
	}

	if (onlyCorrect)
	{
		LibnovaDegDist c_ra (corrRaDec->getRa ());
		LibnovaDegDist c_dec (corrRaDec->getDec ());

		logStream (MESSAGE_INFO) << "correcting to " << syncTo
			<< " from " << syncFrom
			<< " distances " << c_ra << " " << c_dec << sendLog;

		maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | TEL_MASK_NEED_STOP | BOP_EXPOSURE,
			TEL_CORRECTING | TEL_MOVING | BOP_EXPOSURE, "correction move started");
	}
	else
	{
		if (getState () & TEL_MASK_CORRECTING)
		{
			maskState (TEL_MASK_MOVING | TEL_MASK_CORRECTING, 0, "correcting interrupted by move");
		}
		logStream (MESSAGE_INFO) << "moving to " << syncTo << " from " << syncFrom << sendLog;
		maskState (TEL_MASK_MOVING | TEL_MASK_CORRECTING | TEL_MASK_NEED_STOP | BOP_EXPOSURE, TEL_MOVING | BOP_EXPOSURE, "move started");
	}
	move_connection = conn;

	setIdleInfoInterval (0.5);

	return ret;
}

int Telescope::setTo (Rts2Conn * conn, double set_ra, double set_dec)
{
	int ret;
	ret = setTo (set_ra, set_dec);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set to");
	return ret;
}

int Telescope::startPark (Rts2Conn * conn)
{
	if (blockMove->getValueBool () == true)
	{
		logStream (MESSAGE_ERROR) << "Telescope parking blocked" << sendLog;
		return -1;
	}
	int ret;
	ret = startPark ();
	if (ret)
	{
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot park");
	}
	else
	{
		tarRaDec->resetValueChanged ();
		telTargetRaDec->resetValueChanged ();
		oriRaDec->resetValueChanged ();
		offsRaDec->resetValueChanged ();
		corrRaDec->resetValueChanged ();

		incMoveNum ();
		setParkTimeNow ();
		maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_PARKING,
			"parking started");
	}

	setIdleInfoInterval (0.5);

	return ret;
}

int Telescope::getFlip ()
{
	int ret;
	ret = info ();
	if (ret)
		return ret;
	return telFlip->getValueInteger ();
}

void Telescope::signaledHUP ()
{
	int ret;
	if (modelFile)
	{
		delete model;
		model = new rts2telmodel::Model (this, modelFile);
		ret = model->load ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Failed to reload model from file " <<
				modelFile << sendLog;
			delete model;
			model = NULL;
		}
		else
		{
			logStream (MESSAGE_INFO) << "Reloaded model from file " <<
				modelFile << sendLog;
		}
	}
	Rts2Device::signaledHUP ();
}

int Telescope::commandAuthorized (Rts2Conn * conn)
{
	double obj_ra;
	double obj_dec;

	int ret;

	if (conn->isCommand ("move"))
	{
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		modelOn ();
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, false);
	}
	else if (conn->isCommand ("move_not_model"))
	{
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		modelOff ();
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, false);
	}
	else if (conn->isCommand ("resync"))
	{
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, false);
	}
	else if (conn->isCommand ("fixed"))
	{
		// tar_ra hold HA (Hour Angle)
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		/*		modelOff ();
				// currently don't know how to handle that */
		return -2;
	}
	else if (conn->isCommand ("setto"))
	{
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		modelOn ();
		return setTo (conn, obj_ra, obj_dec);
	}
	else if (conn->isCommand ("correct"))
	{
		int cor_mark;
		int corr_img;
		int img_id;
		double total_cor_ra;
		double total_cor_dec;
		double pos_err;
		if (conn->paramNextInteger (&cor_mark)
			|| conn->paramNextInteger (&corr_img)
			|| conn->paramNextInteger (&img_id)
			|| conn->paramNextDouble (&total_cor_ra)
			|| conn->paramNextDouble (&total_cor_dec)
			|| conn->paramNextDouble (&pos_err)
			|| !conn->paramEnd ())
			return -2;
		if (cor_mark == moveNum->getValueInteger ()
			&& corr_img == corrImgId->getValueInteger ()
			&& img_id > wCorrImgId->getValueInteger ())
		{
			if (pos_err < ignoreCorrection->getValueDouble ())
			{
				conn->sendCommandEnd (DEVDEM_E_IGNORE, "ignoring correction as is too smallk");
				return -1;
			}

			wcorrRaDec->setValueRaDec (total_cor_ra, total_cor_dec);

			posErr->setValueDouble (pos_err);
			sendValueAll (posErr);

			wCorrImgId->setValueInteger (img_id);
			sendValueAll (wCorrImgId);

			if (pos_err < smallCorrection->getValueDouble ())
				return startResyncMove (conn, true);
			sendValueAll (wcorrRaDec);
			// else set BOP_EXPOSURE, and wait for result of statusUpdate call
			maskState (BOP_EXPOSURE, BOP_EXPOSURE, "blocking exposure for correction");
			// correction was accepted, will be carried once it will be possible
			return 0;
		}
		conn->sendCommandEnd (DEVDEM_E_IGNORE, "ignoring correction as it is for incorrect movement mark");
		return -1;
	}
	else if (conn->isCommand ("park"))
	{
		if (!conn->paramEnd ())
			return -2;
		modelOn ();
		return startPark (conn);
	}
	else if (conn->isCommand ("change"))
	{
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		offsRaDec->incValueRaDec (obj_ra, obj_dec);
		woffsRaDec->incValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, true);
	}
	else if (conn->isCommand ("save_model"))
	{
		ret = saveModel ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot save model");
		}
		return ret;
	}
	else if (conn->isCommand ("load_model"))
	{
		ret = loadModel ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot load model");
		}
		return ret;
	}
	else if (conn->isCommand ("worm_stop"))
	{
		ret = stopWorm ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot stop worm");
		}
		return ret;
	}
	else if (conn->isCommand ("worm_start"))
	{
		ret = startWorm ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot start worm");
		}
		return ret;
	}
	else if (conn->isCommand ("reset"))
	{
		ret = resetMount ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot reset mount");
		}
		return ret;
	}
	return Rts2Device::commandAuthorized (conn);
}

void Telescope::setFullBopState (int new_state)
{
	Rts2Device::setFullBopState (new_state);
	if ((woffsRaDec->wasChanged () || wcorrRaDec->wasChanged ()) && !(new_state & BOP_TEL_MOVE))
		startResyncMove (NULL, true);
}
