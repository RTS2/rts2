/*
 * Telescope control daemon.
 * Copyright (C) 2003-2017 Petr Kubanek <petr@kubanek.net>
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
#include "clirotator.h"

#include "pluto/norad.h"
#include "pluto/observe.h"

#include "telmodel.h"
#include "gpointmodel.h"
#include "tpointmodel.h"

#include "dut1.h"

#ifdef RTS2_LIBERFA
#include "erfa.h"
#endif

#define OPT_BLOCK_ON_STANDBY  OPT_LOCAL + 117
#define OPT_HORIZON	   OPT_LOCAL + 118
#define OPT_CORRECTION	OPT_LOCAL + 119
#define OPT_WCS_MULTI	 OPT_LOCAL + 120
#define OPT_PARK_POS	  OPT_LOCAL + 121
#define OPT_DEC_UPPER_LIMIT   OPT_LOCAL + 122
#define OPT_RTS2_MODEL	OPT_LOCAL + 123
#define OPT_T_POINT_MODEL	 OPT_LOCAL + 124
#define OPT_DUT1_USNO       OPT_LOCAL + 125

#define EVENT_TELD_MPEC_REFRESH  RTS2_LOCAL_EVENT + 1200
#define EVENT_TRACKING_TIMER	 RTS2_LOCAL_EVENT + 1201

using namespace rts2teld;

Telescope::Telescope (int in_argc, char **in_argv, bool diffTrack, bool hasTracking, int hasUnTelCoordinates, bool hasAltAzDiff, bool parkingBlock, bool hasDerotators):rts2core::Device (in_argc, in_argv, DEVICE_TYPE_MOUNT, "T0")
{
	parkingBlockMove = parkingBlock;

	for (int i = 0; i < 4; i++)
	{
		timerclear (dir_timeouts + i);
	}

	parkPos = NULL;
	parkFlip = NULL;

	nextCupSync = 0;
	lastTrackLog = 0;
	unstableDist = 0;

	useParkFlipping = false;

	decUpperLimit = NULL;
	dut1fn = NULL;

	createValue (telPressure, "PRESSURE", "observatory atmospheric pressure", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	telPressure->setValueFloat (1000);

	createValue (telAmbientTemperature, "AMBTEMP", "[C] observatory ambient temperature", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	telAmbientTemperature->setValueFloat (10);

	createValue (telHumidity, "AMBHUMIDITY", "[%] observatory relative humidity", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE | RTS2_DT_PERCENTS);
	telHumidity->setValueFloat (70);

	createValue (telWavelength, "WAVELENGTH", "[nm] incoming radiation wavelength", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	telWavelength->setValueFloat (500);

	createValue (telDUT1, "DUT1", "[s] UT1 - UTC", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	telDUT1->setValueDouble (0);

	createValue (pointingModel, "MOUNT", "mount pointing model (equ, alt-az, ...)", false, 0, 0);
	pointingModel->addSelVal ("EQU");
	pointingModel->addSelVal ("ALT-AZ");
	pointingModel->addSelVal ("ALT-ALT");

	// object
	createValue (oriRaDec, "ORI", "original position (epoch)", true, RTS2_VALUE_WRITABLE);
	createValue (oriEpoch, "OEPOCH", "epoch (2000,..)", true);
	oriEpoch->setValueDouble (2000.0);
	createValue (pmRaDec, "PM", "proper motion/year", true, RTS2_VALUE_WRITABLE | RTS2_DT_ARCSEC);
	pmRaDec->setValueRaDec (0, 0);
	// users offset
	createValue (offsRaDec, "OFFS", "object offset", true, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	offsRaDec->setValueRaDec (0, 0);

	createValue (gOffsRaDec, "GOFFS", "guiding offsets", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE);
	gOffsRaDec->setValueRaDec (0, 0);

	createValue (guidingTime, "GTIME", "time the telescope is guiding", false);
	createValue (lastGuideTime, "LGTIME", "time when last guide command was received", false);

	if (hasAltAzDiff)
	{
		createValue (offsAltAz, "AZALOFFS", "alt-azimuth offsets", true, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
		offsAltAz->setValueAltAz (0, 0);
	}
	else
	{
		offsAltAz = NULL;
	}

	createValue (woffsRaDec, "woffs", "offsets waiting to be applied", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	woffsRaDec->setValueRaDec (0, 0);
	woffsRaDec->resetValueChanged ();

	cos_lat = NAN;
	sin_lat = NAN;
	tan_lat = NAN;

	if (diffTrack)
	{
		createValue (diffTrackRaDec, "DRATE", "[deg/hour] differential tracking rate", true, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
		diffTrackRaDec->setValueRaDec (0, 0);

		createValue (diffTrackStart, "DSTART", "start time of differential tracking", false);
		createValue (diffTrackOn, "DON", "differential tracking on/off", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		diffTrackOn->setValueBool (false);
	}
	else
	{
		diffTrackRaDec = NULL;
		diffTrackStart = NULL;
		diffTrackOn = NULL;
	}

	if (hasAltAzDiff)
	{
		createValue (diffTrackAltAz, "AZALRATE", "[deg/hour] differential tracking rate in alt/az", true, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
		diffTrackAltAz->setValueAltAz (0, 0);

		createValue (diffTrackAltAzStart, "AZALSTART", "start time of differential tracking", false);
		createValue (diffTrackAltAzOn, "AZALON", "differential tracking on/off", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		diffTrackAltAzOn->setValueBool (false);
	}
	else
	{
		diffTrackAltAz = NULL;
		diffTrackAltAzStart = NULL;
		diffTrackAltAzOn = NULL;
	}

	if (hasTracking)
	{
		createValue (tracking, "TRACKING", "telescope tracking", true, RTS2_VALUE_WRITABLE);
		tracking->addSelVal ("off");
		tracking->addSelVal ("on");
		tracking->addSelVal ("sidereal");
		if (hasDerotators)
			tracking->addSelVal ("off derotators");
		createValue (trackingInterval, "tracking_interval", "[s] interval for tracking loop", false, RTS2_VALUE_WRITABLE | RTS2_DT_TIMEINTERVAL);
		trackingInterval->setValueFloat (0.5);

		createValue (trackingFrequency, "tracking_frequency", "[Hz] tracking frequency", false);
		createValue (trackingFSize, "tracking_num", "numbers of tracking request to calculate tracking stat", false, RTS2_VALUE_WRITABLE);
		createValue (trackingWarning, "tracking_warning", "issue warning if tracking frequency drops bellow this number", false, RTS2_VALUE_WRITABLE);
		trackingWarning->setValueFloat (1);
		trackingFSize->setValueInteger (20);

		createValue (skyVect, "SKYSPD", "[deg/hour] tracking speeds vector (in RA/DEC)", true, RTS2_DT_DEGREES);
	}
	else
	{
		tracking = NULL;
		trackingInterval = NULL;
		trackingFrequency = NULL;
		trackingWarning = NULL;
		skyVect = NULL;
	}

	lastTrackingRun = NAN;
	trackingNum = 0;

	createValue (objRaDec, "OBJ", "telescope FOV center position (J2000) - with offsets applied", true);

	tarTelRaDec = NULL;
	tarTelAltAz = NULL;
	modelTarAltAz = NULL;

	switch (hasUnTelCoordinates)
	{
		case 1:
			createValue (tarTelRaDec, "TAR_TEL", "target position (OBJ) in telescope coordinates (DEC in 180 .. -180 range)", true);
			break;

		case -1:
			createValue (tarTelAltAz, "TAR_TEL", "target position (OBJ) in telescope coordinates (AZ in 180 .. -180 range)", true);
			createValue (modelTarAltAz, "TAR_MOD", "modelled target position (OBJ) in telescope coordinates (AZ in 180 .. -180 range)", false);
			break;
	}

	createValue (tarRaDec, "TAR", "target position with computed corrections (precession, refraction, aberation) applied", true);

	createValue (corrRaDec, "CORR_", "correction from closed loop", true, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	corrRaDec->setValueRaDec (0, 0);

	createValue (wcorrRaDec, "wcorr", "corrections which waits for being applied", false, RTS2_DT_DEG_DIST_180 | RTS2_VALUE_WRITABLE, 0);
	wcorrRaDec->setValueRaDec (0, 0);
	wcorrRaDec->resetValueChanged ();

	createValue (total_offsets, "total_offsets", "[deg] OFFS - corr", false, RTS2_DT_DEG_DIST_180);

	createValue (wCorrImgId, "wcorr_img", "Image id waiting for correction", false, RTS2_VALUE_WRITABLE, 0);
	createValue (wCorrObsId, "wcorr_obs", "Observation id waiting for correction", false, RTS2_VALUE_WRITABLE, 0);

	// position error
	createValue (posErr, "pos_err", "error in degrees", false, RTS2_DT_DEG_DIST_180);

#ifndef RTS2_LIBERFA
	createValue (precessed, "precessed", "target coordinates, aberated and precessed", false);
	createValue (nutated, "nutated", "target coordinates, nutated", false);
	createValue (aberated, "aberated", "target coordinates, aberated", false);
	createValue (refraction, "refraction", "[deg] refraction (in altitude)", false, RTS2_DT_DEG_DIST_180);
#endif

	createValue (modelRaDec, "MO_RTS2", "[deg] RTS2 model offsets", true, RTS2_DT_DEGREES, 0);
	modelRaDec->setValueRaDec (0, 0);

	createValue (telTargetRaDec, "tel_target", "target RA DEC telescope coordinates - virtual coordinates fed to TCS", false);

	wcs_crval1 = wcs_crval2 = NULL;

	// target + model + corrections = sends to tel ... TEL (read from sensors, if possible)
	createValue (telRaDec, "TEL", "mount position (from sensors, sky coordinates)", true);

	// like TEL, but raw values, no normalization (i.e. no flip transformation):
	telUnRaDec = NULL;
	telUnAltAz = NULL;
	switch (hasUnTelCoordinates)
	{
		case 1:
			createValue (telUnRaDec, "U_TEL", "mount position (from sensors, raw coordinates, no flip transformation)", true);
			break;
		case -1:
			createValue (telUnAltAz, "U_TEL", "mount position (from sensors, raw coordinates, no flip transformation)", true);
			break;
	}

	createValue (telAltAz, "TEL_", "horizontal telescope coordinates", true, RTS2_VALUE_WRITABLE);

	createValue (mpec, "mpec_target", "MPEC string (used for target calculation, if set)", false, RTS2_VALUE_WRITABLE);
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

	createValue (trackingLogInterval, "tracking_log_interval", "[s] interval for tracking logs", false, RTS2_VALUE_WRITABLE);
	trackingLogInterval->setValueDouble (30);

	createValue (tle_l1, "tle_l1_target", "TLE target line 1", false);
	createValue (tle_l2, "tle_l2_target", "TLE target line 2", false);
	createValue (tle_ephem, "tle_ephem", "TLE emphemeris type", false);
	createValue (tle_distance, "tle_distance", "[km] satellite distance", false);
	createValue (tle_freeze, "tle_freeze", "if true, stop updating TLE positions; put current speed vector to DRATE", false, RTS2_VALUE_WRITABLE);
	tle_freeze->setValueBool (false);
	createValue (tle_rho_sin_phi, "tle_rho_sin", "TLE rho_sin_phi (observatory position)", false);
	createValue (tle_rho_cos_phi, "tle_rho_cos", "TLE rho_cos_phi (observatory position)", false);
	createValue (tle_refresh, "tle_refresh", "refresh TLE ra_diff and dec_diff every tle_refresh seconds", false, RTS2_VALUE_WRITABLE);

	createConstValue (telLatitude, "LATITUDE", "observatory latitude", true, RTS2_DT_DEGREES);
	createConstValue (telLongitude, "LONGITUD", "observatory longitude", true, RTS2_DT_DEGREES);
	createConstValue (telAltitude, "ALTITUDE", "observatory altitude", true);

	createValue (refreshIdle, "refresh_idle", "idle and tracking refresh interval", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE);
	createValue (refreshSlew, "refresh_slew", "slew refresh interval", false, RTS2_DT_TIMEINTERVAL | RTS2_VALUE_WRITABLE);

	refreshIdle->setValueDouble (60);
	refreshSlew->setValueDouble (0.5);

	createValue (mountParkTime, "PARKTIME", "Time of last mount park");

	createValue (blockMove, "block_move", "if true, any software movement of the telescope is blocked", false, RTS2_VALUE_WRITABLE);
	blockMove->setValueBool (false);

	createValue (blockOnStandby, "block_on_standby", "Block telescope movement if switched to standby/off mode. Enable it if switched back to on.", false, RTS2_VALUE_WRITABLE);
	blockOnStandby->setValueBool (false);

	createValue (airmass, "AIRMASS", "Airmass of target location");
	createValue (hourAngle, "HA", "Location hour angle", true, RTS2_DT_RA);
	createValue (lst, "LST", "Local Sidereal Time", true, RTS2_DT_RA);
	createValue (jdVal, "JD", "Modified Julian Date", true);

	createValue (targetDistance, "target_distance", "distance to the target in degrees", false, RTS2_DT_DEG_DIST);
	createValue (targetStarted, "move_started", "time when movement was started", false);
	createValue (targetReached, "move_end", "expected time when telescope will reach the destination", false);
	if (hasTracking)
		createValue (targetDistanceStat, "tdist_stat", "statistics of target distances", false, RTS2_DT_DEG_DIST);
	else
		targetDistanceStat = NULL;

	targetDistance->setValueDouble (NAN);
	targetStarted->setValueDouble (NAN);
	targetReached->setValueDouble (NAN);

	createValue (moveNum, "MOVE_NUM", "number of movements performed by the driver; used in corrections for synchronization", true);
	moveNum->setValueInteger (0);

	createValue (corrImgId, "CORR_IMG", "ID of last image used for correction", true, RTS2_VALUE_WRITABLE);
	corrImgId->setValueInteger (0);
	createValue (corrObsId, "CORR_OBS", "ID of last observation used for correction", true, RTS2_VALUE_WRITABLE);
	corrObsId->setValueInteger (0);

	createValue (failedMoveNum, "move_failed", "number of failed movements", false);
	failedMoveNum->setValueInteger (0);

	createValue (lastFailedMove, "move_failed_last", "last time movement failed", false);

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
	createValue (peekFlip, "peek_flip", "predicted telescope flip for peek command", false);

	flip_move_start = 0;
	flip_longest_path = -1;

	// default is to aply model corrections

#ifndef RTS2_LIBERFA
	createValue (calPrecession, "CAL_PREC", "if precession is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calPrecession->setValueBool (false);

	createValue (calNutation, "CAL_NUT", "if nutation is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calNutation->setValueBool (false);

	createValue (calAberation, "CAL_ABER", "if aberation is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calAberation->setValueBool (false);

	createValue (calRefraction, "CAL_REFR", "if refraction is included in target calculations", false, RTS2_VALUE_WRITABLE);
	calRefraction->setValueBool (false);
#endif

	createValue (calModel, "CAL_MODE", "if model calculations are included in target calcuations", false, RTS2_VALUE_WRITABLE);
	calModel->setValueBool (false);

	createValue (cupolas, "cupolas", "Cupola(s) connected to telescope. They should sync as telescope moves.", false);

	createValue (rotators, "rotators", "Rotator(s) connected to telescope.", false);

	tPointModelFile = NULL;
	rts2ModelFile = NULL;
	model = NULL;

	createValue (standbyPark, "standby_park", "Park telescope when switching to standby", false, RTS2_VALUE_WRITABLE);
	standbyPark->addSelVal ("no");
	standbyPark->addSelVal ("daytime only");
	standbyPark->addSelVal ("nightime");

	standbyPark->setValueInteger (0);

	horizonFile = NULL;
	hardHorizon = NULL;

	wcs_multi = '!';

	useOEpoch = true;

	addOption (OPT_RTS2_MODEL, "rts2-model", 1, "RTS2 pointing model filename");
	addOption (OPT_T_POINT_MODEL, "t-point-model", 1, "T-Point model filename");
	addOption ('l', NULL, 1, "separation limit (corrections above that number in degrees will be ignored)");
	addOption ('g', NULL, 1, "minimal good separation. Correction above that number will be aplied immediately. Default to 180 deg");

	addOption ('c', NULL, 1, "minimal value for corrections. Corrections bellow that value will be rejected.");

	addOption ('s', NULL, 1, "park when switched to standby - 1 only at day, 2 even at night");
	addOption (OPT_BLOCK_ON_STANDBY, "block-on-standby", 0, "block telescope movement when switching to standby");

	addOption ('r', NULL, 1, "telescope rotang");
	addOption (OPT_HORIZON, "horizon", 1, "telescope hard horizon");
	addOption (OPT_CORRECTION, "max-correction", 1, "correction limit (in arcsec)");
	addOption (OPT_WCS_MULTI, "wcs-multi", 1, "letter for multiple WCS (A-Z,-)");
	addOption (OPT_DEC_UPPER_LIMIT, "dec-upper-limit", 1, "maximal declination the telescope is able to point to");
	addOption (OPT_DUT1_USNO, "dut1-filename", 1, "filename of USNO DUT1 offset file");

	setIdleInfoInterval (refreshIdle->getValueDouble ());

	moveInfoCount = 0;
	moveInfoMax = 100;
}

Telescope::~Telescope (void)
{
	delete model;
}

int Telescope::checkTracking (double maxDist)
{
	// check tracking stability
	if (targetDistanceStat != NULL && (getState () & (TEL_MASK_MOVING | TEL_MASK_CORRECTING | TEL_MASK_OFFSETING)) == TEL_OBSERVING)
	{
		if ((getState () & TEL_MASK_UNSTABLE) == TEL_UNSTABLE)
		{
			if (targetDistanceStat->getMax () <= maxDist)
			{
				valueGood (targetDistanceStat);
				maskState (DEVICE_ERROR_MASK | TEL_MASK_UNSTABLE, TEL_STABLE, "stable pointing");
				logStream (MESSAGE_INFO) << "stable telescope tracking - max distance " << LibnovaDegDist (unstableDist) << ", current " << LibnovaDegDist (targetDistanceStat->getMax ()) << sendLog;
				unstableDist = 0;
				return 1;
			}
			else
			{
				if (targetDistanceStat->getMax () > unstableDist)
					unstableDist = targetDistanceStat->getMax ();
			}
		}
		else
		{
			if (targetDistanceStat->getMax () > maxDist)
			{
				valueWarning (targetDistanceStat);
				maskState (DEVICE_ERROR_MASK | TEL_MASK_UNSTABLE, DEVICE_ERROR_HW | TEL_UNSTABLE, "unstable poiting");
				logStream (MESSAGE_WARNING) << "unstable telescope tracking - max distance " << LibnovaDegDist (targetDistanceStat->getMax ()) << sendLog;
				return -1;
			}
		}
	}
	return 0;
}

double Telescope::getLocSidTime (double JD)
{
	double ret;
	ret = ln_get_apparent_sidereal_time (JD) * 15.0 + telLongitude->getValueDouble ();
	return ln_range_degrees (ret) / 15.0;
}

void Telescope::setTarTelRaDec (struct ln_equ_posn *pos)
{
	if (tarTelRaDec == NULL)
		return;

	tarTelRaDec->setValueRaDec (pos->ra, pos->dec);
}

void Telescope::setTarTelAltAz (struct ln_hrz_posn *hrz)
{
	if (tarTelAltAz == NULL)
		return;

	tarTelAltAz->setValueAltAz (hrz->alt, hrz->az);
}

void Telescope::setModelTarAltAz (struct ln_hrz_posn *hrz)
{
	if (modelTarAltAz == NULL)
		return;

	modelTarAltAz->setValueAltAz (hrz->alt, hrz->az);
}

void Telescope::getModelTarAltAz (struct ln_hrz_posn *pos)
{
	if (modelTarAltAz == NULL)
		return;

	modelTarAltAz->getAltAz (pos);
}

int Telescope::calculateTarget (const double utc1, const double utc2, struct ln_equ_posn *out_tar, struct ln_hrz_posn *out_hrz, int32_t &ac, int32_t &dc, bool writeValues, double haMargin, bool forceShortest)
{
	double tar_distance = NAN;

	switch (tracking->getValueInteger ())
	{
		case 1:	   // on object
			// calculate from MPEC..
			if (mpec->getValueString ().length () > 0)
			{
				struct ln_lnlat_posn observer;
				struct ln_equ_posn parallax;
				observer.lat = telLatitude->getValueDouble ();
				observer.lng = telLongitude->getValueDouble ();
				LibnovaCurrentFromOrbit (out_tar, &mpec_orbit, &observer, telAltitude->getValueDouble (), utc1 + utc2, &parallax);
                                if (writeValues)
					setOrigin (out_tar->ra, out_tar->dec);
				break;
			}
			// calculate from TLE..
			else if (tle_freeze->getValueBool () == false && tle_l1->getValueString ().length () > 0 && tle_l2->getValueString ().length () > 0)
			{
				calculateTLE (utc1 + utc2, out_tar->ra, out_tar->dec, tar_distance);
				out_tar->ra = ln_rad_to_deg (out_tar->ra);
				out_tar->dec = ln_rad_to_deg (out_tar->dec);
				if (writeValues)
					setOrigin (out_tar->ra, out_tar->dec);
				break;
			}
			// don't break, as sidereal tracking will be used
		default:
			// get from ORI, if constant..
			getOrigin (out_tar);
			if (!std::isnan (diffTrackStart->getValueDouble ()))
				addDiffRaDec (out_tar, (utc1 + utc2 - diffTrackStart->getValueDouble ()) * 86400.0);
	}

	// offsets, corrections,..
	applyOffsets (out_tar);

	// crossed pole, put on opposite side..
	if (out_tar->dec > 90)
	{
		out_tar->ra = ln_range_degrees (out_tar->ra + 180.0);
		out_tar->dec = 180 - out_tar->dec;
	}
	else if (out_tar->dec < -90)
	{
		out_tar->ra = ln_range_degrees (out_tar->ra + 180.0);
		out_tar->dec = -180 - out_tar->dec;
	}

	if (writeValues)
		setObject (out_tar->ra, out_tar->dec);

	return sky2counts (utc1, utc2, out_tar, out_hrz, ac, dc, writeValues, haMargin, forceShortest);
}

int Telescope::calculateTracking (const double utc1, const double utc2, double sec_step, int32_t &ac, int32_t &dc, double &ac_speed, double &dc_speed, double &ea_speed, double &ed_speed, double &speed_angle, double &err_angle)
{
	struct ln_equ_posn eqpos, t_eqpos;
	struct ln_hrz_posn hrz, t_hrz;
	// refresh current target..

	int32_t c_ac = ac;
	int32_t c_dc = dc;
	int32_t t_ac = ac;
	int32_t t_dc = dc;
	int ret = calculateTarget (utc1, utc2, &eqpos, &hrz, c_ac, c_dc, true, 0, true);
	if (ret)
		return ret;

	ret = calculateTarget (utc1, utc2 + sec_step / 86400.0, &t_eqpos, &t_hrz, t_ac, t_dc, false, 0, true);
	if (ret)
		return ret;

	//std::cout << "calculateTracking " << utc1 << " " << utc2 << " " << LibnovaRaDec (&eqpos) << " " << LibnovaRaDec (&t_eqpos) << " " << sec_step << " current " << c_ac << " " << c_dc << " target " << t_ac << " " << t_dc << " ac " << ac << " " << dc << std::endl;

	// for speed vector calculation..
	double ra_diff = t_eqpos.ra - eqpos.ra;
	double dec_diff = t_eqpos.dec - eqpos.dec;

	if (ra_diff > 180.0)
		ra_diff = ra_diff - 360;
	else if (ra_diff < -180.0)
		ra_diff = ra_diff + 360;

	skyVect->setValueRaDec (3600 * ra_diff / sec_step, 3600 * dec_diff / sec_step);

	ac_speed = (c_ac - t_ac) / sec_step;
	dc_speed = (c_dc - t_dc) / sec_step;

	ea_speed = (ac - c_ac) / sec_step;
	ed_speed = (dc - c_dc) / sec_step;

	speed_angle = ln_rad_to_deg (atan2 (ac_speed, dc_speed));
	err_angle = ln_rad_to_deg (atan2 (ea_speed, ed_speed));

	ac = t_ac;
	dc = t_dc;

	return 0;
}

int Telescope::sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, struct ln_hrz_posn *hrz_out, int32_t &ac, int32_t &dc, bool writeValues, double haMargin, bool forceShortest)
{
	return -1;
}

void Telescope::addDiffRaDec (struct ln_equ_posn *tar, double secdiff)
{
	if (diffTrackRaDec && diffTrackOn->getValueBool () == true)
	{
		tar->ra += getDiffTrackRa () * secdiff / 3600.0;
		tar->dec += getDiffTrackDec () * secdiff / 3600.0;
	}
}

void Telescope::addDiffAltAz (struct ln_hrz_posn *hrz, double secdiff)
{
	if (diffTrackAltAz && diffTrackAltAzOn->getValueBool () == true)
	{
		hrz->az += getDiffTrackAz () * secdiff / 3600.0;
		hrz->alt += getDiffTrackAlt () * secdiff / 3600.0;
	}
}

void Telescope::createRaPAN ()
{
	createValue (raPAN, "ra_pan", "RA panning", false, RTS2_VALUE_WRITABLE);
	raPAN->addSelVal ("NONE");
	raPAN->addSelVal ("MINUS");
	raPAN->addSelVal ("PLUS");

	createValue (raPANSpeed, "ra_pan_speed", "speed of RA panning", false, RTS2_VALUE_WRITABLE);
}

void Telescope::createDecPAN ()
{
	createValue (decPAN, "dec_pan", "DEC panning", false, RTS2_VALUE_WRITABLE);
	decPAN->addSelVal ("NONE");
	decPAN->addSelVal ("MINUS");
	decPAN->addSelVal ("PLUS");

	createValue (decPANSpeed, "dec_pan_speed", "speed of DEC panning", false, RTS2_VALUE_WRITABLE);
}


void Telescope::applyOffsets (struct ln_equ_posn *pos)
{
	if (useOEpoch && oriEpoch->getValueDouble () != 2000.0)
	{
		struct ln_date ld;
		struct ln_equ_posn ori;
		ori.ra = pos->ra;
		ori.dec = pos->dec;
		ld.years = floor (oriEpoch->getValueDouble ());
		ld.months = 1;
		ld.days = 1;
		ld.hours = 0;
		ld.minutes = 0;
		ld.seconds = 0;
		double JDfrom = ln_get_julian_day (&ld) + (oriEpoch->getValueDouble () - ld.years) * 365.24219;
		ln_get_equ_prec2 (&ori, JDfrom, JD2000, pos);
	}

	pos->ra += offsRaDec->getRa () + gOffsRaDec->getRa ();
	pos->dec += offsRaDec->getDec () + gOffsRaDec->getDec ();
}


int Telescope::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_RTS2_MODEL:
			rts2ModelFile = optarg;
			calModel->setValueBool (true);
			break;
		case OPT_T_POINT_MODEL:
			tPointModelFile = optarg;
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
		case OPT_DEC_UPPER_LIMIT:
			createValue (decUpperLimit, "dec_upper_limit", "upper limit on telescope declination", false, RTS2_VALUE_WRITABLE);
			decUpperLimit->setValueCharArr (optarg);
			break;

		case OPT_PARK_POS:
			{
				std::istringstream *is;
				is = new std::istringstream (std::string(optarg));
				double palt,paz;
				int flip;
				char c;
				*is >> palt >> c >> paz;
				if (is->fail () || c != ':')
				{
					logStream (MESSAGE_ERROR) << "Cannot parse alt-az park position " << optarg << sendLog;
					delete is;
					return -1;
				}
				*is >> c >> flip;
				if (is->fail ())
					flip = -1;

				delete is;
				if (parkPos == NULL)
				{
					createParkPos (palt, paz, flip);
				}
				else
				{
					parkPos->setValueAltAz (palt, paz);
					parkFlip->setValueInteger (flip);
				}
			}
			break;
		case OPT_DUT1_USNO:
			dut1fn = optarg;
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

	double ast = ln_get_apparent_sidereal_time(jd);

	ln_get_hrz_from_equ_sidereal_time (&equ_target, &observer, ast, &tarAltAz);

	if (corrRaDec->getRa () == 0 && corrRaDec->getDec () == 0)
	{
		corrAltAz.alt = tarAltAz.alt;
		corrAltAz.az = tarAltAz.az;
	}
	else
	{
		ln_get_hrz_from_equ_sidereal_time (&equ_corr, &observer, ast, &corrAltAz);
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
	if (modelTarAltAz != NULL)
	{
		struct ln_hrz_posn hrz_tar, hrz_tel;
		modelTarAltAz->getAltAz (&hrz_tar);
		getTelAltAz (&hrz_tel);

		tar.ra = hrz_tar.az;
		tar.dec = hrz_tar.alt;
		tel.ra = hrz_tel.az;
		tel.dec = hrz_tel.alt;

		if (offsAltAz != NULL)
		{
			tel.ra += offsAltAz->getAz ();
			tel.dec += offsAltAz->getAlt ();
		}
	}
	else
	{
		getTelTargetRaDec (&tar);
		getTelRaDec (&tel);
		normalizeRaDec (tel.ra, tel.dec);
	}

	if (std::isnan(tar.ra) || std::isnan(tar.dec) || std::isnan(tel.ra) || std::isnan(tel.dec))
		return -1;

	return ln_get_angular_separation (&tel, &tar);
}

void Telescope::getTargetAltAz (struct ln_hrz_posn *hrz, double jd)
{
	struct ln_equ_posn tar;
	getTarget (&tar);
	struct ln_lnlat_posn observer;
	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();
	ln_get_hrz_from_equ_sidereal_time (&tar, &observer, ln_get_apparent_sidereal_time (jd), hrz);
}

double Telescope::getTargetHa ()
{
	return getTargetHa (ln_get_julian_from_sys ());
}

double Telescope::getTargetHa (double jd)
{
	return ln_range_degrees (ln_get_apparent_sidereal_time (jd) - tarRaDec->getRa ());
}

double Telescope::getLstDeg (double utc1, double utc2)
{
#ifdef RTS2_LIBERFA
	double ut11, ut12, tt1, tt2;
	if (eraUtcut1 (utc1, utc2, telDUT1->getValueDouble (), &ut11, &ut12))
		return NAN;
	if (eraUt1tt (ut11, ut12, telDUT1->getValueDouble (), &tt1, &tt2))
		return NAN;
	return ln_range_degrees (ln_rad_to_deg (eraGst06a (ut11, ut12, tt1, tt2)) + telLongitude->getValueDouble ());
#else
	return ln_range_degrees (15. * ln_get_apparent_sidereal_time (utc1 + utc2) + telLongitude->getValueDouble ());
#endif
}

int Telescope::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == tracking)
	{
		if (((rts2core::ValueBool *) new_value)->getValueBool ())
		{
			// we don't allow starting of tracking via setValue in these cases...
			if (blockMove->getValueBool () == true || (getState () & TEL_MASK_MOVING) == TEL_PARKING || (getState () & TEL_MASK_MOVING) == TEL_PARKED)
				return -2;
		}
		if (setTracking (new_value->getValueInteger (), true, false))
			return -2;
	}
	else if (old_value == mpec)
	{
		std::string desc;
		if (LibnovaEllFromMPC (&mpec_orbit, desc, new_value->getValue ()))
			if (LibnovaEllFromMPCComet (&mpec_orbit, desc, new_value->getValue ()))
				return -2;
	}
	else if (old_value == diffTrackRaDec)
	{
	  	setDiffTrack (((rts2core::ValueRaDec *)new_value)->getRa (), ((rts2core::ValueRaDec *)new_value)->getDec ());
	}
	else if (old_value == diffTrackOn)
	{
		logStream (MESSAGE_INFO) << "switching DON to " << ((rts2core::ValueBool *) new_value)->getValueBool () << sendLog;
		if (((rts2core::ValueBool *) new_value)->getValueBool () == true)
		{
			setDiffTrack (getDiffTrackRa (), getDiffTrackDec ());
		}
		else
		{
			setDiffTrack (0, 0);
		}
	}
	else if (old_value == diffTrackAltAz)
	{
		setDiffTrackAltAz (((rts2core::ValueAltAz *)new_value)->getAz (), ((rts2core::ValueAltAz *)new_value)->getAlt ());
	}
	else if (old_value == diffTrackAltAzOn)
	{
		logStream (MESSAGE_INFO) << "switching AZALON to " << ((rts2core::ValueBool *) new_value)->getValueBool () << sendLog;
		if (((rts2core::ValueBool *) new_value)->getValueBool () == true)
		{
			setDiffTrackAltAz (getDiffTrackAz (), getDiffTrackAlt ());
		}
		else
		{
			setDiffTrackAltAz (0, 0);
		}
	}
	else if (old_value == refreshIdle)
	{
		switch (getState () & TEL_MASK_MOVING)
		{
			case TEL_OBSERVING:
			case TEL_PARKED:
				setIdleInfoInterval (((rts2core::ValueDouble *)new_value)->getValueDouble ());
				break;
		}
	}
	else if (old_value == refreshSlew)
	{
		switch (getState () & TEL_MASK_MOVING)
		{
			case TEL_MOVING:
			case TEL_PARKING:
				setIdleInfoInterval (((rts2core::ValueDouble *)new_value)->getValueDouble ());
				break;
		}
	}
	else if (old_value == tle_freeze)
	{
		if (tle_l1->getValueString ().length () == 0 || tle_l2->getValueString ().length () == 0)
			return -2;

		if (((rts2core::ValueBool *) new_value)->getValueBool () == false)
		{
			// reset DRATE, probably set by tle_freeze = true
			setDiffTrack (0, 0);
			setDiffTrackAltAz (0, 0);
		}
		else
		{
			// TLE not yet calculated..
			double JD = ln_get_julian_from_sys ();
			struct ln_equ_posn p1, p2, speed;
			double d1, d2;
			const double sec_step = 10.0;
			calculateTLE (JD, p1.ra, p1.dec, d1);
			calculateTLE (JD + sec_step / 86400.0, p2.ra, p2.dec, d2);
			speed.ra = (3600 * ln_rad_to_deg (p2.ra - p1.ra)) / sec_step;
			if (speed.ra > 180.0)
				speed.ra -= 360.0;
			else if (speed.ra < -180.0)
				speed.ra += 360.0;
			speed.dec = (3600 * ln_rad_to_deg (p2.dec - p1.dec)) / sec_step;
			diffTrackRaDec->setValueRaDec (speed.ra, speed.dec);
			sendValueAll (diffTrackRaDec);
			setDiffTrack (speed.ra, speed.dec);
		}
	}
	else if (old_value == offsAltAz)
	{
		maskState (TEL_MASK_OFFSETING, TEL_OFFSETING, "offseting telescope");
	}
	else if (old_value == gOffsRaDec)
	{
		double now = getNow ();
		if (std::isnan (guidingTime->getValueDouble ()))
			guidingTime->setValueDouble (now);
		lastGuideTime->setValueDouble (now);
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

#ifndef RTS2_LIBERFA
void Telescope::applyAberation (struct ln_equ_posn *pos, double JD, bool writeValue)
{
	ln_get_equ_aber (pos, JD, pos);
	if (writeValue)
		aberated->setValueRaDec (pos->ra, pos->dec);
}

void Telescope::applyNutation (struct ln_equ_posn *pos, double JD, bool writeValue)
{
	struct ln_nutation nut;
	ln_get_nutation (JD, &nut);

	double rad_ecliptic = ln_deg_to_rad (nut.ecliptic + nut.obliquity);
	double sin_ecliptic = sin (rad_ecliptic);

	double rad_ra = ln_deg_to_rad (pos->ra);
	double rad_dec = ln_deg_to_rad (pos->dec);

	double sin_ra = sin (rad_ra);
	double cos_ra = cos (rad_ra);

	double tan_dec = tan (rad_dec);

	double d_ra = (cos (rad_ecliptic) + sin_ecliptic * sin_ra * tan_dec) * nut.longitude - cos_ra * tan_dec * nut.obliquity;
	double d_dec = (sin_ecliptic * cos_ra) * nut.longitude + sin_ra * nut.obliquity;

	pos->ra += d_ra;
	pos->dec += d_dec;
	if (writeValue)
		nutated->setValueRaDec (pos->ra, pos->dec);
}

void Telescope::applyPrecession (struct ln_equ_posn *pos, double JD, bool writeValue)
{
	ln_get_equ_prec (pos, JD, pos);
	if (writeValue)
		precessed->setValueRaDec (pos->ra, pos->dec);
}
#endif

void Telescope::applyRefraction (struct ln_equ_posn *pos, double JD, bool writeValue)
{
	struct ln_hrz_posn hrz;
	struct ln_lnlat_posn obs;
	double ref;

	obs.lng = telLongitude->getValueDouble ();
	obs.lat = telLatitude->getValueDouble ();

	double ast = ln_get_apparent_sidereal_time(JD);

	ln_get_hrz_from_equ_sidereal_time (pos, &obs, ast, &hrz);
#ifdef RTS2_LIBERFA
	double refa, refb;
	double zd = ln_deg_to_rad (90 - hrz.alt);
	double tzd = tan (zd);
	eraRefco (getPressure (), telAmbientTemperature->getValueFloat (), telHumidity->getValueFloat () / 100.0, telWavelength->getValueFloat () / 1000.0, &refa, &refb);
	ref = ln_rad_to_deg (refa * tzd + refb * tzd * tzd * tzd);
#else
	ref = ln_get_refraction_adj (hrz.alt, getPressure (), 10);
	if (writeValue)
		refraction->setValueDouble (ref);
#endif
	hrz.alt += ref;
	ln_get_equ_from_hrz (&hrz, &obs, JD, pos);
}

void Telescope::afterMovementStart ()
{
	nextCupSync = 0;
	startCupolaSync ();
}

void Telescope::afterParkingStart ()
{
}

void Telescope::incMoveNum ()
{
	if (diffTrackRaDec)
	{
		diffTrackRaDec->setValueRaDec (0, 0);
		diffTrackStart->setValueDouble (NAN);
	  	setDiffTrack (0,0);
		diffTrackOn->setValueBool (false);
	}

	if (diffTrackAltAz)
	{
		diffTrackAltAz->setValueAltAz (0, 0);
		diffTrackStart->setValueDouble (NAN);
		setDiffTrackAltAz (0, 0);
		diffTrackAltAzOn->setValueBool (false);
	}

	tle_freeze->setValueBool (false);

	// reset offsets
	offsRaDec->setValueRaDec (0, 0);
	offsRaDec->resetValueChanged ();

	gOffsRaDec->setValueRaDec (0, 0);
	gOffsRaDec->resetValueChanged ();

	guidingTime->setValueDouble (NAN);
	lastGuideTime->setValueDouble (NAN);

	if (offsAltAz)
	{
		offsAltAz->setValueAltAz (0, 0);
		offsAltAz->resetValueChanged ();
	}

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
	corrObsId->setValueInteger (0);
	wCorrImgId->setValueInteger (0);
	wCorrObsId->setValueInteger (0);

	trackingNum = 0;
	changeIdleMovingTracking ();
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
	int decCoeff = 1;

	// to take care of situation when *pos holds raw mount coordinates
	if (fabs (pos->dec) > 90.0)
		decCoeff = -1;

	if (invertRa)
		pos2.ra = ln_range_degrees (pos->ra - corrRaDec->getRa ());
	else
		pos2.ra = ln_range_degrees (pos->ra + corrRaDec->getRa ());

	if (invertDec)
		pos2.dec = pos->dec - decCoeff * corrRaDec->getDec ();
	else
		pos2.dec = pos->dec + decCoeff * corrRaDec->getDec ();
	if (ln_get_angular_separation (&pos2, pos) > correctionLimit->getValueDouble ())
	{
		logStream (MESSAGE_WARNING) << "correction " << LibnovaDegDist (corrRaDec->getRa ()) << " " << LibnovaDegDist (corrRaDec->getDec ()) << " is above limit, ignoring it" << sendLog;
		return -1;
	}
	pos->ra = pos2.ra;
	pos->dec = pos2.dec;
	return 0;
}

void Telescope::applyModel (struct ln_equ_posn *m_pos, struct ln_hrz_posn *hrz, struct ln_equ_posn *tt_pos, struct ln_equ_posn *model_change, double utc1, double utc2)
{
	computeModel (m_pos, hrz, model_change, utc1, utc2);

	modelRaDec->setValueRaDec (model_change->ra, model_change->dec);
        tt_pos->ra -= model_change->ra;
        tt_pos->dec -= model_change->dec;

	// also include corrRaDec correction to get resulting values...
	if (applyCorrRaDec (tt_pos) == 0)
	{
		model_change->ra -= corrRaDec->getRa();
		if (fabs (m_pos->dec) > 90.0)
			model_change->dec += corrRaDec->getDec();
		else
			model_change->dec -= corrRaDec->getDec();
	}
}

void Telescope::applyModelAltAz (struct ln_hrz_posn *hrz, struct ln_equ_posn *equ, struct ln_hrz_posn *err)
{
	if (!model || calModel->getValueBool () == false)
		return;
	model->getErrAltAz (hrz, equ, err);
	modelRaDec->setValueRaDec (err->alt, err->az);
}

void Telescope::applyModelPrecomputed (struct ln_equ_posn *pos, struct ln_equ_posn *model_change, bool applyCorr)
{
	modelRaDec->setValueRaDec (model_change->ra, model_change->dec);

	// also include corrRaDec correction to get resulting values when applyCorr set to true...
	if (applyCorr == true && applyCorrRaDec (pos) == 0)
	{
		model_change->ra -= corrRaDec->getRa();
		if (fabs (pos->dec) > 90.0)
			model_change->dec += corrRaDec->getDec();
		else
			model_change->dec -= corrRaDec->getDec();
	}
}

void Telescope::computeModel (struct ln_equ_posn *pos, struct ln_hrz_posn *hrz, struct ln_equ_posn *model_change, double utc1, double utc2)
{
	struct ln_equ_posn hadec;
	double ra;
	double ls;

	if (!model || calModel->getValueBool () == false)
	{
		model_change->ra = 0;
		model_change->dec = 0;
		return;
	}
	ls = getLstDeg (utc1, utc2);
	hadec.ra = ls - pos->ra;	// intentionally without ln_range_degrees
	hadec.dec = pos->dec;

	model->reverse (&hadec, hrz);

	// get back from model - from HA
	ra = ls - hadec.ra;

	// calculate change
	model_change->ra = ln_range_degrees (pos->ra - ra);	// here must be ln_range_degrees, flip computation above might have destroyed the continuity
	if (model_change->ra > 180.0)
		model_change->ra -= 360.0;
	model_change->dec = pos->dec - hadec.dec;

	hadec.ra = ra;

	double sep = ln_get_angular_separation (&hadec, pos);

	// change above modelLimit are strange - reject them
	if (sep > modelLimit->getValueDouble ())
	{
		logStream (MESSAGE_WARNING)
			<< "telescope computeModel big change - rejecting "
			<< model_change->ra << " "
			<< model_change->dec << " sep " << sep << sendLog;
		model_change->ra = 0;
		model_change->dec = 0;
		return;
	}

	if (getDebug ())
	{
		logStream (MESSAGE_DEBUG)
			<< "Telescope::computeModel offsets ra: "
			<< model_change->ra << " dec: " << model_change->dec
			<< sendLog;
	}
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

	if (tPointModelFile && rts2ModelFile)
	{
		std::cerr << "cannot specify both 'tpoint-model' and 'rts2-model'. Please choose only one model file" << std::endl;
		return -1;
	}

	if (rts2ModelFile)
	{
		model = new rts2telmodel::GPointModel (getLatitude ());
		ret = model->load (rts2ModelFile);
		if (ret)
			return ret;
		rts2core::DoubleArray *p;
		createValue (p, "rts2_model", "RTS2 model parameters", false);
		for (int i = 0; i < 9; i++)
			p->addValue (((rts2telmodel::GPointModel *) model)->params[i]);

	}
	else if (tPointModelFile)
	{
		model = new rts2telmodel::TPointModel (getLatitude ());
		ret = model->load (tPointModelFile);
		if (ret)
			return ret;
		for (std::vector <rts2telmodel::TPointModelTerm *>::iterator iter = ((rts2telmodel::TPointModel*) model)->begin (); iter != ((rts2telmodel::TPointModel*) model)->end (); iter++)
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
	ret = rts2core::Device::initValues ();
	if (ret)
		return ret;

	ret = info ();
	if (ret)
		return ret;
	tarRaDec->setFromValue (telRaDec);
	setOri (telRaDec->getRa (), telRaDec->getDec ());
	objRaDec->setFromValue (telRaDec);
#ifndef RTS2_LIBERFA
	aberated->setFromValue (telRaDec);
	nutated->setFromValue (telRaDec);
	precessed->setFromValue (telRaDec);
	refraction->setValueDouble (NAN);
#endif
	modelRaDec->setValueRaDec (0, 0);
	telTargetRaDec->setFromValue (telRaDec);

	if (wcs_crval1 && wcs_crval2)
	{
		wcs_crval1->setValueDouble (objRaDec->getRa ());
		wcs_crval2->setValueDouble (objRaDec->getDec ());
	}

	// calculate rho_sin_phi, ...
	double r_s, r_c;
	lat_alt_to_parallax (ln_deg_to_rad (getLatitude ()), getAltitude (), &r_c, &r_s);

	tle_rho_cos_phi->setValueDouble (r_c);
	tle_rho_sin_phi->setValueDouble (r_s);

	updateDUT1 ();

	return ret;
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
			failedMove ();
			if (move_connection)
				sendInfo (move_connection);
			maskState (DEVICE_ERROR_MASK | TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE, DEVICE_ERROR_HW | TEL_NOT_CORRECTING | TEL_OBSERVING, "move finished with error");
			move_connection = NULL;
			setIdleInfoInterval (refreshIdle->getValueDouble ());
		}
		else if (ret == -2)
		{
			infoAll ();
			ret = endMove ();
			if (ret)
			{
				failedMove ();

				maskState (DEVICE_ERROR_MASK | TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE, DEVICE_ERROR_HW | TEL_NOT_CORRECTING | TEL_OBSERVING, "move finished with error");
			}
			else
			{
				maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE, TEL_NOT_CORRECTING | TEL_OBSERVING, "move finished without error");
			}

			if (move_connection)
			{
				sendInfo (move_connection);
			}
			move_connection = NULL;
			setIdleInfoInterval (refreshIdle->getValueDouble ());
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
			failedMove ();

			useParkFlipping = false;
			infoAll ();
			maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING | BOP_EXPOSURE, DEVICE_ERROR_HW | TEL_PARKED, "park command finished with error");
			setIdleInfoInterval (refreshIdle->getValueDouble ());
		}
		else if (ret == -2)
		{
			useParkFlipping = false;
			infoAll ();
			logStream (MESSAGE_INFO) << "parking of telescope finished" << sendLog;
			ret = endPark ();
			if (ret)
			{
				failedMove ();
				maskState (DEVICE_ERROR_MASK | TEL_MASK_MOVING | BOP_EXPOSURE, DEVICE_ERROR_HW | TEL_PARKED, "park command finished with error");
			}
			else
			{
				changeIdleMovingTracking ();
				maskState (TEL_MASK_MOVING | BOP_EXPOSURE, TEL_PARKED, "park command finished without error");
			}
			if (move_connection)
			{
				sendInfo (move_connection);
			}
			setIdleInfoInterval (refreshIdle->getValueDouble ());
		}
	}

	if ((getState () & TEL_MASK_OFFSETING) == TEL_OFFSETING)
	{
		ret = isOffseting();
		if (ret < 0)
		{
			if (targetDistanceStat != NULL)
				targetDistanceStat->clearStat ();
			maskState (TEL_MASK_OFFSETING, TEL_NO_OFFSETING, "offseting finished");
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
		case EVENT_TRACKING_TIMER:
			// if tracking is still relevant, reschedule
			if (isTracking ())
			{
				runTracking ();
				addTimer (trackingInterval->getValueFloat (), event);
				return;
			}
			// don't call stopMove - it shall be called if tracking was stop by stopTracking..
			break;
		case EVENT_CUP_SYNCED:
			maskState (TEL_MASK_CUP, TEL_NO_WAIT_CUP);
			break;
		case EVENT_CUP_ENDED:
			logStream (MESSAGE_INFO) << "removed cupola " << ((ClientCupola *)(event->getArg ()))->getName () << sendLog;
			cupolas->remove (((ClientCupola *)(event->getArg ()))->getName ());
			break;
		case EVENT_ROT_ENDED:
			logStream (MESSAGE_INFO) << "removed rotator " << ((ClientRotator *)(event->getArg ()))->getName () << sendLog;
			rotators->remove (((ClientRotator *)(event->getArg ()))->getName ());
			break;
	}
	rts2core::Device::postEvent (event);
}

int Telescope::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_CUPOLA || in_addr->getType () == DEVICE_TYPE_ROTATOR)
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
		case DEVICE_TYPE_ROTATOR:
			logStream (MESSAGE_INFO) << "connecting to rotator " << conn->getName () << sendLog;
			rotators->addValue (std::string (conn->getName ()));
			conn->setSendAll (false);
			return new ClientRotator (conn);
	}
	return rts2core::Device::createOtherType (conn, other_device_type);
}

void Telescope::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	// stop when STOP is triggered
	if ((getState () & STOP_MASK) != STOP_EVERYTHING && (new_state & STOP_MASK) == STOP_EVERYTHING)
	{
		blockMove->setValueBool (true);
		sendValueAll (blockMove);
		maskState (TEL_MASK_TRACK, TEL_NOTRACK, "tracking blocked");
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
		|| (new_state & SERVERD_ONOFF_MASK)) && standbyPark->getValueInteger () != 0)
	{
		// ignore nighttime park request
		if (standbyPark->getValueInteger () == 2 || ! ((new_state & SERVERD_STATUS_MASK) == SERVERD_NIGHT || (new_state & SERVERD_STATUS_MASK) == SERVERD_DUSK || (new_state & SERVERD_STATUS_MASK) == SERVERD_DAWN))
		{
	  		if ((getState () & TEL_MASK_MOVING) != TEL_PARKED && (getState () & TEL_MASK_MOVING) != TEL_PARKING)
				startPark (NULL);
		}
	}

	// update DUT1 during evening
	if (dut1fn != NULL && (new_state & SERVERD_STATUS_MASK) == SERVERD_EVENING)
	{
		requestDUT1();
	}

	if (blockOnStandby->getValueBool () == true)
	{
		if (new_state & SERVERD_ONOFF_MASK)
			blockMove->setValueBool (true);
		// only set blockMove to false if switching from off/standy
		else if (old_state & SERVERD_ONOFF_MASK)
			blockMove->setValueBool (false);
	}

	rts2core::Device::changeMasterState (old_state, new_state);
}

void Telescope::setTelLongLat (double longitude, double latitude)
{
	telLongitude->setValueDouble (longitude);
	telLatitude->setValueDouble (latitude);

	cos_lat = cos (ln_deg_to_rad (latitude));
	sin_lat = sin (ln_deg_to_rad (latitude));
	tan_lat = sin_lat / cos_lat;
}

void Telescope::setTelAltitude (float altitude)
{
	telAltitude->setValueDouble (altitude);
	telPressure->setValueFloat (getAltitudePressure (altitude, 1010));
}

int Telescope::loadModelStream (std::istream &is)
{
	delete model;
	model = new rts2telmodel::GPointModel (getLatitude ());
	model->load (is);
	if (is.fail ())
	{
		logStream (MESSAGE_ERROR) << "Failed to reload model from stream" << sendLog;
		delete model;
		model = NULL;
		return -1;
	}
	calModel->setValueBool (true);
	logStream (MESSAGE_INFO) << "Loaded model from stream" << sendLog;
	return 0;
}

void Telescope::getTelAltAz (struct ln_hrz_posn *hrz)
{
	struct ln_equ_posn telpos;
	struct ln_lnlat_posn observer;

	telpos.ra = telRaDec->getRa ();
	telpos.dec = telRaDec->getDec ();

	normalizeRaDec (telpos.ra, telpos.dec);

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	ln_get_hrz_from_equ_sidereal_time (&telpos, &observer, ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()), hrz);
}

double Telescope::estimateTargetTime ()
{
	// most of mounts move at 2 degrees per second.
	return getTargetDistance () / 2.0;
}

int Telescope::setTracking (int track, bool addTrackingTimer, bool send)
{
	if (tracking != NULL)
	{
		tracking->setValueInteger (track);
		// make sure we will not run two timers
		deleteTimers (EVENT_TRACKING_TIMER);
		if (track == 1 || track == 2)
		{
			maskState (TEL_MASK_TRACK, TEL_TRACKING, "tracking started");
			if (addTrackingTimer == true)
			{
				runTracking ();
				addTimer (trackingInterval->getValueFloat (), new rts2core::Event (EVENT_TRACKING_TIMER));
			}
		}
		else
		{
			stopTracking ();
		}
		if (send == true)
			sendValueAll (tracking);
	}
	return 0;
}

void Telescope::startTracking (bool check)
{
	if (tracking == NULL)
		return;

	if (check && tracking->getValueInteger () == 0)
	{
		tracking->setValueInteger (1);
		sendValueAll (tracking);
	}
	trackingFrequency->clearStat ();
	setTracking (tracking->getValueInteger (), true);
}

void Telescope::stopTracking (const char *msg)
{
	lastTrackingRun = NAN;
	stopMove ();
	maskState (TEL_MASK_TRACK, TEL_NOTRACK, msg);
}

void Telescope::runTracking ()
{
	double n = getNow ();
	if (lastTrackLog < n)
	{
		logTracking ();
		lastTrackLog = n + trackingLogInterval->getValueDouble ();
	}
	startCupolaSync ();
	trackingNum++;
}

void Telescope::logTracking ()
{
#ifdef RTS2_LIBERFA
	rts2core::LogStream ls = logStream (MESSAGE_DEBUG | DEBUG_MOUNT_TRACKING_SHORT_LOG);
		// 1                            2
	ls	<< tarRaDec->getRa () << " " << tarRaDec->getDec () << " "
		// 3                          4
		<< modelRaDec->getRa () << " " << modelRaDec->getDec () << " ";
#else
	rts2core::LogStream ls = logStream (MESSAGE_DEBUG | DEBUG_MOUNT_TRACKING_LOG);
		// 1                            2
	ls	<< tarRaDec->getRa () << " " << tarRaDec->getDec () << " "
		// 3                          4
		<< precessed->getRa () << " " << precessed->getDec () << " "
		// 5                           6
		<< nutated->getRa () << " " << nutated->getDec () << " "
		// 7                            8
		<< aberated->getRa () << " " << aberated->getDec () << " "
		// 9
		<< refraction->getValueDouble () << " "
		// 10                             11
		<< modelRaDec->getRa () << " " << modelRaDec->getDec () << " ";
#endif
	if (tarTelRaDec != NULL)
		//    12                              13
		ls << tarTelRaDec->getRa () << " " << tarTelRaDec->getDec () << " ";
	else
		ls << "-0 -0 ";

	if (tarTelAltAz != NULL)
		//    14                              15
		ls << tarTelAltAz->getAz () << " " << tarTelAltAz->getAlt ();
	else
		ls << "-0 -0 ";
	ls << sendLog;
}

void Telescope::updateTrackingFrequency ()
{
	double n = getNow ();
	if (!std::isnan (lastTrackingRun) && n != lastTrackingRun)
	{
		double tv = 1 / (n - lastTrackingRun);
		trackingFrequency->addValue (tv, trackingFSize->getValueInteger ());
		trackingFrequency->calculate ();
		if (tv < trackingWarning->getValueFloat ())
			logStream (MESSAGE_ERROR) << "lost telescope tracking, frequency is " << tv << sendLog;
	}
	lastTrackingRun = n;
}


int Telescope::moveTLE (const char *l1, const char *l2)
{
	int ret = parse_elements (l1, l2, &tle);
	if (ret != 0)
	{
		logStream (MESSAGE_ERROR) << "cannot target on TLEs" << sendLog;
		logStream (MESSAGE_ERROR) << "Line 1: " << l1 << sendLog;
		logStream (MESSAGE_ERROR) << "Line 2: " << l2 << sendLog;
		return DEVDEM_E_PARAMSVAL;
	}
	return 0;
}

int Telescope::parseTLE (rts2core::Connection *conn, const char *l1, const char *l2)
{
	int ret = moveTLE (l1, l2);
	if (ret)
		return ret;

	setTLE (l1, l2);

	int ephem = 1;

	int is_deep = select_ephemeris (&tle);
	if (is_deep && (ephem == 1 || ephem == 2))
		ephem += 2;	/* switch to an SDx */
	if (!is_deep && (ephem == 3 || ephem == 4))
		ephem -= 2;	/* switch to an SGx */
	tle_ephem->setValueInteger (ephem);

	startTracking (true);

	return startResyncMove (conn, 0);
}

