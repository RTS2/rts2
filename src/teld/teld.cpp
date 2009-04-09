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

#include "telescope.h"
#include "rts2devclicop.h"

#include "model/telmodel.h"

#define OPT_BLOCK_ON_STANDBY  OPT_LOCAL + 117
#define OPT_HORIZON           OPT_LOCAL + 118

Rts2DevTelescope::Rts2DevTelescope (int in_argc, char **in_argv):
Rts2Device (in_argc, in_argv, DEVICE_TYPE_MOUNT, "T0")
{
	for (int i = 0; i < 4; i++)
	{
		timerclear (dir_timeouts + i);
	}

	createValue (pointingModel, "pointing", "pointing model (equ, alt-az, ...)", false, 0, 0, true);
	pointingModel->addSelVal ("EQU");
	pointingModel->addSelVal ("ALT-AZ");

	createConstValue (telLatitude, "LATITUDE", "observatory latitude", true);
	createConstValue (telLongitude, "LONGITUD", "observatory longitude", true);
	createConstValue (telAltitude, "ALTITUDE", "observatory altitude", true);

	// object
	createValue (objRaDec, "OBJ", "object position (J2000)", true);
	// users offset
	createValue (offsetRaDec, "OFFS", "object offset", true, RTS2_DT_DEGREES, 0, true);
	offsetRaDec->setValueRaDec (0, 0);

	createValue (tarRaDec, "TAR", "target position (J2000)", true);

	createValue (corrRaDec, "CORR_", "correction from closed loop", true, RTS2_DT_DEGREES,
		0, true);
	corrRaDec->setValueRaDec (0, 0);

	createValue (waitingCorrRaDec, "wcorr", "corrections which waits for being applied",
		false, RTS2_DT_DEGREES, 0, true);
	waitingCorrRaDec->setValueRaDec (0, 0);

	createValue (wCorrImgId, "wcorr_img", "Image id waiting for correction",
		false, 0, 0, true);

	// position error
	createValue (posErr, "pos_err", "error in degrees", false, RTS2_DT_DEG_DIST);

	// target + model + corrections = sends to tel ... TEL (read from sensors, if possible)
	createValue (telRaDec, "TEL", "mount position (read from sensors)", true);

	createValue (telAltAz, "TEL_", "horizontal telescope coordinates", true);

	createValue (mountParkTime, "PARKTIME", "Time of last mount park");

	createValue (blockMove, "block_move", "if true, any software movement of the telescope is blocked", false);
	blockMove->setValueBool (false);

	createValue (blockOnStandby, "block_on_standby", "Block telescope movement if switched to standby/off mode. Enable it if switched back to on.", false);
	blockOnStandby->setValueBool (false);

	createValue (airmass, "AIRMASS", "Airmass of target location");
	createValue (hourAngle, "HA", "Location hour angle", true, RTS2_DT_RA);
	createValue (lst, "LST", "Local Sidereal Time", true, RTS2_DT_RA);

	createValue (moveNum, "MOVE_NUM", "number of movements performed by the driver; used in corrections for synchronization", true);
	moveNum->setValueInteger (0);

	createValue (corrImgId, "CORR_IMG", "ID of last image used for correction", true);
	corrImgId->setValueInteger (0);

	defaultRotang = 0;
	createValue (rotang, "MNT_ROTA", "mount rotang", true, RTS2_DT_ROTANG);

	move_connection = NULL;

	createValue (ignoreCorrection, "ignore_correction", "corrections below this value will be ignored", false, RTS2_DT_DEG_DIST, 0, true);
	ignoreCorrection->setValueDouble (0);

	createValue (smallCorrection, "small_correction", "correction bellow this value will be considered as small",
		false, RTS2_DT_DEG_DIST);
	smallCorrection->setValueDouble (0);

	createValue (modelLimit, "model_limit", "model separation limit", false, RTS2_DT_DEG_DIST);
	modelLimit->setValueDouble (5.0);

	createValue (telFov, "telescope_fov", "telescope field of view", false, RTS2_DT_DEG_DIST);
	telFov->setValueDouble (180.0);

	createValue (telFlip, "MNT_FLIP", "telescope flip");

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

	// default is to aply model corrections
	createValue (correctionsMask, "RTS_COR", "RTS2 corrections bitmask", true);
	correctionsMask->setValueInteger (COR_MODEL);

	// send telescope position every 60 seconds
	setIdleInfoInterval (60);

	moveInfoCount = 0;
	moveInfoMax = 100;
}


