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

#include "device.h"
#include "libnova_cpp.h"

#include "teld.h"
#include "clicupola.h"

#include "telmodel.h"

#define OPT_BLOCK_ON_STANDBY  OPT_LOCAL + 117
#define OPT_HORIZON           OPT_LOCAL + 118
#define OPT_CORRECTION        OPT_LOCAL + 119
#define OPT_WCS_MULTI         OPT_LOCAL + 120

#define EVENT_TELD_MPEC_REFRESH  RTS2_LOCAL_EVENT + 560

using namespace rts2teld;

Telescope::Telescope (int in_argc, char **in_argv, bool diffTrack, bool hasTracking):rts2core::Device (in_argc, in_argv, DEVICE_TYPE_MOUNT, "T0")
{
	for (int i = 0; i < 4; i++)
	{
		timerclear (dir_timeouts + i);
	}

	raGuide = decGuide = NULL;

	// object
	createValue (oriRaDec, "ORI", "original position (J2000)", true, RTS2_VALUE_WRITABLE);
	// users offset
	createValue (offsRaDec, "OFFS", "object offset", true, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	offsRaDec->setValueRaDec (0, 0);

	createValue (woffsRaDec, "woffs", "offsets waiting to be applied", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	woffsRaDec->setValueRaDec (0, 0);
	woffsRaDec->resetValueChanged ();

	if (diffTrack)
	{
		createValue (diffTrackRaDec, "DSPEED", "[deg/hour] differential tracking speed", true, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
		diffTrackRaDec->setValueRaDec (0, 0);
	}
	else
	{
		diffTrackRaDec = NULL;
	}

	if (hasTracking)
	{
		createValue (tracking, "TRACKING", "telescope tracking", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	}
	else
	{
		tracking = NULL;
	}


	createValue (objRaDec, "OBJ", "telescope FOV center position (J200) - with offsets applied", true);

	createValue (tarRaDec, "TAR", "target position (J2000)", true);

	createValue (corrRaDec, "CORR_", "correction from closed loop", true, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	corrRaDec->setValueRaDec (0, 0);

	createValue (wcorrRaDec, "wcorr", "corrections which waits for being applied", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	wcorrRaDec->setValueRaDec (0, 0);
	wcorrRaDec->resetValueChanged ();

	createValue (total_offsets, "total_offsets", "[deg] OFFS - corr", false, RTS2_DT_DEG_DIST_180);

	createValue (wCorrImgId, "wcorr_img", "Image id waiting for correction", false, RTS2_VALUE_WRITABLE, 0);

	// position error
	createValue (posErr, "pos_err", "error in degrees", false, RTS2_DT_DEG_DIST_180);

	createValue (telTargetRaDec, "tel_target", "target RA DEC telescope coordinates - one feeded to TCS", false);

	createValue (modelRaDec, "MO_RTS2", "[deg] RTS2 model offsets", true, RTS2_DT_DEGREES, 0);
	modelRaDec->setValueRaDec (0, 0);

	wcs_crval1 = wcs_crval2 = NULL;

	// target + model + corrections = sends to tel ... TEL (read from sensors, if possible)
	createValue (telRaDec, "TEL", "mount position (read from sensors)", true);

	createValue (telAltAz, "TEL_", "horizontal telescope coordinates", true, RTS2_VALUE_WRITABLE);

	createValue (pointingModel, "pointing", "pointing model (equ, alt-az, ...)", false, 0, 0);
	pointingModel->addSelVal ("EQU");
	pointingModel->addSelVal ("ALT-AZ");
	pointingModel->addSelVal ("ALT-ALT");

	createValue (mpec, "mpec_target", "MPEC string (used for target calculation, if set", false, RTS2_VALUE_WRITABLE);
	createValue (mpec_refresh, "mpec_refresh", "refresh MPEC ra_diff and dec_diff every mpec_refresh seconds", false, RTS2_VALUE_WRITABLE);
	mpec_refresh->setValueDouble (3600);

	createValue (mpec_angle, "mpec_angle", "timespan from which MPEC ra_diff and dec_diff will be computed", false, RTS2_VALUE_WRITABLE);
	mpec_angle->setValueDouble (3600);

	if (diffTrack)
	{
		createValue (diffRaDec, "mpec_dif", "[degrees/hour] calculated MPEC differential tracking", true, RTS2_DT_DEGREES, TEL_MOVING);
		diffRaDec->setValueRaDec (0, 0);
	}
	else
	{
		diffRaDec = NULL;
	}

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

	targetDistance->setValueDouble (NAN);
	targetStarted->setValueDouble (NAN);
	targetReached->setValueDouble (NAN);

	createValue (moveNum, "MOVE_NUM", "number of movements performed by the driver; used in corrections for synchronization", true);
	moveNum->setValueInteger (0);

	createValue (corrImgId, "CORR_IMG", "ID of last image used for correction", true, RTS2_VALUE_WRITABLE);
	corrImgId->setValueInteger (0);

	defaultRotang = 0;
	createValue (rotang, "MNT_ROTA", "mount rotang", true, RTS2_DT_WCS_ROTANG | RTS2_VALUE_WRITABLE);

	move_connection = NULL;

	createValue (ignoreCorrection, "ignore_correction", "corrections below this value will be ignored", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
	ignoreCorrection->setValueDouble (0);

	createValue (defIgnoreCorrection, "def_ignore_cor", "default ignore correction. ignore_correction is changed to this value at the end of the script", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
	defIgnoreCorrection->setValueDouble (0);

	createValue (smallCorrection, "small_correction", "correction bellow this value will be considered as small", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
	smallCorrection->setValueDouble (0);

	createValue (correctionLimit, "correction_limit", "corrections above this limit will be ignored", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
	correctionLimit->setValueDouble (5.0);

	createValue (modelLimit, "model_limit", "model separation limit", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
	modelLimit->setValueDouble (5.0);

	createValue (telFov, "telescope_fov", "telescope field of view", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
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

	createValue (cupolas, "cupolas", "Cupola(s) connected to telescope. They should sync as telescope moves.", false);

	modelFile = NULL;
	model = NULL;

	createValue (standbyPark, "standby_park", "Park telescope when switching to standby", false, RTS2_VALUE_WRITABLE);
	standbyPark->addSelVal ("no");
	standbyPark->addSelVal ("daytime only");
	standbyPark->addSelVal ("nightime");

	standbyPark->setValueInteger (0);

	horizonFile = NULL;
	hardHorizon = NULL;

	wcs_multi = '!';

	addOption ('m', NULL, 1, "name of file holding model parameters, calculated by T-Point");
	addOption ('l', NULL, 1, "separation limit (corrections above that number in degrees will be ignored)");
	addOption ('g', NULL, 1, "minimal good separation. Correction above that number will be aplied immediately. Default to 180 deg");

	addOption ('c', NULL, 1, "minimal value for corrections. Corrections bellow that value will be rejected.");

	addOption ('s', NULL, 1, "park when switched to standby - 1 only at day, 2 even at night");
	addOption (OPT_BLOCK_ON_STANDBY, "block-on-standby", 0, "block telescope movement when switching to standby");

	addOption ('r', NULL, 1, "telescope rotang");
	addOption (OPT_HORIZON, "horizon", 1, "telescope hard horizon");
	addOption (OPT_CORRECTION, "max-correction", 1, "correction limit (in arcsec)");
	addOption (OPT_WCS_MULTI, "wcs-multi", 1, "letter for multiple WCS (A-Z,-)");

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

void Telescope::createRaGuide ()
{
	createValue (raGuide, "ra_guide", "RA guiding status", false, RTS2_VALUE_WRITABLE);
	raGuide->addSelVal ("NONE");
	raGuide->addSelVal ("MINUS");
	raGuide->addSelVal ("PLUS");
}

void Telescope::createDecGuide ()
{
	createValue (decGuide, "dec_guide", "DEC guiding status", false, RTS2_VALUE_WRITABLE);
	decGuide->addSelVal ("NONE");
	decGuide->addSelVal ("MINUS");
	decGuide->addSelVal ("PLUS");
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
			defIgnoreCorrection->setValueCharArr (optarg);
			ignoreCorrection->setValueDouble (defIgnoreCorrection->getValueDouble ());
			break;
		case OPT_BLOCK_ON_STANDBY:
			blockOnStandby->setValueBool (true);
			break;
		case 's':
			standbyPark->setValueCharArr (optarg);
			break;
		case 'r':
			defaultRotang = atof (optarg);
			rotang->setValueDouble (defaultRotang);
			break;
		case OPT_HORIZON:
			horizonFile = optarg;
			break;
		case OPT_CORRECTION:
			correctionLimit->setValueCharArr (optarg);
			break;
		case OPT_WCS_MULTI:
			if (strlen (optarg) != 1 || ! (optarg[0] == '-' || (optarg[0] >= 'A' || optarg[0] <= 'Z')))
			{
				std::cerr << "invalid --wcs-multi option " << optarg << std::endl;
				return -1;
			}
			if (optarg[0] == '-')
				wcs_multi = '\0';
			else
				wcs_multi = optarg[0];
			break;
		default:
			return rts2core::Device::processOption (in_opt);
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

	equ_corr.ra = equ_target.ra;
	equ_corr.dec = equ_target.dec;

	applyCorrRaDec (&equ_corr, true, true);

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
	return ln_range_degrees (15. * ln_get_apparent_sidereal_time (JD) +
		telLongitude->getValueDouble ());
}

int Telescope::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == mpec)
	{
		std::string desc;
		if (LibnovaEllFromMPC (&mpec_orbit, desc, new_value->getValue ()))
			return -2;
	}
	else if (old_value == diffTrackRaDec)
	{
	  	setDiffTrack (((rts2core::ValueRaDec *)new_value)->getRa (), ((rts2core::ValueRaDec *)new_value)->getDec ());
		return 0;
	}
	return rts2core::Device::setValue (old_value, new_value);
}

void Telescope::valueChanged (rts2core::Value * changed_value)
{
	if (changed_value == woffsRaDec)
	{
		maskState (BOP_EXPOSURE, BOP_EXPOSURE, "blocking exposure for offsets");
	}
	if (changed_value == oriRaDec)
	{
		mpec->setValueString ("");
		startResyncMove (NULL, 0);
	}
	if (changed_value == mpec)
	{
		recalculateMpecDIffs ();
		startResyncMove (NULL, 0);
	}
	if (changed_value == offsRaDec || changed_value == corrRaDec)
	{
		startOffseting (changed_value);
	}
	if (changed_value == telAltAz)
	{
		moveAltAz ();
	}
	rts2core::Device::valueChanged (changed_value);
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
	if (diffTrackRaDec)
	{
		diffTrackRaDec->setValueRaDec (0, 0);
	  	setDiffTrack (0,0);
	}
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

	ignoreCorrection->setValueDouble (defIgnoreCorrection->getValueDouble ());

	corrImgId->setValueInteger (0);
	wCorrImgId->setValueInteger (0);
}

void Telescope::recalculateMpecDIffs ()
{
	if (mpec->getValueString ().length () <= 0)
		return;

	double mp_diff = mpec_angle->getValueDouble () / 86400.0;

	double JD = ln_get_julian_from_sys () - 1.5 * mp_diff;

	// get MPEC positions..
	struct ln_equ_posn pos[4];

	struct ln_lnlat_posn observer;
	struct ln_equ_posn parallax;
	observer.lat = telLatitude->getValueDouble ();
	observer.lng = telLongitude->getValueDouble ();

	int i;

	for (i = 0; i < 4; i++)
	{
		LibnovaCurrentFromOrbit (pos + i, &mpec_orbit, &observer, telAltitude->getValueDouble (), JD, &parallax);
		JD += mp_diff;
	}

	struct ln_equ_posn pos_diffs[3];

	for (i = 0; i < 3; i++)
	{
		pos_diffs[i].ra = pos[i].ra - pos[i+1].ra;
		pos_diffs[i].dec = pos[i].dec - pos[i+1].dec;
	}

	if (diffRaDec)
	{
		diffRaDec->setValueRaDec (pos_diffs[1].ra, pos_diffs[1].dec);
	}
}

int Telescope::applyCorrRaDec (struct ln_equ_posn *pos, bool invertRa, bool invertDec)
{
  	struct ln_equ_posn pos2;
	if (invertRa)
		pos2.ra = ln_range_degrees (pos->ra - corrRaDec->getRa ());
	else
		pos2.ra = ln_range_degrees (pos->ra + corrRaDec->getRa ());

	if (invertDec)
		pos2.dec = pos->dec - corrRaDec->getDec ();
	else
		pos2.dec = pos->dec + corrRaDec->getDec ();
	if (ln_get_angular_separation (&pos2, pos) > correctionLimit->getValueDouble ())
	{
		logStream (MESSAGE_WARNING) << "correction " << LibnovaDegDist (corrRaDec->getRa ()) << " " << LibnovaDegDist (corrRaDec->getDec ()) << " is above limit, ignoring it" << sendLog;
		return -1; 
	}
	pos->ra = pos2.ra;
	pos->dec = pos2.dec;
	return 0;
}

void Telescope::applyModel (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, int flip, double JD)
{
	struct ln_equ_posn hadec;
	double ra;
	double ls;
	double un_dec;
	// normalize correction to prevent runaway
	if (!model || calModel->getValueBool () == false)
	{
		if (applyCorrRaDec (pos) == 0)
		{
			model_change->ra = -1 * corrRaDec->getRa();
			model_change->dec = -1 * corrRaDec->getDec();
		}

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

		un_dec = hadec.dec;
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
		un_dec = hadec.dec;
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
		applyCorrRaDec (pos);
		telTargetRaDec->setValueRaDec (pos->ra, pos->dec);
		return;
	}

	logStream (MESSAGE_DEBUG)
		<< "Telescope::applyModel offsets ra: "
		<< model_change->ra << " dec: " << model_change->dec
		<< sendLog;

	modelRaDec->setValueRaDec (model_change->ra, model_change->dec);

	if (applyCorrRaDec (pos) == 0)
	{
		model_change->ra -= corrRaDec->getRa();
		model_change->dec -= corrRaDec->getDec();
	}

	telTargetRaDec->setValueRaDec (pos->ra, un_dec);
}

int Telescope::init ()
{
	int ret;
	ret = rts2core::Device::init ();
	if (ret)
		return ret;

	if (wcs_multi != '!')
	{
		createValue (wcs_crval1, multiWCS ("CRVAL1", wcs_multi), "first reference value", false, RTS2_DT_WCS_CRVAL1);
		createValue (wcs_crval2, multiWCS ("CRVAL2", wcs_multi), "second reference value", false, RTS2_DT_WCS_CRVAL2);
	}

	if (modelFile)
	{
		model = new rts2telmodel::Model (this, modelFile);
		ret = model->load ();
		if (ret)
			return ret;
		for (std::vector <rts2telmodel::ModelTerm *>::iterator iter = model->begin (); iter != model->end (); iter++)
			addValue (*iter, TEL_MOVING);
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

	if (wcs_crval1 && wcs_crval2)
	{
		wcs_crval1->setValueDouble (objRaDec->getRa ());
		wcs_crval2->setValueDouble (objRaDec->getDec ());
	}

	return rts2core::Device::initValues ();
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
			logStream (MESSAGE_INFO) << "parking of telescope finished" << sendLog;
			ret = endPark ();
			if (ret)
				maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING | BOP_EXPOSURE, DEVICE_ERROR_HW | TEL_PARKED, "park command finished with error");
			else
				maskState (TEL_MASK_MOVING | BOP_EXPOSURE, TEL_PARKED, "park command finished without error");
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
	return rts2core::Device::idle ();
}

void Telescope::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_CUP_SYNCED:
			maskState (TEL_MASK_CUP, TEL_NO_WAIT_CUP);
			break;
		case EVENT_CUP_ENDED:
			logStream (MESSAGE_INFO) << "removed " << ((ClientCupola *)(event->getArg ()))->getName () << sendLog;
			cupolas->remove (((ClientCupola *)(event->getArg ()))->getName ());
			break;
	}
	rts2core::Device::postEvent (event);
}

int Telescope::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_CUPOLA)
		return 1;
	return rts2core::Device::willConnect (in_addr);
}

rts2core::DevClient *Telescope::createOtherType (rts2core::Connection * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_CUPOLA:
			logStream (MESSAGE_INFO) << "connecting to cupula " << conn->getName () << sendLog;
			cupolas->addValue (std::string (conn->getName ()));
			return new ClientCupola (conn);
	}
	return rts2core::Device::createOtherType (conn, other_device_type);
}

void Telescope::changeMasterState (int old_state, int new_state)
{
	// stop when STOP is triggered
	if ((getState () & STOP_MASK) != STOP_EVERYTHING && (new_state & STOP_MASK) == STOP_EVERYTHING)
	{
		blockMove->setValueBool (true);
		sendValueAll (blockMove);
		stopMove ();
		logStream (MESSAGE_WARNING) << "stoped movement of the delescope, triggered by STOP_MASK change" << sendLog;
	}
	// enable movement when stop state was cleared
	else if ((old_state & STOP_MASK) == STOP_EVERYTHING && (new_state & STOP_MASK) != STOP_EVERYTHING)
	{
		blockMove->setValueBool (false);
		sendValueAll (blockMove);
		logStream (MESSAGE_DEBUG) << "reenabled movement as all stop states were cleared" << sendLog;
	}

	// park us during day..
	if ((((new_state & SERVERD_STATUS_MASK) == SERVERD_DAY)
		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF)
		|| ((new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF)
		|| (new_state & SERVERD_STANDBY_MASK)) && standbyPark->getValueInteger () != 0)
	{
		// ignore nighttime park request
		if (standbyPark->getValueInteger () == 2 || ! ((new_state & SERVERD_STATUS_MASK) == SERVERD_NIGHT || (new_state & SERVERD_STATUS_MASK) == SERVERD_DUSK || (new_state & SERVERD_STATUS_MASK) == SERVERD_DAWN))
		{
	  		if ((getState () & TEL_MASK_MOVING) != TEL_PARKED && (getState () & TEL_MASK_MOVING) != TEL_PARKING)
				startPark (NULL);
		}
	}

	if (blockOnStandby->getValueBool () == true)
	{
		if ((new_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF
		  || (new_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF
		  || (new_state & SERVERD_STANDBY_MASK))
			blockMove->setValueBool (true);
		// only set blockMove to false if switching from off/standy
		else if ((old_state & SERVERD_STATUS_MASK) == SERVERD_SOFT_OFF
		  || (old_state & SERVERD_STATUS_MASK) == SERVERD_HARD_OFF
		  || (old_state & SERVERD_STANDBY_MASK))
			blockMove->setValueBool (false);
	}

	rts2core::Device::changeMasterState (old_state, new_state);
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

	return rts2core::Device::info ();
}

int Telescope::scriptEnds ()
{
	corrImgId->setValueInteger (0);
	woffsRaDec->setValueRaDec (0, 0);
	return rts2core::Device::scriptEnds ();
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

void Telescope::startCupolaSync ()
{
	struct ln_equ_posn tar;
	getTarget (&tar);
	postEvent (new rts2core::Event (EVENT_CUP_START_SYNC, (void*) &tar));
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

int Telescope::startResyncMove (rts2core::Connection * conn, int correction)
{
	int ret;

	struct ln_equ_posn pos;

	// calculate from MPEC..
	if (mpec->getValueString ().length () > 0)
	{
		struct ln_lnlat_posn observer;
		struct ln_equ_posn parallax;
		observer.lat = telLatitude->getValueDouble ();
		observer.lng = telLongitude->getValueDouble ();
		LibnovaCurrentFromOrbit (&pos, &mpec_orbit, &observer, telAltitude->getValueDouble (), ln_get_julian_from_sys (), &parallax);

		oriRaDec->setValueRaDec (pos.ra, pos.dec);
	}

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

	// update total_offsets
	total_offsets->setValueRaDec (offsRaDec->getRa () - corrRaDec->getRa (), offsRaDec->getDec () - corrRaDec->getDec ());
	sendValueAll (total_offsets);

	LibnovaRaDec l_obj (oriRaDec->getRa (), oriRaDec->getDec ());

	// first apply offset
	applyOffsets (&pos);
	pos.ra = ln_range_degrees (pos.ra);

	objRaDec->setValueRaDec (pos.ra, pos.dec);
	sendValueAll (objRaDec);

	setCRVAL (pos.ra, pos.dec);

	// apply corrections
	double JD = ln_get_julian_from_sys ();

	applyCorrections (&pos, JD);

	LibnovaRaDec syncTo (&pos);
	LibnovaRaDec syncFrom (telRaDec->getRa (), telRaDec->getDec ());

	// now we have target position, which can be feeded to telescope
	tarRaDec->setValueRaDec (pos.ra, pos.dec);
	// calculate target after corrections
	applyCorrRaDec (&pos, true, true);

	telTargetRaDec->setValueRaDec (pos.ra, pos.dec);
	modelRaDec->setValueRaDec (0, 0);

	moveInfoCount = 0;

	if (hardHorizon)
	{
		struct ln_hrz_posn hrpos;
		getTelTargetAltAz (&hrpos, JD);
		if (!hardHorizon->is_good (&hrpos))
		{
			logStream (MESSAGE_ERROR) << "target is below hard horizon: alt az" << LibnovaHrz (&hrpos) << sendLog;
			maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE, TEL_NOT_CORRECTING | TEL_OBSERVING, "cannot perform move");
			if (conn)
				conn->sendCommandEnd (DEVDEM_E_HW, "unaccesible target");
			return -1;
		}
	}

	if (blockMove->getValueBool () == true)
	{
		logStream (MESSAGE_ERROR) << "telescope move blocked" << sendLog;
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "telescope move blocked");
		return -1;
	}

	if ((getState () & TEL_MASK_MOVING) == TEL_MOVING)
	{
		ret = stopMove ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "failure calling stopMove before starting actual movement" << sendLog;
			return ret;
		}
	}

	ret = startResync ();
	if (ret)
	{
		maskState (BOP_EXPOSURE, 0, "correction failed");
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot move to location");
		return ret;
	}

	// synchronize cupola
	startCupolaSync ();

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

	if (correction)
	{
		LibnovaDegDist c_ra (corrRaDec->getRa ());
		LibnovaDegDist c_dec (corrRaDec->getDec ());

		const char *cortype = "offseting and correcting";
	
		switch (correction)
		{
			case 1:
				cortype = "offseting";
				break;
			case 2:
				cortype = "correcting";
				break;
		}

		logStream (MESSAGE_INFO) << cortype << " to " << syncTo << " from " << syncFrom << " distances " << c_ra << " " << c_dec << sendLog;

		maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | TEL_MASK_NEED_STOP | BOP_EXPOSURE, TEL_CORRECTING | TEL_MOVING | BOP_EXPOSURE, "correction move started");
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

int Telescope::setTo (rts2core::Connection * conn, double set_ra, double set_dec)
{
	int ret;
	ret = setTo (set_ra, set_dec);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set to");
	sendValueAll (telRaDec);
	return ret;
}

int Telescope::setToPark (rts2core::Connection * conn)
{
	int ret;
	ret = setToPark ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set to park");
	sendValueAll (telRaDec);
	return ret;
}

int Telescope::startPark (rts2core::Connection * conn)
{
	if (blockMove->getValueBool () == true)
	{
		logStream (MESSAGE_ERROR) << "Telescope parking blocked" << sendLog;
		return -1;
	}
	int ret;
	if ((getState () & TEL_MASK_MOVING) == TEL_MOVING)
	{
		ret = stopMove ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "failure calling stopMove before starting parking" << sendLog;
			return ret;
		}
	}
	ret = startPark ();
	if (ret < 0)
	{
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot park");
		logStream (MESSAGE_ERROR) << "parking failed" << sendLog;
	}
	else
	{
	  	logStream (MESSAGE_INFO) << "parking telescope" << sendLog;
		if (ret == 0)
		{
			tarRaDec->setValueRaDec (NAN, NAN);
			tarRaDec->resetValueChanged ();
			telTargetRaDec->setValueRaDec (NAN, NAN);
			telTargetRaDec->resetValueChanged ();
			oriRaDec->setValueRaDec (NAN, NAN);
			oriRaDec->resetValueChanged ();
			offsRaDec->resetValueChanged ();
			corrRaDec->resetValueChanged ();
		}

		incMoveNum ();
		setParkTimeNow ();
		maskState (TEL_MASK_MOVING | TEL_MASK_NEED_STOP, TEL_PARKING, "parking started");
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
	rts2core::Device::signaledHUP ();
}

void Telescope::startOffseting (rts2core::Value *changed_value)
{
	startResyncMove (NULL, 0);
}

int Telescope::moveAltAz ()
{
	struct ln_hrz_posn hrz;
	telAltAz->getAltAz (&hrz);
	struct ln_lnlat_posn observer;
	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();
	struct ln_equ_posn target;

	ln_get_equ_from_hrz (&hrz, &observer, ln_get_julian_from_sys (), &target);

	oriRaDec->setValueRaDec (target.ra, target.dec);
	return startResyncMove (NULL, 0);
}

int Telescope::commandAuthorized (rts2core::Connection * conn)
{
	double obj_ra;
	double obj_ha;
	double obj_dec;

	int ret;

	if (conn->isCommand (COMMAND_TELD_MOVE))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		modelOn ();
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		mpec->setValueString ("");
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("move_ha_sg"))
	{
		if (conn->paramNextHMS (&obj_ha) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		double JD = ln_get_julian_from_sys ();
		obj_ra = getLocSidTime ( JD) * 15. - obj_ha ;

		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		mpec->setValueString ("");
		tarRaDec->setValueRaDec (NAN, NAN);
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("do_move"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		modelOn ();
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		mpec->setValueString ("");
		tarRaDec->setValueRaDec (NAN, NAN);
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("move_not_model"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		modelOff ();
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		mpec->setValueString ("");
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("move_mpec"))
	{
		char *str;
		if (conn->paramNextString (&str) || !conn->paramEnd ())
			return -2;
		modelOn ();
		mpec->setValueString (str);
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("altaz"))
	{
		if (conn->paramNextDMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		telAltAz->setValueAltAz (obj_ra, obj_dec);
		return moveAltAz ();
	}
	else if (conn->isCommand ("resync"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		oriRaDec->setValueRaDec (obj_ra, obj_dec);
		mpec->setValueString ("");
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("setto"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return -2;
		modelOn ();
		return setTo (conn, obj_ra, obj_dec);
	}
	else if (conn->isCommand ("settopark"))
	{
		if (!conn->paramEnd ())
			return -2;
		return setToPark (conn);
	}
	else if (conn->isCommand ("synccorr"))
	{
		if (!conn->paramEnd ())
			return -2;
		ret = setTo (conn, objRaDec->getRa (), objRaDec->getDec ());
		if (ret)
			return -2;
		corrRaDec->setValueRaDec (0, 0);
		wcorrRaDec->setValueRaDec (0, 0);
		return ret;
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
		if (applyCorrectionsFixed (total_cor_ra, total_cor_dec) == 0)
			return 0;
		if (cor_mark == moveNum->getValueInteger () && corr_img == corrImgId->getValueInteger () && img_id > wCorrImgId->getValueInteger ())
		{
			if (pos_err < ignoreCorrection->getValueDouble ())
			{
				conn->sendCommandEnd (DEVDEM_E_IGNORE, "ignoring correction as it is too small");
				return -1;
			}

			wcorrRaDec->setValueRaDec (total_cor_ra, total_cor_dec);

			posErr->setValueDouble (pos_err);
			sendValueAll (posErr);

			wCorrImgId->setValueInteger (img_id);
			sendValueAll (wCorrImgId);

			if (pos_err < smallCorrection->getValueDouble ()) {

			  logStream (MESSAGE_ERROR) << "NOT correcting pos_err" << pos_err<< sendLog;
			  conn->sendCommandEnd (DEVDEM_E_IGNORE, "ignoring correction debugging phase");

			  return -1;
			  //	return startResyncMove (conn, 1);
			}
			sendValueAll (wcorrRaDec);
			// else set BOP_EXPOSURE, and wait for result of statusUpdate call
			maskState (BOP_EXPOSURE, BOP_EXPOSURE, "blocking exposure for correction");
			// correction was accepted, will be carried once it will be possible
			return 0;
		}
		std::ostringstream _os;
		_os << "ignoring correction - cor_mark " << cor_mark << " moveNum " << moveNum->getValueInteger () << " corr_img " << corr_img << " corrImgId " << corrImgId->getValueInteger () << " img_id " << img_id << " wCorrImgId " << wCorrImgId->getValueInteger ();
		conn->sendCommandEnd (DEVDEM_E_IGNORE, _os.str ().c_str ());
		return -1;
	}
	else if (conn->isCommand ("park"))
	{
		if (!conn->paramEnd ())
			return -2;
		modelOn ();
		return startPark (conn);
	}
	else if (conn->isCommand ("stop"))
	{
		if (!conn->paramEnd ())
			return -2;
		return stopMove ();
	}
	else if (conn->isCommand ("change"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec)
			|| !conn->paramEnd ())
			return -2;
		offsRaDec->incValueRaDec (obj_ra, obj_dec);
		woffsRaDec->incValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, 1);
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
	return rts2core::Device::commandAuthorized (conn);
}

void Telescope::setFullBopState (int new_state)
{
	rts2core::Device::setFullBopState (new_state);
	if ((woffsRaDec->wasChanged () || wcorrRaDec->wasChanged ()) && !(new_state & BOP_TEL_MOVE))
		startResyncMove (NULL, (woffsRaDec->wasChanged () ? 1 : 0) | (wcorrRaDec->wasChanged () ? 2 : 0));
}