void Telescope::setTLE (const char *l1, const char *l2)
{
	// we would like to always calculate next TLE position
	tle_freeze->setValueBool (false);

	mpec->setValueString ("");
	tle_l1->setValueString (l1);
	tle_l2->setValueString (l2);
}

void Telescope::calculateTLE (double JD, double &ra, double &dec, double &dist_to_satellite)
{
	double sat_params[N_SAT_PARAMS], observer_loc[3];
	double t_since;
	double sat_pos[3]; /* Satellite position vector */

	observer_cartesian_coords (JD, ln_deg_to_rad (getLongitude ()), tle_rho_cos_phi->getValueDouble (), tle_rho_sin_phi->getValueDouble (), observer_loc);

	t_since = (JD - tle.epoch) * 1440.;
	switch (tle_ephem->getValueInteger ())
	{
		case 0:
			SGP_init (sat_params, &tle);
			SGP (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 1:
			SGP4_init (sat_params, &tle);
			SGP4 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 2:
			SGP8_init (sat_params, &tle);
			SGP8 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 3:
			SDP4_init (sat_params, &tle);
			SDP4 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 4:
			SDP8_init (sat_params, &tle);
			SDP8 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		default:
			logStream (MESSAGE_ERROR) << "invalid tle_ephem " << tle_ephem->getValueInteger () << sendLog;
	}
	get_satellite_ra_dec_delta (observer_loc, sat_pos, &ra, &dec, &dist_to_satellite);
}

void Telescope::setDiffTrack (double dra, double ddec)
{
	if (dra == 0 && ddec == 0)
	{
		diffTrackStart->setValueDouble (NAN);
		diffTrackOn->setValueBool (false);
	}
	else
	{
		diffTrackStart->setValueDouble (ln_get_julian_from_sys ());
		diffTrackOn->setValueBool (true);
	}
	sendValueAll (diffTrackStart);
	sendValueAll (diffTrackOn);
}

void Telescope::setDiffTrackAltAz (double daz, double dalt)
{
	if (daz == 0 && dalt == 0)
	{
		diffTrackAltAzStart->setValueDouble (NAN);
		diffTrackAltAzOn->setValueBool (false);
	}
	else
	{
		diffTrackAltAzStart->setValueDouble (ln_get_julian_from_sys ());
		diffTrackAltAzOn->setValueBool (true);
	}
	sendValueAll (diffTrackAltAzStart);
	sendValueAll (diffTrackAltAzOn);
}

void Telescope::addParkPosOption ()
{
	addOption (OPT_PARK_POS, "park-position", 1, "parking position (alt:az[:flip])");
}

void Telescope::createParkPos (double alt, double az, int flip)
{
	createValue (parkPos, "park_position", "mount park position", false);
	createValue (parkFlip, "park_flip", "mount parked flip strategy", false);
	parkPos->setValueAltAz (alt, az);
	parkFlip->setValueInteger (flip);
}

void Telescope::getHrzFromEqu (struct ln_equ_posn *pos, double JD, struct ln_hrz_posn *hrz)
{
	struct ln_lnlat_posn obs;
	obs.lat = getLatitude ();
	obs.lng = getLongitude ();

	ln_get_hrz_from_equ_sidereal_time (pos, &obs, ln_get_apparent_sidereal_time(JD), hrz);
}

void Telescope::getHrzFromEquST (struct ln_equ_posn *pos, double ST, struct ln_hrz_posn *hrz)
{
	struct ln_lnlat_posn obs;
	obs.lat = getLatitude ();
	obs.lng = getLongitude ();

	ln_get_hrz_from_equ_sidereal_time (pos, &obs, ST, hrz);
}

void Telescope::getEquFromHrz (struct ln_hrz_posn *hrz, double JD, struct ln_equ_posn *pos)
{
	struct ln_lnlat_posn obs;
	obs.lat = getLatitude ();
	obs.lng = getLongitude ();

	ln_get_equ_from_hrz (hrz, &obs, JD, pos);
}

double Telescope::getAltitudePressure (double alt, double see_pres)
{
	return see_pres * pow (1 - (0.0065 * alt) / 288.15, (9.80665 * 0.0289644) / (8.31447 * 0.0065));
}

int Telescope::info ()
{
	double utc1, utc2;
#ifdef RTS2_LIBERFA
	getEraUTC (utc1, utc2);
#else
	utc1 = ln_get_julian_from_sys ();
	utc2 = 0;
#endif
	return infoUTC (utc1, utc2);
}

int Telescope::infoUTC (const double utc1, const double utc2)
{
	return infoUTCLST (utc1, utc2, getLstDeg (utc1, utc2));
}

int Telescope::infoLST (double telLST)
{
	double utc1, utc2;
#ifdef RTS2_LIBERFA
	getEraUTC (utc1, utc2);
#else
	utc1 = ln_get_julian_from_sys ();
	utc2 = 0;
#endif
	return infoUTCLST (utc1, utc2, telLST);
}

int Telescope::infoUTCLST (const double utc1, const double utc2, double telLST)
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
	jdVal->setValueDouble (utc1 + utc2);
	lst->setValueDouble (telLST);
	hourAngle->setValueDouble (ln_range_degrees (lst->getValueDouble () - telRaDec->getRa ()));

	double tdist = getTargetDistance ();
	targetDistance->setValueDouble (tdist);
	if (targetDistanceStat != NULL)
	{
		targetDistanceStat->addValue (tdist, trackingFSize->getValueInteger ());
		targetDistanceStat->calculate ();
	}

	// check if we aren't bellow hard horizon - if yes, stop tracking..
	if (hardHorizon)
	{
		struct ln_hrz_posn hrpos;
		hrpos.alt = telAltAz->getAlt ();
		hrpos.az = telAltAz->getAz ();
		if (!hardHorizon->is_good (&hrpos))
		{
			int ret = abortMoveTracking ();
			if (ret <= 0)
				logStream (MESSAGE_ERROR) << "info retrieved below horizon position, stop move. alt az: " << hrpos.alt << " " << hrpos.az << sendLog;
		}
		else
		{
			// we are at good position, above horizon
			telescopeAboveHorizon ();
		}
	}

	return rts2core::Device::info ();
}