Rts2DevTelescope::~Rts2DevTelescope (void)
{
	delete model;
}


double
Rts2DevTelescope::getLocSidTime (double JD)
{
	double ret;
	ret = ln_get_apparent_sidereal_time (JD) * 15.0 + telLongitude->getValueDouble ();
	return ln_range_degrees (ret) / 15.0;
}


int
Rts2DevTelescope::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'm':
			modelFile = optarg;
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


void
Rts2DevTelescope::calculateCorrAltAz ()
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


double
Rts2DevTelescope::getCorrZd ()
{
	if (corrRaDec->getRa () == 0 && corrRaDec->getDec () == 0)
		return 0;

	calculateCorrAltAz ();

	return corrAltAz.alt - tarAltAz.alt;
}


double
Rts2DevTelescope::getCorrAz ()
{
	if (corrRaDec->getRa () == 0 && corrRaDec->getDec () == 0)
		return 0;

	calculateCorrAltAz ();

	return tarAltAz.az - corrAltAz.az;
}


double
Rts2DevTelescope::getTargetDistance ()
{
	struct ln_equ_posn tar,tel;
	getTarget (&tar);
	tar.ra += corrRaDec->getRa ();
	tar.dec += corrRaDec->getDec ();
	getTelRaDec (&tel);

	if (isnan(tar.ra) || isnan(tar.dec) || isnan(tel.ra) || isnan(tel.dec))
		return -1;

	return ln_get_angular_separation (&tel, &tar);
}


void
Rts2DevTelescope::getTargetAltAz (struct ln_hrz_posn *hrz)
{
	getTargetAltAz (hrz, ln_get_julian_from_sys ());
}


void
Rts2DevTelescope::getTargetAltAz (struct ln_hrz_posn *hrz, double jd)
{
	struct ln_equ_posn tar;
	getTarget (&tar);
	struct ln_lnlat_posn observer;
	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();
	ln_get_hrz_from_equ (&tar, &observer, jd, hrz);
}


double
Rts2DevTelescope::getLstDeg (double JD)
{
	return ln_range_degrees (15 * ln_get_apparent_sidereal_time (JD) +
		telLongitude->getValueDouble ());
}


int
Rts2DevTelescope::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == modelLimit
		|| old_value == telFov
		|| old_value == rotang
		|| old_value == smallCorrection
		|| old_value == ignoreCorrection
		|| old_value == correctionsMask
		|| old_value == blockMove
		|| old_value == blockOnStandby)
	{
		return 0;
	}
	if (old_value == objRaDec
		|| old_value == offsetRaDec
		|| old_value == corrRaDec
		|| old_value == waitingCorrRaDec
		|| old_value == corrImgId
		|| old_value == wCorrImgId)
	{
		return 0;
	}
	return Rts2Device::setValue (old_value, new_value);
}


void
Rts2DevTelescope::valueChanged (Rts2Value * changed_value)
{
	if (changed_value == objRaDec
		|| changed_value == offsetRaDec
		|| changed_value == corrRaDec)
	{
		startResyncMove (NULL, false);
	}
	Rts2Device::valueChanged (changed_value);
}


void
Rts2DevTelescope::applyAberation (struct ln_equ_posn *pos, double JD)
{
	ln_get_equ_aber (pos, JD, pos);
}


void
Rts2DevTelescope::applyPrecession (struct ln_equ_posn *pos, double JD)
{
	ln_get_equ_prec (pos, JD, pos);
}


void
Rts2DevTelescope::applyRefraction (struct ln_equ_posn *pos, double JD)
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


void
Rts2DevTelescope::incMoveNum ()
{
	// reset offsets
	offsetRaDec->setValueRaDec (0, 0);
	offsetRaDec->resetValueChanged ();

	corrRaDec->setValueRaDec (0, 0);
	corrRaDec->resetValueChanged ();

	waitingCorrRaDec->setValueRaDec (0, 0);
	waitingCorrRaDec->resetValueChanged ();

	moveNum->inc ();

	corrImgId->setValueInteger (0);
	wCorrImgId->setValueInteger (0);
}


