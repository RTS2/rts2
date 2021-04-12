/*
 * Abstract class for Alt-Az mounts.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

#include "altaz.h"
#include "configuration.h"

#include "libnova_cpp.h"
#include <libnova/libnova.h>

using namespace rts2teld;

AltAz::AltAz (int in_argc, char **in_argv, bool diffTrack, bool hasTracking, bool hasUnTelCoordinates, bool hasAltAzDiff, bool parkingBlock):Telescope (in_argc, in_argv, diffTrack, hasTracking, hasUnTelCoordinates ? -1 : 0, hasAltAzDiff, parkingBlock, true)
{
	createValue (parallAngle, "PANGLE", "[deg] parallactic angle", true, RTS2_DT_DEGREES);
	createValue (derRate, "DERRATE", "[deg/hour] derotator rate", false, RTS2_DT_DEGREES);

	createValue (meanParallAngle, "mean_pa", "[deg] mean parallactic angle", false, RTS2_DT_DEGREES);

	createValue (az_ticks, "_az_ticks", "[cnts] counts per full revolution on az axis", false);
	createValue (alt_ticks, "_alt_ticks", "[cnts] counts per full revolution on alt axis", false);

	createValue (azZero, "_az_zero", "[deg] azimuth zero offset", false);
	createValue (zdZero, "_zd_zero", "[deg] zenith zero offset", false);

	createValue (azCpd, "_az_cpd", "[cnts] azimuth counts per azimth degree", false);
	createValue (altCpd, "_alt_cpd", "[cnts] altitude counts per azimth degree", false);

	createValue (azMin, "_az_min", "[cnts] minimal azimuth axis count", false);
	createValue (azMax, "_az_max", "[cnts] maximal azimuth axis count", false);
	createValue (altMin, "_alt_min", "[cnts] minimal altitude axis count", false);
	createValue (altMax, "_alt_max", "[cnts] maximal altitude axis count", false);

	createValue (azSlewMargin, "az_slew_margin", "[deg] azimuth slew margin", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	azSlewMargin->setValueDouble (0);

	setPointingModel (1);

	nextParUpdate = 0;
}

AltAz::~AltAz (void)
{
}

int AltAz::infoUTCLST (const double utc1, const double utc2, double LST)
{
	int ret = Telescope::infoUTCLST (utc1, utc2, LST);
	if (ret)
		return ret;

	struct ln_equ_posn tar;
	struct ln_hrz_posn hrz;

	getTarget (&tar);
	getModelTarAltAz (&hrz);

	double HA = ln_range_degrees (LST - tar.ra);
	double dec = tar.dec;

	double pa = 0;
	double parate = 0;
	meanParallacticAngle (HA, dec, pa, parate);
	meanParallAngle->setValueDouble (pa);
	derRate->setValueDouble (parate);

	effectiveParallacticAngle (utc1, utc2, &tar, &hrz, pa, parate, 0.01);
	parallAngle->setValueDouble (pa);

	return ret;
}

int AltAz::calculateMove (double JD, int32_t c_azc, int32_t c_altc, int32_t &t_azc, int32_t &t_altc)
{
	struct ln_hrz_posn hrz, u_hrz;
	// check if both start and end are above horizon
	counts2hrz (c_azc, c_altc, hrz.az, hrz.alt, u_hrz.az, u_hrz.alt);
	if (!isGood (&hrz))
		return -1;
	counts2hrz (t_azc, t_altc, hrz.az, hrz.alt, u_hrz.az, u_hrz.alt);
	if (!isGood (&hrz))
		return -2;
	return 0;
}

int AltAz::sky2counts (const double utc1, const double utc2, struct ln_equ_posn *pos, struct ln_hrz_posn *hrz_out, int32_t &azc, int32_t &altc, bool writeValue, double haMargin, bool forceShortest)
{
	struct ln_equ_posn tar_pos;
	tar_pos.ra = pos->ra;
	tar_pos.dec = pos->dec;

	struct ln_hrz_posn hrz;

	applyCorrections (&tar_pos, utc1, utc2, &hrz, writeValue);

	hrz_out->alt = hrz.alt;
	hrz_out->az = hrz.az;

	int used_flipping = 0; // forceShortes ? 0 : flipping->getValueInteger ();
        bool use_flipped;

	struct ln_hrz_posn model_change;

	applyModelAltAz (hrz_out, pos, &model_change);

	int ret = hrz2counts (hrz_out, azc, altc, used_flipping, use_flipped, writeValue, haMargin, false);
	if (ret)
		return ret;

	if (writeValue)
	{
		setTelTarget (tar_pos.ra, tar_pos.dec);
		setTarTelAltAz (&hrz);
		setModelTarAltAz (hrz_out);
	}
	return 0;
}

int AltAz::hrz2counts (struct ln_hrz_posn *hrz, int32_t &azc, int32_t &altc, int used_flipping, bool &use_flipped, bool writeValue, double haMargin, int az_zero)
{
	applyAltAzOffsets (hrz);

	int32_t d_alt = altc - ((90 - hrz->alt) - zdZero->getValueDouble ()) * altCpd->getValueDouble ();
	int32_t d_az = azc - (hrz->az - azZero->getValueDouble ()) * azCpd->getValueDouble ();

	d_alt %= alt_ticks->getValueLong ();
	d_az %= az_ticks->getValueLong ();

	if (d_alt > alt_ticks->getValueLong () / 2.0)
		d_alt -= alt_ticks->getValueLong ();
	else if (d_alt < -alt_ticks->getValueLong () / 2.0)
		d_alt += alt_ticks->getValueLong ();

	if (d_az > az_ticks->getValueLong () / 2.0)
		d_az -= az_ticks->getValueLong ();
	else if (d_az < -az_ticks->getValueLong () / 2.0)
		d_az += az_ticks->getValueLong ();

	int32_t t_alt = altc - d_alt;
	int32_t t_az = azc - d_az;

	// put target closer to 0 than az_ticks / 2.0
	if (az_zero < 0)
	{
		while (t_az > az_ticks->getValueLong () / 2.0)
			t_az -= az_ticks->getValueLong ();
		while (t_az < -az_ticks->getValueLong () / 2.0)
			t_az += az_ticks->getValueLong ();
	}
	else if (az_zero > 0)
	{
		while (t_az > az_ticks->getValueLong ())
			t_az -= az_ticks->getValueLong ();
		while (t_az < 0)
			t_az += az_ticks->getValueLong ();
	}

	while (t_az < azMin->getValueLong ())
		t_az += az_ticks->getValueLong ();
	while (t_az > azMax->getValueLong ())
		t_az -= az_ticks->getValueLong ();

	if (t_alt < altMin->getValueLong () || t_alt > altMax->getValueLong ())
		return -1;
	
	if (t_az < azMin->getValueLong () || t_az > azMax->getValueLong ())
		return -1;

	azc = t_az;
	altc = t_alt;

	return 0;
}

void AltAz::counts2hrz (int32_t azc, int32_t altc, double &az, double &alt, double &un_az, double &un_zd)
{
	un_az = azc / azCpd->getValueDouble () + azZero->getValueDouble ();
	un_zd = altc / altCpd->getValueDouble () + zdZero->getValueDouble ();

	az = ln_range_degrees (un_az);
	alt = ln_range_degrees (90 - un_zd);

	reverseAltAzOffsets (alt, az);

	if (alt > 270)
	{
		alt = -360 + alt;
	}
	else if (alt > 90)
	{
		alt = 180 - alt;
		az = ln_range_degrees (az + 180);
	}
}

void AltAz::counts2sky (double JD, int32_t azc, int32_t altc, double &ra, double &dec)
{
	struct ln_hrz_posn hrz;
	double un_az, un_zd;
	counts2hrz (azc, altc, hrz.az, hrz.alt, un_az, un_zd);
	struct ln_equ_posn pos;
	getEquFromHrz (&hrz, JD, &pos);
	ra = pos.ra;
	dec = pos.dec;
}

void AltAz::meanParallacticAngle (double ha, double dec, double &pa, double &parate)
{
	parallacticAngle (ha, dec, sin_lat, cos_lat, tan_lat, pa, parate);
}

void AltAz::effectiveParallacticAngle (const double utc1, const double utc2, struct ln_equ_posn *pos, struct ln_hrz_posn *hrz, double &pa, double &parate, double up_change)
{
	struct ln_equ_posn pos_up;
	struct ln_hrz_posn hrz_up;
	pos_up.ra = pos->ra;
	double fabs_up = up_change;
	pos_up.dec = pos->dec + (getLatitude () > 0 ? fabs_up : -fabs_up);
	int32_t up_ac, up_dc;
	sky2counts (utc1, utc2, &pos_up, &hrz_up, up_ac, up_dc, false, 0, true);

	struct ln_equ_posn t1, t2;
	t1.ra = hrz->az;
	t1.dec = hrz->alt;
	t2.ra = hrz_up.az;
	t2.dec = hrz_up.alt;

	pa = ln_get_rel_posn_angle (&t1, &t2);
}

int AltAz::checkTrajectory (double JD, int32_t azc, int32_t altc, int32_t &azt, int32_t &altt, int32_t azs, int32_t alts, unsigned int steps, double alt_margin, double az_margin, bool ignore_soft_beginning)
{
	int32_t t_az = azc;
	int32_t t_alt = altc;

	int32_t step_az = azs;
	int32_t step_alt = alts;

	int32_t soft_az = azc;
	int32_t soft_alt = altc;

	bool hard_beginning = false;

	if (azc > azt)
		step_az = -azs;

	if (altc > altt)
		step_alt = -alts;

	// turned to true if we are in "soft" boundaries, e.g hit with margin applied
	bool soft_hit = false;

	for (unsigned int c = 0; c < steps; c++)
	{
		// check if still visible
		struct ln_hrz_posn hrz, u_hrz;

		int32_t n_az;
		int32_t n_alt;

		// if we already reached destionation, e.g. currently computed position is within step to target, don't go further..
		if (labs (t_az - azt) < azs)
		{
			n_az = azt;
			step_az = 0;
		}
		else
		{
			n_az = t_az + step_az;
		}

		if (labs (t_alt - altt) < alts)
		{
			n_alt = altt;
			step_alt = 0;
		}
		else
		{
			n_alt = t_alt + step_alt;
		}

		counts2hrz (n_az, n_alt, hrz.az, hrz.alt, u_hrz.az, u_hrz.alt);

		// uncomment to see which checks are being performed
		//std::cerr << "checkTrajectory hrz " << n_a << " " << n_d << " hrz alt az " << hrz.alt << " " << hrz.az << std::endl;

		if (soft_hit == true || ignore_soft_beginning == true)
		{
			// if we really cannot go further
			if (hardHorizon.is_good (&hrz) == 0)
			{
				// even at hard hit on first step, let's see if it can move out of limits
				if (c == 0) 
				{
					logStream (MESSAGE_WARNING) << "below hard limit, see if we can move above in a few steps" << sendLog;
					hard_beginning = true;
				}
				else if (hard_beginning == false || c > 20)
				{
					logStream (MESSAGE_DEBUG) << "hit hard limit at alt az " << hrz.alt << " " << hrz.az << " " << soft_az << " " << soft_alt << " " << n_az << " " << n_alt << sendLog;
					if (soft_hit == true)
					{
						// then use last good position, and return we reached horizon..
						azt = soft_az;
						altt = soft_alt;
						return 2;
					}
					else
					{
						// case when moving within soft will lead to hard hit..we don't want this path
						azt = t_az;
						altt = t_alt;
						return 3;
					}
					hard_beginning = false;
				}
			}
			else
			{
				hard_beginning = false;
			}
		}

		// we don't need to move anymore, full trajectory is valid
		if (step_az == 0 && step_alt == 0)
			return 0;

		if (soft_hit == false && hard_beginning == false)
		{
			// check soft margins..
			if (hardHorizon.is_good_with_margin (&hrz, alt_margin, az_margin) == 0)
			{
				if (ignore_soft_beginning == false)
				{
					soft_hit = true;
					soft_az = t_az;
					soft_alt = t_alt;
				}
			}
			else
			{
				// we moved away from soft hit region
				if (ignore_soft_beginning == true)
				{
					ignore_soft_beginning = false;
					soft_az = t_az;
					soft_alt = t_alt;
				}
			}
		}

		t_az = n_az;
		t_alt = n_alt;
	}

	if (soft_hit == true)
	{
		azt = soft_az;
		altt = soft_alt;
	}
	else
	{
		azt = t_az;
		altt = t_alt;
	}

	// we are fine to move at least to the given coordinates
	return 1;
}

void AltAz::unlockPointing ()
{
	az_ticks->setWritable ();
	alt_ticks->setWritable ();

	azZero->setWritable ();
	zdZero->setWritable ();
	azCpd->setWritable ();
	altCpd->setWritable ();

	azMin->setWritable ();
	azMax->setWritable ();
	altMin->setWritable ();
	altMax->setWritable ();

	updateMetaInformations (az_ticks);
	updateMetaInformations (alt_ticks);

	updateMetaInformations (azZero);
	updateMetaInformations (zdZero);
	updateMetaInformations (azCpd);
	updateMetaInformations (altCpd);

	updateMetaInformations (azMin);
	updateMetaInformations (azMax);
	updateMetaInformations (altMin);
	updateMetaInformations (altMax);
}

void AltAz::runTracking ()
{
	parallacticTracking ();
	Telescope::runTracking ();
}

int AltAz::setTracking (int track, bool addTrackingTimer, bool send)
{
	if (track == 3)
	{
		rts2core::CommandParallacticAngle cmd (this, getInfoTime (), parallAngle->getValueDouble (), 0, telAltAz->getAlt(), telAltAz->getAz());
		queueCommandForType (DEVICE_TYPE_ROTATOR, cmd, NULL, true);
	}
	return Telescope::setTracking (track, addTrackingTimer, send);
}

void AltAz::parallacticTracking ()
{
	double n = getNow ();
	if (nextParUpdate > n)
		return;
	sendPA ();
	nextParUpdate = n + 1;
}

void AltAz::sendPA ()
{
	rts2core::CommandParallacticAngle cmd (this, getInfoTime (), parallAngle->getValueDouble (), derRate->getValueDouble (), telAltAz->getAlt(), telAltAz->getAz());
	queueCommandForType (DEVICE_TYPE_ROTATOR, cmd, NULL, true);
}

void AltAz::afterMovementStart ()
{
	Telescope::afterMovementStart ();
	nextParUpdate = 0;
	parallacticTracking ();
}

void AltAz::afterParkingStart ()
{
	rts2core::Command cmd (this, COMMAND_ROTATOR_PARK);
	queueCommandForType (DEVICE_TYPE_ROTATOR, cmd, NULL, true);
}