int Telescope::scriptEnds ()
{
	woffsRaDec->setValueRaDec (0, 0);
	if (tracking)
		tracking->setValueInteger (1);

	tle_freeze->setValueBool (false);

	return rts2core::Device::scriptEnds ();
}

void Telescope::applyCorrections (struct ln_equ_posn *pos, double utc1, double utc2, struct ln_hrz_posn *hrz, bool writeValues)
{
#ifdef RTS2_LIBERFA
	double aob, zob, hob, dob, rob, eo, ri, di;

	double rc = ln_deg_to_rad (pos->ra);
	double dc = ln_deg_to_rad (pos->dec);

	eraASTROM astrom;

	int status = eraApco13 (utc1, utc2, telDUT1->getValueDouble (), ln_deg_to_rad (getLongitude ()), ln_deg_to_rad (getLatitude ()), getAltitude (), 0, 0, getPressure (), telAmbientTemperature->getValueFloat (), telHumidity->getValueFloat () / 100.0, telWavelength->getValueFloat () / 1000.0, &astrom, &eo);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "cannot apply corrections to " << pos->ra << " " << pos->dec << sendLog;
		return;
	}

	// transform ICRS to CIRS
	eraAtciq (rc, dc, ln_deg_to_rad (pmRaDec->getRa ()), ln_deg_to_rad (pmRaDec->getDec ()), 0, 0, &astrom, &ri, &di);

	// transform CISC to observed
	eraAtioq (ri, di, &astrom, &aob, &zob, &hob, &dob, &rob);

	pos->ra = ln_rad_to_deg (eraAnp (rob - eo));
	pos->dec = ln_rad_to_deg (dob);
	if (hrz != NULL)
	{
		// what is returned is 0 north, let's transform to IAU 0 south..
		hrz->az = ln_range_degrees (180 + ln_rad_to_deg (aob));
		hrz->alt = 90 - ln_rad_to_deg (zob);
	}