void
Rts2DevTelescope::applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD)
{
	struct ln_equ_posn hadec;
	double ra;
	double ls;
	if (!model || !(correctionsMask->getValueInteger () & COR_MODEL))
	{
		model_change->ra = -1 * corrRaDec->getRa();
		model_change->dec = -1 * corrRaDec->getDec();

		pos->ra = ln_range_degrees (pos->ra + corrRaDec->getRa ());
		pos->dec = pos->dec + corrRaDec->getDec ();
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
		return;
	}

	logStream (MESSAGE_DEBUG)
		<< "Rts2DevTelescope::applyModel offsets ra: "
		<< model_change->ra << " dec: " << model_change->dec
		<< sendLog;

	model_change->ra -= corrRaDec->getRa();
	model_change->dec -= corrRaDec->getDec();

	pos->ra = ln_range_degrees (ra - corrRaDec->getRa ());
	pos->dec = hadec.dec - corrRaDec->getDec ();
}


int
Rts2DevTelescope::init ()
{
	int ret;
	ret = Rts2Device::init ();
	if (ret)
		return ret;

	if (modelFile)
	{
		model = new Rts2TelModel (this, modelFile);
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


int
Rts2DevTelescope::initValues ()
{
	int ret;
	ret = info ();
	if (ret)
		return ret;
	tarRaDec->setFromValue (telRaDec);
	objRaDec->setFromValue (telRaDec);

	return Rts2Device::initValues ();
}


void
Rts2DevTelescope::checkMoves ()
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
		}
	}
}


void
Rts2DevTelescope::checkGuiding ()
{
	struct timeval now;
	gettimeofday (&now, NULL);
	if (dir_timeouts[0].tv_sec > 0 && timercmp (&now, dir_timeouts + 0, >))
		stopGuide (DIR_NORTH);
	if (dir_timeouts[1].tv_sec > 0 && timercmp (&now, dir_timeouts + 1, >))
		stopGuide (DIR_EAST);
	if (dir_timeouts[2].tv_sec > 0 && timercmp (&now, dir_timeouts + 2, >))
		stopGuide (DIR_SOUTH);
	if (dir_timeouts[3].tv_sec > 0 && timercmp (&now, dir_timeouts + 3, >))
		stopGuide (DIR_WEST);
}


int
Rts2DevTelescope::idle ()
{
	checkMoves ();
	checkGuiding ();
	return Rts2Device::idle ();
}


void
Rts2DevTelescope::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_COP_SYNCED:
			maskState (TEL_MASK_COP, TEL_NO_WAIT_COP);
			break;
	}
	Rts2Device::postEvent (event);
}


int
Rts2DevTelescope::willConnect (Rts2Address * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_COPULA)
		return 1;
	return Rts2Device::willConnect (in_addr);
}


Rts2DevClient *
Rts2DevTelescope::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_COPULA:
			return new Rts2DevClientCupolaTeld (conn);
	}
	return Rts2Device::createOtherType (conn, other_device_type);
}


int
Rts2DevTelescope::changeMasterState (int new_state)
{
	if (blockOnStandby->getValueBool () == true)
	{
		if ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF
		  || (new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF
		  || (new_state & SERVERD_STANDBY_MASK))
			blockMove->setValueBool (true);
		else
			blockMove->setValueBool (false);
	}

	// park us during day..
	if (((new_state & SERVERD_STATUS_MASK) == SERVERD_DAY)
		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF)
		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF)
		|| ((new_state & SERVERD_STANDBY_MASK) && standbyPark))
	{
		if ((getState () & TEL_MASK_MOVING) == 0)
			startPark (NULL);
	}
	return Rts2Device::changeMasterState (new_state);
}


int
Rts2DevTelescope::startGuide (char dir, double dir_dist)
{
	/*	struct timeval *tv;
		struct timeval tv_add;
		int state_dir;
		switch (dir)
		{
			case DIR_NORTH:
				tv = dir_timeouts + 0;
				state_dir = TEL_GUIDE_NORTH;
				break;
			case DIR_EAST:
				tv = dir_timeouts + 1;
				state_dir = TEL_GUIDE_EAST;
				break;
			case DIR_SOUTH:
				tv = dir_timeouts + 2;
				state_dir = TEL_GUIDE_SOUTH;
				break;
			case DIR_WEST:
				tv = dir_timeouts + 3;
				state_dir = TEL_GUIDE_WEST;
				break;
			default:
				return -1;
		}
		double dir_timeout = (dir_dist / 15.0) * telGuidingSpeed->getValueDouble ();
		logStream (MESSAGE_INFO)
			<< "telescope startGuide dir "
			<< dir_dist << " timeout " << dir_timeout << sendLog;
		gettimeofday (&tv_add, NULL);
		tv_add.tv_sec = (int) (floor (dir_timeout));
		tv_add.tv_usec = (int) ((dir_timeout - tv_add.tv_sec) * USEC_SEC);
		timeradd (tv, &tv_add, tv);
		maskState (state_dir, state_dir, "started guiding"); */
	return -1;
}