#else
	// apply all posible corrections
	if (calPrecession->getValueBool () == true)
		applyPrecession (pos, utc1 + utc2, writeValues);

	// always apply proper motion - if set
	struct ln_equ_posn pm;
	pm.ra = pmRaDec->getRa ();
	pm.dec = pmRaDec->getDec ();
	ln_get_equ_pm (pos, &pm, utc1 + utc2, pos);

	if (calNutation->getValueBool () == true)
		applyNutation (pos, utc1 + utc2, writeValues);
	if (calAberation->getValueBool () == true)
		applyAberation (pos, utc1 + utc2, writeValues);
	if (calRefraction->getValueBool () == true)
		applyRefraction (pos, utc1 + utc2, writeValues);

	if (hrz != NULL)
	{
		getHrzFromEqu (pos, utc1 + utc2, hrz);
	}
#endif
}

#ifdef RTS2_LIBERFA
void Telescope::getEraUTC (double &utc1, double &utc2)
{
	struct timeval tv;

	gettimeofday (&tv, NULL);

	struct tm *gt = gmtime (&(tv.tv_sec));

	int status = eraDtf2d ("UTC", gt->tm_year + 1900, gt->tm_mon + 1, gt->tm_mday, gt->tm_hour, gt->tm_min, gt->tm_sec + (double) tv.tv_usec / USEC_SEC, &utc1, &utc2);

	if (status)
		logStream (MESSAGE_ERROR) << "cannot get system time" << sendLog;
}
#endif

void Telescope::startCupolaSync ()
{
	double n = getNow ();
	if (nextCupSync > n)
		return;
	struct ln_equ_posn tar;
	getOrigin (&tar);
	rts2core::CommandCupolaSyncTel cmd (this, tar.ra, tar.dec);
	queueCommandForType (DEVICE_TYPE_CUPOLA, cmd, NULL, true);
	nextCupSync = n + 2;
}

int Telescope::endMove ()
{
	if (targetDistanceStat != NULL)
		targetDistanceStat->clearStat ();
	startTracking ();

	// this leads to 'moved to .... requested ... target ...' in logs
	logStream (INFO_MOUNT_SLEW_END | MESSAGE_INFO) << telRaDec->getRa () << " " << telRaDec->getDec () << " " << tarRaDec->getRa () << " " << tarRaDec->getDec () << " " << telTargetRaDec->getRa () << " " << telTargetRaDec->getDec () << sendLog;
	changeIdleMovingTracking ();
	return 0;
}

void Telescope::failedMove ()
{
	failedMoveNum->inc ();
	lastFailedMove->setNow ();
	changeIdleMovingTracking ();
}

int Telescope::abortMoveTracking ()
{
	stopTracking ("tracking below horizon");
	changeIdleMovingTracking ();
	return 0;
}