int
Rts2DevTelescope::stopGuide (char dir)
{
	/*	int state_dir;
		switch (dir)
		{
			case DIR_NORTH:
				dir_timeouts[0].tv_sec = 0;
				state_dir = TEL_GUIDE_NORTH;
				break;
			case DIR_EAST:
				dir_timeouts[1].tv_sec = 0;
				state_dir = TEL_GUIDE_EAST;
				break;
			case DIR_SOUTH:
				dir_timeouts[2].tv_sec = 0;
				state_dir = TEL_GUIDE_SOUTH;
				break;
			case DIR_WEST:
				dir_timeouts[3].tv_sec = 0;
				state_dir = TEL_GUIDE_WEST;
				break;
			default:
				return -1;
		}
		logStream (MESSAGE_INFO) << "telescope stopGuide dir " << dir << sendLog;
		maskState (state_dir, TEL_NOGUIDE, "guiding ended"); */
	return -1;
}


int
Rts2DevTelescope::stopGuideAll ()
{
	logStream (MESSAGE_INFO) << "telescope stopGuideAll" << sendLog;
	maskState (TEL_GUIDE_MASK, TEL_NOGUIDE, "guiding stoped");
	return 0;
}


void
Rts2DevTelescope::getAltAz ()
{
	struct ln_equ_posn telpos;
	struct ln_lnlat_posn observer;
	struct ln_hrz_posn hrz;

	telpos.ra = telRaDec->getRa ();
	telpos.dec = telRaDec->getDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	ln_get_hrz_from_equ_sidereal_time (&telpos, &observer,
		ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()),
		&hrz);

	telAltAz->setValueAltAz (hrz.alt, hrz.az);
}


int
Rts2DevTelescope::info ()
{
	// calculate alt+az
	getAltAz ();

	if (telFlip->getValueInteger ())
		rotang->setValueDouble (ln_range_degrees (defaultRotang + 180.0));
	else
		rotang->setValueDouble (defaultRotang);

	// fill in airmass, ha and lst
	airmass->setValueDouble (ln_get_airmass (telAltAz->getAlt (), 750));
	lst->setValueDouble (getLstDeg (ln_get_julian_from_sys ()));
	hourAngle->setValueDouble (lst->getValueDouble () - telRaDec->getRa ());

	return Rts2Device::info ();
}


int
Rts2DevTelescope::scriptEnds ()
{
	corrImgId->setValueInteger (0);
	return Rts2Device::scriptEnds ();
}


void
Rts2DevTelescope::applyCorrections (struct ln_equ_posn *pos, double JD)
{
	// apply all posible corrections
	if (correctionsMask->getValueInteger () & COR_ABERATION)
		applyAberation (pos, JD);
	if (correctionsMask->getValueInteger () & COR_PRECESSION)
		applyPrecession (pos, JD);
	if (correctionsMask->getValueInteger () & COR_REFRACTION)
		applyRefraction (pos, JD);
}


void
Rts2DevTelescope::applyCorrections (double &tar_ra, double &tar_dec)
{
	struct ln_equ_posn pos;
	pos.ra = tar_ra;
	pos.dec = tar_dec;

	applyCorrections (&pos, ln_get_julian_from_sys ());

	tar_ra = pos.ra;
	tar_dec = pos.dec;
}


int
Rts2DevTelescope::endMove ()
{
	LibnovaRaDec l_to (telRaDec->getRa (), telRaDec->getDec ());
	LibnovaRaDec l_req (tarRaDec->getRa (), tarRaDec->getDec ());

	logStream (MESSAGE_INFO)
		<< "moved to " << l_to
		<< " requested " << l_req
		<< sendLog;
	return 0;
}