int Telescope::startResyncMove (rts2core::Connection * conn, int correction)
{
	int ret;

	struct ln_equ_posn pos;

	// apply computed corrections (precession, aberation, refraction)
	double utc1, utc2;
	useOEpoch = true;

#ifdef RTS2_LIBERFA
	getEraUTC (utc1, utc2);
#else
	utc1 = ln_get_julian_from_sys ();
	utc2 = 0;
#endif

	// calculate from MPEC..
	if (mpec->getValueString ().length () > 0)
	{
		struct ln_lnlat_posn observer;
		struct ln_equ_posn parallax;
		observer.lat = telLatitude->getValueDouble ();
		observer.lng = telLongitude->getValueDouble ();
		LibnovaCurrentFromOrbit (&pos, &mpec_orbit, &observer, telAltitude->getValueDouble (), ln_get_julian_from_sys (), &parallax);

		setOri (pos.ra, pos.dec);
		useOEpoch = false;
	}

	// calculate from TLE..
	if (tle_freeze->getValueBool () == false && tle_l1->getValueString ().length () > 0 && tle_l2->getValueString ().length () > 0)
	{
		double ra, dec, dist_to_satellite;
		calculateTLE (utc1 + utc2, ra, dec, dist_to_satellite);
		setOri (ln_rad_to_deg (ra), ln_rad_to_deg (dec));
		tle_distance->setValueDouble (dist_to_satellite);
		useOEpoch = false;
	}

	// if object was not specified, do not move
	if (std::isnan (oriRaDec->getRa ()) || std::isnan (oriRaDec->getDec ()))
	{
		logStream (MESSAGE_ERROR) << "cannot move, null RA or DEC " << oriRaDec->getRa () << " " << oriRaDec->getDec () << sendLog;
		return -1;
	}

	// MPEC objects changes coordinates regularly, so we will not bother incrementing
	if (mpec->getValueString ().length () > 0)
	{
		if (mpec->wasChanged ())
			incMoveNum ();
	}
	// same for TLEs
	else if (tle_l1->getValueString ().length () > 0)
	{
		if (tle_l1->wasChanged () || tle_l2->wasChanged ())
			incMoveNum ();
	}
	// object changed from last call to startResyncMove and it is a stationary object
	else if (oriRaDec->wasChanged ())
	{
		incMoveNum ();
	}

	// if some value is waiting to be applied..
	else if (wcorrRaDec->wasChanged ())
	{
		corrRaDec->incValueRaDec (wcorrRaDec->getRa (), wcorrRaDec->getDec ());
		corrImgId->setValueInteger (wCorrImgId->getValueInteger ());
		corrObsId->setValueInteger (wCorrObsId->getValueInteger ());
	}

	if (woffsRaDec->wasChanged ())
	{
		offsRaDec->setValueRaDec (woffsRaDec->getRa (), woffsRaDec->getDec ());
	}

	if (targetDistanceStat != NULL)
		targetDistanceStat->clearStat ();

	// update total_offsets
	total_offsets->setValueRaDec (offsRaDec->getRa () - corrRaDec->getRa (), offsRaDec->getDec () - corrRaDec->getDec ());
	sendValueAll (total_offsets);

	LibnovaRaDec l_obj (oriRaDec->getRa (), oriRaDec->getDec ());

	// first apply offset
	pos.ra = oriRaDec->getRa ();
	pos.dec = oriRaDec->getDec ();

	applyOffsets (&pos);

	pos.ra = ln_range_degrees (pos.ra);

	objRaDec->setValueRaDec (pos.ra, pos.dec);
	sendValueAll (objRaDec);

	setCRVAL (pos.ra, pos.dec);

	struct ln_hrz_posn hrztar;
	// apply computed corrections (precession, aberation, refraction)
	applyCorrections (&pos, utc1, utc2, &hrztar, true);

	LibnovaRaDec syncTo (&pos);
	LibnovaRaDec syncFrom (telRaDec->getRa (), telRaDec->getDec ());

	// now we have target position, which can be fed to telescope (but still without astrometry-feedback corrections and model!)
	tarRaDec->setValueRaDec (pos.ra, pos.dec);

	// calculate target after corrections from astrometry
	applyCorrRaDec (&pos, false, false);
	// set telTargetRaDec (i.e. tel_target) - must be set here for mounts, which use RA/DEC coordinates and don't integrate modelling (e.g. lx200)
	telTargetRaDec->setValueRaDec (pos.ra, pos.dec);
	modelRaDec->setValueRaDec (0, 0);

	moveInfoCount = 0;

	// The following sequence is:
	// - check block_move (don't touch anything if block_move set)
	// - check TEL_PARKING (when true, return with error, we don't interrupt parking process)
	// - check TEL_MOVING and stop when needed
	// - check hardHorizon (when not fulfiled, stop with error, change state to observing)
	// - maskState TEL_MOVING | BOP_EXPOSURE
	// - startResync ()
	// This way the exposure blocking state is set ASAP and all resulting states are corresponging with real mount states (mount stopped, when needed etc).

	if (blockMove->getValueBool () == true)
	{
		logStream (MESSAGE_ERROR) << "telescope move blocked" << sendLog;
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "telescope move blocked");
		return -1;
	}

	if (parkingBlockMove && (getState () & TEL_MASK_MOVING) == TEL_PARKING)
	{
		logStream (MESSAGE_ERROR) << "telescope cannot move during parking" << sendLog;
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "telescope move blocked");
		return -1;
	}

	if (((getState () & TEL_MASK_MOVING) == TEL_MOVING) || ((getState () & TEL_MASK_CORRECTING) == TEL_CORRECTING))
	{
		ret = stopMove ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "failure calling stopMove before starting actual movement" << sendLog;
			return ret;
		}
	}

	if (hardHorizon)
	{
		if (!hardHorizon->is_good (&hrztar))
		{
			useParkFlipping = false;
			logStream (MESSAGE_ERROR) << "target is below hard horizon: alt az " << LibnovaHrz (&hrztar) << sendLog;
			setOri (NAN, NAN);
			objRaDec->setValueRaDec (NAN, NAN);
			tarRaDec->setValueRaDec (NAN, NAN);
			stopMove ();
			maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE | TEL_MASK_TRACK, TEL_NOT_CORRECTING | TEL_OBSERVING | TEL_NOTRACK, "cannot perform move");
			if (conn)
				conn->sendCommandEnd (DEVDEM_E_HW, "unaccesible target");
			return -1;
		}
	}

	//check if decupperlimit option exists -if yes, apply declination constraints
	if (decUpperLimit && !std::isnan (decUpperLimit->getValueFloat ()))
	{
		if ((telLatitude->getValueDouble () > 0 && pos.dec > decUpperLimit->getValueFloat ())
				|| (telLatitude->getValueDouble () < 0 && pos.dec < decUpperLimit->getValueFloat ()))
		{
			useParkFlipping = false;
			logStream (MESSAGE_ERROR) << "target declination is outside of allowed values, is " << pos.dec << " limit is " << decUpperLimit->getValueFloat () << sendLog;
			setOri (NAN, NAN);
			objRaDec->setValueRaDec (NAN, NAN);
			tarRaDec->setValueRaDec (NAN, NAN);
			stopMove ();
			maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE | TEL_MASK_TRACK, TEL_NOT_CORRECTING | TEL_OBSERVING | TEL_NOTRACK, "cannot perform move");
			if (conn)
				conn->sendCommandEnd (DEVDEM_E_HW, "declination limit violated");
			return -1;
		}
	}

	// all checks done, give proper info and set TEL_ state and BOP
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

		struct ln_hrz_posn hrz;
		getTelAltAz (&hrz);

		logStream (INFO_MOUNT_SLEW_START | MESSAGE_INFO) << telRaDec->getRa () << " " << telRaDec->getDec () << " " << pos.ra << " " << pos.dec << " " << hrz.az << " " << hrz.alt << " " << hrztar.az << " " << hrztar.alt << sendLog;
		maskState (TEL_MASK_MOVING | TEL_MASK_CORRECTING | TEL_MASK_NEED_STOP | TEL_MASK_TRACK | BOP_EXPOSURE, TEL_MOVING | BOP_EXPOSURE, "move started");
		flip_move_start = telFlip->getValueInteger ();
	}

	// everything is OK and prepared, let's move!
	ret = startResync ();
	if (ret)
	{
		useParkFlipping = false;
		maskState (TEL_MASK_CORRECTING | TEL_MASK_MOVING | BOP_EXPOSURE | TEL_MASK_TRACK, TEL_NOT_CORRECTING | TEL_OBSERVING, "movement failed");
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot move to location");
		return ret;
	}

	targetStarted->setNow ();
	targetReached->setValueDouble (targetStarted->getValueDouble () + estimateTargetTime ());

	infoAll ();

	// synchronize cupola...
	afterMovementStart ();

	tarRaDec->resetValueChanged ();
	oriRaDec->resetValueChanged ();
	offsRaDec->resetValueChanged ();
	corrRaDec->resetValueChanged ();
	telTargetRaDec->resetValueChanged ();
	mpec->resetValueChanged ();
	tle_l1->resetValueChanged ();
	tle_l2->resetValueChanged ();

	if (woffsRaDec->wasChanged ())
	{
		woffsRaDec->resetValueChanged ();
	}

	if (wcorrRaDec->wasChanged ())
	{
		wcorrRaDec->setValueRaDec (0, 0);
		wcorrRaDec->resetValueChanged ();
	}

	move_connection = conn;

	setIdleInfoInterval (refreshSlew->getValueDouble ());

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
	resetMpecTLE ();
	maskState (TEL_MASK_TRACK | TEL_MASK_OFFSETING | TEL_MASK_UNSTABLE, TEL_NOTRACK | TEL_NO_OFFSETING | TEL_STABLE , "stop tracking while telescope is being parked");
	if ((getState () & TEL_MASK_MOVING) == TEL_MOVING)
	{
		ret = stopMove ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "failure calling stopMove before starting parking" << sendLog;
			return ret;
		}
	}
	useParkFlipping = true;
	if (tarTelAltAz && parkPos)
		tarTelAltAz->setFromValue (parkPos);
	stopTracking ("parking telescope");
	changeIdleMovingTracking ();
	ret = startPark ();
	if (ret < 0)
	{
		useParkFlipping = false;
		if (conn)
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot park");
		logStream (MESSAGE_ERROR) << "parking failed" << sendLog;
	}
	else
	{
	  	logStream (MESSAGE_INFO) << "parking telescope" << sendLog;
		if (ret == 0)
		{
			offsRaDec->resetValueChanged ();
			corrRaDec->resetValueChanged ();
		}

		incMoveNum ();
		setParkTimeNow ();
		afterParkingStart ();
		maskState (TEL_MASK_MOVING | TEL_MASK_TRACK | TEL_MASK_NEED_STOP, TEL_PARKING, "parking started");
	}

	setIdleInfoInterval (refreshSlew->getValueDouble ());

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
	if (tPointModelFile)
	{
		delete model;
		model = new rts2telmodel::TPointModel (getLatitude ());
		ret = model->load (tPointModelFile);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Failed to reload model from file " << tPointModelFile << sendLog;
			delete model;
			model = NULL;
		}
		else
		{
			logStream (MESSAGE_INFO) << "Reloaded model from file " << tPointModelFile << sendLog;
		}
	}
	if (rts2ModelFile)
	{
		delete model;
		model = new rts2telmodel::GPointModel (getLatitude ());
		ret = model->load (rts2ModelFile);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Failed to reload model from file " << rts2ModelFile << sendLog;
			delete model;
			model = NULL;
		}
		else
		{
			logStream (MESSAGE_INFO) << "Reloaded model from file " << rts2ModelFile << sendLog;
		}
	}

	rts2core::Device::signaledHUP ();
}

void Telescope::startOffseting (rts2core::Value *changed_value)
{
	startResyncMove (NULL, 0);
}

int Telescope::peek (double ra, double dec)
{
	return 0;
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

	setOri (target.ra, target.dec);
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
			return DEVDEM_E_PARAMSNUM;
		modelOn ();
		setOri (obj_ra, obj_dec);
		resetMpecTLE ();
		startTracking (true);
		ret = startResyncMove (conn, 0);
		if (ret)
			maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking, move cannot be perfomed");
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_MOVE_PM))
	{
		double pmRa, pmDec;
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || conn->paramNextDouble (&pmRa) || conn->paramNextDouble (&pmDec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		modelOn ();
		setOri (obj_ra, obj_dec, 2000.0, pmRa / 3600.0, pmDec / 3600.0);
		resetMpecTLE ();
		startTracking (true);
		ret = startResyncMove (conn, 0);
		if (ret)
			maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking, move cannot be perfomed");
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_MOVE_EPOCH))
	{
		double epoch;
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || conn->paramNextDouble (&epoch))
			return DEVDEM_E_PARAMSNUM;

		modelOn ();
		setOri (obj_ra, obj_dec, epoch, 0, 0);
		resetMpecTLE ();
		startTracking (true);
		ret = startResyncMove (conn, 0);
		if (ret)
			maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking, move cannot be perfomed");
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_MOVE_EPOCH_PM))
	{
		double epoch, pmRa, pmDec;
		pmRa = 0;
		pmDec = 0;
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || conn->paramNextDouble (&epoch) || conn->paramNextDouble (&pmRa) || conn->paramNextDouble (&pmDec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;

		modelOn ();
		setOri (obj_ra, obj_dec, epoch, pmRa / 3600.0, pmDec / 3600.0);
		resetMpecTLE ();
		startTracking (true);
		ret = startResyncMove (conn, 0);
		if (ret)
			maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking, move cannot be perfomed");
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_HADEC))
	{
		if (conn->paramNextHMS (&obj_ha) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		double JD = ln_get_julian_from_sys ();
		obj_ra = getLocSidTime ( JD) * 15. - obj_ha ;

		setOri (obj_ra, obj_dec);
		resetMpecTLE ();
		tarRaDec->setValueRaDec (NAN, NAN);
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("move_not_model"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		modelOff ();
		setOri (obj_ra, obj_dec);
		resetMpecTLE ();
		startTracking (true);
		ret = startResyncMove (conn, 0);
		if (ret)
			maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking, move cannot be perfomed");
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_MOVE_MPEC))
	{
		char *str;
		if (conn->paramNextString (&str) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		modelOn ();
		mpec->setValueString (str);
		std::string desc;
		if (LibnovaEllFromMPC (&mpec_orbit, desc, str))
			if (LibnovaEllFromMPCComet (&mpec_orbit, desc, str))
				return DEVDEM_E_PARAMSVAL;
		tle_l1->setValueString ("");
		tle_l2->setValueString ("");
		startTracking (true);
		ret = startResyncMove (conn, 0);
		if (ret)
			maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking, move cannot be perfomed");
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_PEEK))
	{
		if (conn->paramNextDMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		return peek (obj_ra, obj_dec) == 0 ? DEVDEM_OK : DEVDEM_E_PARAMSVAL;
	}
	else if (conn->isCommand (COMMAND_TELD_ALTAZ))
	{
		if (conn->paramNextDMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		telAltAz->setValueAltAz (obj_ra, obj_dec);
		resetMpecTLE ();
		maskState (TEL_MASK_TRACK, TEL_NOTRACK, "stop tracking while in AltAz");
		return moveAltAz () == 0 ? DEVDEM_OK : DEVDEM_E_PARAMSVAL;
	}
	else if (conn->isCommand ("resync"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		setOri (obj_ra, obj_dec);
		resetMpecTLE ();
		return startResyncMove (conn, 0);
	}
	else if (conn->isCommand ("setto"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		modelOn ();
		return setTo (conn, obj_ra, obj_dec);
	}
	else if (conn->isCommand ("settopark"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		return setToPark (conn);
	}
	else if (conn->isCommand ("synccorr"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		ret = setTo (conn, tarRaDec->getRa (), tarRaDec->getDec ());
		if (ret)
			return DEVDEM_E_PARAMSVAL;
		corrRaDec->setValueRaDec (0, 0);
		wcorrRaDec->setValueRaDec (0, 0);
		return ret;
	}
	else if (conn->isCommand ("correct"))
	{
		int cor_mark;
		int corr_img;
		int corr_obs;
		int img_id;
		int obs_id;
		double total_cor_ra;
		double total_cor_dec;
		double pos_err;
		if (conn->paramNextInteger (&cor_mark)
			|| conn->paramNextInteger (&corr_img)
			|| conn->paramNextInteger (&corr_obs)
			|| conn->paramNextInteger (&img_id)
			|| conn->paramNextInteger (&obs_id)
			|| conn->paramNextDouble (&total_cor_ra)
			|| conn->paramNextDouble (&total_cor_dec)
			|| conn->paramNextDouble (&pos_err)
			|| !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;

		logStream (MESSAGE_DEBUG) << "correct " << cor_mark << "=" << moveNum->getValueInteger () << "   "
								 << corr_img << ":" << corr_obs  << "=" << corrImgId->getValueInteger () << ":" << corrObsId->getValueInteger () << "   "
								 << img_id << ":" << obs_id << ">" << wCorrImgId->getValueInteger () << ":" << wCorrObsId->getValueInteger () << "   "
								 << "cor_ra " << total_cor_ra << " cor_dec " << total_cor_dec << " err " << pos_err << sendLog;

		if (applyCorrectionsFixed (total_cor_ra, total_cor_dec) == 0)
			return DEVDEM_OK;
		if (cor_mark == moveNum->getValueInteger () &&
			corr_img == corrImgId->getValueInteger () && corr_obs == corrObsId->getValueInteger () &&
			(obs_id > wCorrObsId->getValueInteger () || img_id > wCorrImgId->getValueInteger ()))
		{
			if (pos_err < ignoreCorrection->getValueDouble ())
			{
				conn->sendCommandEnd (DEVDEM_E_IGNORE, "ignoring correction as it is too small");
				return DEVDEM_E_COMMAND;
			}

			wcorrRaDec->setValueRaDec (total_cor_ra, total_cor_dec);

			posErr->setValueDouble (pos_err);
			sendValueAll (posErr);

			wCorrImgId->setValueInteger (img_id);
			sendValueAll (wCorrImgId);
			wCorrObsId->setValueInteger (obs_id);
			sendValueAll (wCorrObsId);

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
		_os << "ignoring correction - cor_mark " << cor_mark << " moveNum " << moveNum->getValueInteger () << " corr_img " << corr_img << ":" << corr_obs << " corrImgId " << corrImgId->getValueInteger () << ":" << corrObsId->getValueInteger () << " img_id " << img_id << ":" << obs_id << " wCorrImgId " << wCorrImgId->getValueInteger () << ":" << wCorrObsId->getValueInteger ();
		conn->sendCommandEnd (DEVDEM_E_IGNORE, _os.str ().c_str ());
		return -1;
	}
	else if (conn->isCommand (COMMAND_TELD_MOVE_TLE))
	{
		char *l1;
		char *l2;
		if (conn->paramNextString (&l1) || conn->paramNextString (&l2) || ! conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;

		return parseTLE (conn, l1, l2);
	}
	else if (conn->isCommand (COMMAND_TELD_PARK))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		modelOn ();
		return startPark (conn);
	}
	else if (conn->isCommand ("stop"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		maskState (TEL_MASK_TRACK, TEL_NOTRACK, "tracking stopped");
		return stopMove ();
	}
	else if (conn->isCommand ("change"))
	{
		if (conn->paramNextHMS (&obj_ra) || conn->paramNextDMS (&obj_dec)
			|| !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		offsRaDec->incValueRaDec (obj_ra, obj_dec);
		woffsRaDec->incValueRaDec (obj_ra, obj_dec);
		return startResyncMove (conn, 1);
	}
	else if (conn->isCommand ("save_model"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		ret = saveModel ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot save model");
		}
		return ret;
	}
	else if (conn->isCommand ("load_model"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		ret = loadModel ();
		if (ret)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot load model");
		}
		return ret;
	}
	else if (conn->isCommand ("worm_stop"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		stopMove ();
		maskState (TEL_MASK_TRACK, TEL_NOTRACK, "tracking stopped by worm_stop command");
		return 0;
	}
	else if (conn->isCommand ("worm_start"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		if (blockMove->getValueBool () == true || (getState () & TEL_MASK_MOVING) == TEL_PARKING || (getState () & TEL_MASK_MOVING) == TEL_PARKED)
		{
			ret = -1;
		}
		else
		{
			if (tracking != NULL && tracking->getValueInteger () == 0)
				tracking->setValueInteger (1);
			ret = setTracking (tracking->getValueInteger (), true);
		}

		if (ret == -1)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot start worm");
		}
		return ret;
	}
	else if (conn->isCommand ("reset"))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		ret = resetMount ();
		if (ret == DEVDEM_E_COMMAND)
		{
			conn->sendCommandEnd (DEVDEM_E_HW, "cannot reset mount");
		}
		return ret;
	}
	else if (conn->isCommand (COMMAND_TELD_GETDUT1))
	{
		if (!conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		if (dut1fn == NULL)
		{
			conn->sendCommandEnd (DEVDEM_E_IGNORE, "DUT1 file was not provided");
			return DEVDEM_E_IGNORE;
		}
		return requestDUT1 ();
	}
	else if (conn->isCommand (COMMAND_TELD_WEATHER))
	{
		double temp = NAN;
		double hum = NAN;
		double pres = NAN;
		if (conn->paramNextDouble (&temp) || conn->paramNextDouble (&hum) || conn->paramNextDouble (&pres) || !conn->paramEnd ())
			return DEVDEM_E_PARAMSNUM;
		if (!std::isnan (temp))
			telAmbientTemperature->setValueDouble (temp);
		if (!std::isnan (hum))
			telHumidity->setValueDouble (hum);
		if (!std::isnan (pres))
			telPressure->setValueDouble (pres);
		return 0;
	}

	return rts2core::Device::commandAuthorized (conn);
}

void Telescope::setFullBopState (rts2_status_t new_state)
{
	rts2core::Device::setFullBopState (new_state);
	if ((woffsRaDec->wasChanged () || wcorrRaDec->wasChanged ()) && !(new_state & BOP_TEL_MOVE))
		startResyncMove (NULL, (woffsRaDec->wasChanged () ? 1 : 0) | (wcorrRaDec->wasChanged () ? 2 : 0));
}

void Telescope::setOri (double obj_ra, double obj_dec, double epoch, double pmRa, double pmDec)
{
	oriRaDec->setValueRaDec (obj_ra, obj_dec);
	oriEpoch->setValueDouble (epoch);
	pmRaDec->setValueRaDec (pmRa, pmDec);
}

void Telescope::resetMpecTLE ()
{
	mpec->setValueString ("");
	tle_l1->setValueString ("");
	tle_l2->setValueString ("");
}

void Telescope::updateDUT1 ()
{
	if (dut1fn == NULL)
		return;

	time_t now;

	time (&now);
	struct tm *gmt = gmtime (&now);

	double dut1 = getDUT1 (dut1fn, gmt);
	if (!std::isnan (dut1))
	{
		telDUT1->setValueDouble (dut1);
		valueGood (telDUT1);
	}
	else
	{
		valueError (telDUT1);
	}
	sendValueAll (telDUT1);
}

int Telescope::requestDUT1 ()
{
	std::string backup (dut1fn);
	backup += ".1";
	unlink (backup.c_str ());
	rename (dut1fn, backup.c_str ());
	if (retrieveDUT1 (dut1fn))
	{
		logStream (MESSAGE_ERROR) << "cannot retrieve DUt1 file" << sendLog;
		return DEVDEM_E_HW;
	}
	updateDUT1 ();
	if (telDUT1->isError ())
	{
		logStream (MESSAGE_ERROR) << "wrong DUT, reverting to old DUT1 file" << sendLog;
		rename (backup.c_str (), dut1fn);
		updateDUT1 ();
		return DEVDEM_E_HW;
	}
	logStream (MESSAGE_INFO) << "update DUT1 file from the internet" << sendLog;
	return DEVDEM_OK;
}