int
Rts2DevTelescope::startResyncMove (Rts2Conn * conn, bool onlyCorrect)
{
	int ret;

	struct ln_equ_posn pos;

	// if object was not specified, do not move
	if (isnan (objRaDec->getRa ()) || isnan (objRaDec->getDec ()))
		return -1;

	// object changed from last call to startResyncMove
	if (objRaDec->wasChanged ())
	{
		incMoveNum ();
	}
	// if some value is waiting to be applied..
	else if (waitingCorrRaDec->wasChanged ())
	{
		corrRaDec->incValueRaDec (waitingCorrRaDec->getRa (), waitingCorrRaDec->getDec ());

		waitingCorrRaDec->setValueRaDec (0, 0);
		waitingCorrRaDec->resetValueChanged ();

		corrImgId->setValueInteger (wCorrImgId->getValueInteger ());
	}

	LibnovaRaDec l_obj (objRaDec->getRa (), objRaDec->getDec ());

	// first apply offset
	pos.ra = ln_range_degrees (objRaDec->getRa () + offsetRaDec->getRa ());
	pos.dec = objRaDec->getDec () + offsetRaDec->getDec ();

	// apply corrections
	applyCorrections (&pos, ln_get_julian_from_sys ());

	LibnovaRaDec syncTo (&pos);
	LibnovaRaDec syncFrom (telRaDec->getRa (), telRaDec->getDec ());

	// now we have target position, which can be feeded to telescope
	tarRaDec->setValueRaDec (pos.ra, pos.dec);

	// calculate target after corrections
	pos.ra = ln_range_degrees (pos.ra - corrRaDec->getRa ());
	pos.dec = pos.dec - corrRaDec->getDec ();
	telRaDec->setValueRaDec (pos.ra, pos.dec);

	moveInfoCount = 0;

	if (hardHorizon)
	{
		struct ln_hrz_posn hrpos;
		getAltAz ();
		hrpos.az = telAltAz->getAz ();
		hrpos.alt = telAltAz->getAlt ();
		if (!hardHorizon->is_good (&hrpos))
		{
			logStream (MESSAGE_ERROR) << "target is not accesible from this telescope" << sendLog;
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

	ret = startMove ();
	if (ret)
	{
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot move to location");
		return ret;
	}

	infoAll ();

	objRaDec->resetValueChanged ();
	offsetRaDec->resetValueChanged ();
	corrRaDec->resetValueChanged ();

	if (onlyCorrect)
	{
		logStream (MESSAGE_INFO) << "correcting to " << syncTo << " from " << syncFrom << sendLog;
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
		maskState (TEL_MASK_MOVING | TEL_MASK_CORRECTING | TEL_MASK_NEED_STOP | BOP_EXPOSURE, TEL_MOVING | BOP_EXPOSURE,
			"move started");
	}
	move_connection = conn;

	return ret;
}


int
Rts2DevTelescope::setTo (Rts2Conn * conn, double set_ra, double set_dec)
{
	int ret;
	ret = setTo (set_ra, set_dec);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set to");
	return ret;
}


int
Rts2DevTelescope::startPark (Rts2Conn * conn)
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
		incMoveNum ();
		setParkTimeNow ();
		maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_PARKING,
			"parking started");
	}
	return ret;
}


int
Rts2DevTelescope::getFlip ()
{
	int ret;
	ret = info ();
	if (ret)
		return ret;
	return telFlip->getValueInteger ();
}


void
Rts2DevTelescope::signaledHUP ()
{
	int ret;
	if (modelFile)
	{
		delete model;
		model = new Rts2TelModel (this, modelFile);
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


int
Rts2DevTelescope::commandAuthorized (Rts2Conn * conn)
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
		objRaDec->setValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, false);
	}
	else if (conn->isCommand ("move_not_model"))
	{
		CHECK_PRIORITY;
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		modelOff ();
		objRaDec->setValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, false);
	}
	else if (conn->isCommand ("resync"))
	{
		if (conn->paramNextDouble (&obj_ra) || conn->paramNextDouble (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		objRaDec->setValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, false);
	}
	else if (conn->isCommand ("fixed"))
	{
		CHECK_PRIORITY;
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
		CHECK_PRIORITY;
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

			waitingCorrRaDec->setValueRaDec (total_cor_ra, total_cor_dec);

			posErr->setValueDouble (pos_err);
			sendValueAll (posErr);

			wCorrImgId->setValueInteger (img_id);
			sendValueAll (wCorrImgId);

			if (pos_err < smallCorrection->getValueDouble ())
				return startResyncMove (conn, true);
			sendValueAll (waitingCorrRaDec);
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
		offsetRaDec->incValueRaDec (obj_ra, obj_dec);
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


void
Rts2DevTelescope::setFullBopState (int new_state)
{
	Rts2Device::setFullBopState (new_state);
	if (waitingCorrRaDec->wasChanged () && !(new_state & BOP_TEL_MOVE))
		startResyncMove (NULL, true);
}
