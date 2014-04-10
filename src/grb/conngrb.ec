/* 
 * GCN socket connection.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "conngrb.h"
#include "libnova_cpp.h"

#include "connection/fork.h"
#include "rts2db/sqlerror.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <libnova/libnova.h>

EXEC SQL include sqlca;

using namespace rts2grbd;

double ConnGrb::getPktSod ()
{
	return lbuf[PKT_SOD]/100.0;
}

void ConnGrb::getTimeTfromTJD (long TJD, double SOD, time_t * in_time, long * usec)
{
	double JD = getJDfromTJD (TJD, SOD);
	ln_get_timet_from_julian (JD, in_time);
	if (usec)
		*usec = (int)((SOD - int (SOD)) * USEC_SEC);
}

int ConnGrb::pr_test ()
{
	logStream (MESSAGE_INFO) << "Test GCN notice Trig#: " << (lbuf[BURST_TRIG])
		<< " TJD: " << (lbuf[BURST_TJD])
		<< " RA: " << (lbuf[BURST_RA] / 10000.0)
		<< " DEC: " << (lbuf[BURST_DEC] / 10000.0)
		<< " Error: " << (lbuf[BURST_ERROR])
		<< sendLog;
	return 0;
}

int ConnGrb::pr_imalive ()
{
	deltaValue = here_sod - getPktSod ();
#ifdef DEBUG
	logStream (MESSAGE_DEBUG) << "ConnGrb::pr_imalive last packet SN=" << getPktSod () << " delta=" << deltaValue << " last_delta=" << (getPktSod () - last_imalive_sod) << sendLog;
#endif
	last_imalive_sod = getPktSod ();
	return 0;
}

int ConnGrb::pr_swift_point ()
{
	double roll;
	char *obs_name;
	float obstime;
	float merit;
	swiftLastRa  = lbuf[7]/10000.0;
	swiftLastDec = lbuf[8]/10000.0;
	roll = lbuf[9]/10000.0;
	getTimeTfromTJD (lbuf[5], lbuf[6]/100.0, &swiftLastPoint);
	obs_name = (char*) (&lbuf[BURST_URL]);
	obstime = lbuf[14]/100.0;
	merit = lbuf[15]/100.0;
	return addSwiftPoint (roll, obs_name, obstime, merit);
}

int ConnGrb::pr_integral_point ()
{
	struct ln_equ_posn pos_int, pos_j2000;
	time_t t;
	pos_int.ra = lbuf[14]/10000.0;
	pos_int.dec = lbuf[15]/10000.0;
	// precess to J2000
	ln_get_equ_prec2 (&pos_int, ln_get_julian_from_sys (), JD2000, &pos_j2000);
	getTimeTfromTJD (lbuf[5], lbuf[6]/100.0, &t);
	return addIntegralPoint (pos_j2000.ra, pos_j2000.dec, &t);
}

int ConnGrb::pr_agile_point ()
{
}

int ConnGrb::pr_fermi_point ()
{
}

int ConnGrb::pr_hete ()
{
	int grb_id;
	int grb_seqn;
	int grb_type;
	struct ln_equ_posn pos_int, pos_j2000;
	int grb_is_grb = 1;
	time_t grb_date;
	long grb_date_usec;
	float grb_errorbox;

	grb_id = ((lbuf[BURST_TRIG] & H_TRIGNUM_MASK) >> H_TRIGNUM_SHIFT);
	grb_seqn = ((lbuf[BURST_TRIG] & H_SEQNUM_MASK) >> H_SEQNUM_SHIFT);
	grb_type = lbuf[PKT_TYPE];

	pos_int.ra = lbuf[BURST_RA] / 10000.0;
	pos_int.dec = lbuf[BURST_DEC] / 10000.0;

	getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date, &grb_date_usec);

	grb_errorbox = (lbuf[H_WXM_DIM_NSIG] >> 16) / 3600.0;

	if (!do_hete_test->getValueBool ()
		&& (grb_type == TYPE_HETE_TEST
		|| (lbuf[H_TRIG_FLAGS] & H_ART_TRIG)
		)
		)
	{
		logStream (MESSAGE_DEBUG) << "ConnGrb::pr_hete test packet" << sendLog;
		return 0;
	}

	// convert to J2000 only when it's true GRB
	// HETE non GRB notices (GRB retraction notices) have ra and dec -999.99, and we
	// need to pass that value futher to addGcnPoint, so it will not update RA & DEC in DB.
	if (pos_int.ra > -300 && pos_int.dec > -300)
	{
		ln_get_equ_prec2 (&pos_int, ln_get_julian_from_timet (&grb_date), JD2000, &pos_j2000);
	}
	else
	{
		pos_j2000.ra = pos_int.ra;
		pos_j2000.dec = pos_int.dec;
	}

	if ((lbuf[H_TRIG_FLAGS] & H_DEF_NOT_GRB)
		|| (lbuf[H_TRIG_FLAGS] & H_DEF_SGR)
		|| (lbuf[H_TRIG_FLAGS] & H_DEF_XRB))
		grb_is_grb = 0;

	return addGcnPoint (grb_id, grb_seqn, grb_type, pos_j2000.ra, pos_j2000.dec, grb_is_grb, &grb_date, grb_date_usec, grb_errorbox, false, true);
}

int ConnGrb::pr_integral ()
{
	int grb_id;
	int grb_seqn;
	int grb_type;
	struct ln_equ_posn pos_int, pos_j2000;
	int grb_is_grb = 1;
	time_t grb_date;
	long grb_date_usec;
	float grb_errorbox;

	if (!do_hete_test->getValueBool ()
		&& ((lbuf[12] & (1 << 31))
		)
		)
	{
		logStream (MESSAGE_DEBUG) << "ConnGrb::pr_integral test packet (" << lbuf[12] << ")" << sendLog;
		return 0;
	}

	grb_id = (lbuf[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
	grb_seqn = (lbuf[BURST_TRIG] & I_SEQNUM_MASK) >> I_SEQNUM_SHIFT;
	grb_type = lbuf[PKT_TYPE];

	pos_int.ra = lbuf[BURST_RA]/10000.0;
	pos_int.dec = lbuf[BURST_DEC]/10000.0;

	getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date, &grb_date_usec);

	ln_get_equ_prec2 (&pos_int, ln_get_julian_from_timet (&grb_date), JD2000, &pos_j2000);

	grb_errorbox = (float) lbuf[BURST_ERROR] / 3600.0;

	if (grb_errorbox < 0 && grb_type == TYPE_INTEGRAL_OFFLINE_SRC)
	{
		grb_is_grb = 0;
		grb_errorbox *= -1;
	}

	return addGcnPoint (grb_id, grb_seqn, grb_type, pos_j2000.ra, pos_j2000.dec, grb_is_grb, &grb_date, grb_date_usec, grb_errorbox, false, true);
}

int ConnGrb::pr_integral_spicas ()
{
	logStream (MESSAGE_INFO) << "INTEGRAL SPIACS" << sendLog;
	return 0;
}

int ConnGrb::pr_swift_with_radec ()
{
	int grb_id;
	int grb_seqn;
	int grb_type;
	double grb_ra;
	double grb_dec;
	int grb_is_grb = 1;
	time_t grb_date;
	long grb_date_usec;
	float grb_errorbox;

	grb_type = (int) (lbuf[PKT_TYPE]);
	grb_id = (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
	grb_seqn = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
	grb_ra = lbuf[BURST_RA] / 10000.0;
	grb_dec = lbuf[BURST_DEC] / 10000.0;

	// we will set grb_is_grb to true in some special cases..
	switch (grb_type)
	{
		case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
		case TYPE_SWIFT_BAT_GRB_LC_SRC:
		case TYPE_SWIFT_SCALEDMAP_SRC:
			if ((lbuf[TRIGGER_ID] & 0x00000002)
				&& ((lbuf[TRIGGER_ID] & 0x00000020) == 0)
				&& !(lbuf[TRIGGER_ID] & 0x00000100))
			{
				grb_is_grb = 1;
			}
			else
			{
				grb_is_grb = 0;
			}
			break;
		case TYPE_SWIFT_XRT_POSITION_SRC:
			// if it's not a grb, or if its in ground cat, ignore it..
			if ((lbuf[TRIGGER_ID] & 0x00000020)
				|| (lbuf[TRIGGER_ID] & 0x00000100))
			{
				grb_is_grb = 0;
			}
			break;
	}

	getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date, &grb_date_usec);
	switch (grb_type)
	{
		case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
		case TYPE_SWIFT_XRT_POSITION_SRC:
		case TYPE_SWIFT_UVOT_POS_SRC:
			grb_errorbox = (float) lbuf[BURST_ERROR] / 10000.0;
			break;
		default:
			grb_errorbox = getInstrumentErrorBox (grb_type);
	}

	// ignore wrong packets.. (2^10 or 2^11 set)
	if (lbuf[MISC] & 0xC00)
	{
		logStream (MESSAGE_INFO) << "setting error box to 360 degrees, as GCN packet, as it's MISC field suggest it is not correct: " << std::hex << lbuf[MISC] << " at RA " << std::dec << grb_ra << " DEC " << grb_dec << sendLog;
		grb_errorbox = 360;
	}

	return addGcnPoint (grb_id, grb_seqn, grb_type, grb_ra, grb_dec, grb_is_grb, &grb_date, grb_date_usec, grb_errorbox, false, true);
}

int ConnGrb::pr_swift_without_radec ()
{
	// those messages have only sence, when they set grb_is_grb flag to false
	EXEC SQL BEGIN DECLARE SECTION;
		int d_grb_id;
		int d_grb_seqn;
		int d_grb_type;
		int d_grb_type_start;
		int d_grb_type_end;
	EXEC SQL END DECLARE SECTION;

	time_t grb_date;
	long grb_date_usec;

	double grb_ra;
	double grb_dec;

	int ret;

	d_grb_type = (int)(lbuf[PKT_TYPE]);
	d_grb_id = (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
	d_grb_seqn = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;

	getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date, &grb_date_usec);

	switch (d_grb_type)
	{
		case TYPE_SWIFT_BAT_GRB_ALERT_SRC:
			// get S/C coordinates to slew on
			// that's special in big errror-box
			// but as we specify last know ra/dec, we will slew to best location we know about burst
			// assume that swift will never spend more then three hours on one location, due to orbit parameters
			// as burst can happen during slew, we have to put in fabs - otherwise we will not respond to burst
			// catched during/before slew, but after pointdir notice was send
			if (fabs (grb_date - swiftLastPoint) < 3 * 3600)
				addGcnPoint (d_grb_id, d_grb_seqn, d_grb_type, swiftLastRa, swiftLastDec, 1, &grb_date, grb_date_usec, getInstrumentErrorBox (d_grb_type), false, true);
			break;
		case TYPE_SWIFT_BAT_GRB_POS_NACK_SRC:
			// update if not grb..
			getGrbBound (d_grb_type, d_grb_type_start, d_grb_type_end);
			EXEC SQL
				UPDATE
					grb
				SET
					grb_type = :d_grb_type,
					grb_seqn = :d_grb_seqn,
					grb_is_grb = false
				WHERE
					grb_id = :d_grb_id
				AND grb_type >= :d_grb_type_start
				AND grb_type <= :d_grb_type_end;
			if (sqlca.sqlcode)
			{
				throw rts2db::SqlError ("cannot update Swift GRB with POS_NACK_SRC");
			}
			else
			{
				logStream (MESSAGE_INFO) << "grb_is_grb = false grb_id " << d_grb_id << sendLog;
				EXEC SQL COMMIT;
			}
			break;
		case TYPE_SWIFT_UVOT_IMAGE_SRC:
		case TYPE_SWIFT_UVOT_IMAGE_PROC_SRC:
			if (lbuf[MISC] & (0x01L << 29))
			{
				logStream (MESSAGE_INFO) << "ignoring SWIFT UVOT SRC with UVOT_SrcList Noticed forced-out via the watchdog timeout" << sendLog;
				return -1;
			}
		case TYPE_SWIFT_SCALEDMAP_SRC:
		case TYPE_SWIFT_XRT_CENTROID_SRC:
		case TYPE_SWIFT_UVOT_SLIST_SRC:
		case TYPE_SWIFT_UVOT_SLIST_PROC_SRC:
			grb_ra = lbuf[BURST_RA] / 10000.0;
			grb_dec = lbuf[BURST_DEC] / 10000.0;
			ret = addGcnPoint (d_grb_id, d_grb_seqn, d_grb_type, grb_ra, grb_dec, 1, &grb_date, grb_date_usec, getInstrumentErrorBox(d_grb_type), true, true);
			// when it was sucessfullt added
			// we don't have to add raw, as it was added in GcnPoint
			if (!ret)
				return ret;
			break;
	}

	return addGcnRaw (d_grb_id, d_grb_seqn, d_grb_type);
}

int ConnGrb::pr_agile ()
{
	int grb_id;
	int grb_type;
	double grb_ra;
	double grb_dec;

	int grb_is_grb = 1;
	time_t grb_date;
	long grb_date_usec;
	float grb_errorbox;

	grb_type = (int) (lbuf[PKT_TYPE]);
	grb_id = lbuf[BURST_TRIG];
	grb_ra = lbuf[BURST_RA] / 10000.0;
	grb_dec = lbuf[BURST_DEC] / 10000.0;

	if (!do_hete_test->getValueBool ()
		&& (grb_type == TYPE_AGILE_GRB_POS_TEST))
	{
		logStream (MESSAGE_DEBUG) << "ConnGrb::pr_agile test packet" << sendLog;
		return 0;
	}

	getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date, &grb_date_usec);

	grb_errorbox = (float) lbuf[BURST_ERROR] / 10000.0;

	grb_is_grb = lbuf[TRIGGER_ID] & 0x0022;

	return addGcnPoint (grb_id, 1, grb_type, grb_ra, grb_dec, grb_is_grb, &grb_date, grb_date_usec, grb_errorbox, false, true);
}

int ConnGrb::pr_fermi_gbm ()
{
	time_t grb_date;
	long grb_date_usec;

	getTimeTfromTJD (lbuf[BURST_TJD], lbuf[BURST_SOD]/100.0, &grb_date, &grb_date_usec);

	double _error = lbuf[BURST_ERROR] / 10000.0;

	bool enabled = false;
	if (gbm_error < 0 || _error <= gbm_error || gbm_enable_above)
		enabled = true;

	if (gbm_record_above || gbm_error < 0 || _error <= gbm_error)
		return addGcnPoint (lbuf[BURST_TRIG], lbuf[PKT_SERNUM], (int) lbuf[PKT_TYPE], lbuf[BURST_RA] / 10000.0, lbuf[BURST_DEC] / 10000.0, true, &grb_date, grb_date_usec, _error, false, enabled);
	logStream (MESSAGE_INFO) << "ignoring GBM above error limit - " << gbm_error << " > " << _error << sendLog;
	return 0;
}

int ConnGrb::pr_fermi_lat ()
{
	logStream (MESSAGE_INFO) << "LAT messagye, type " << lbuf[PKT_TYPE] << sendLog;
	return 0;
}

int ConnGrb::pr_fermi_sc ()
{

}

int ConnGrb::addSwiftPoint (double roll, char * obs_name, float obstime, float merit)
{
	EXEC SQL BEGIN DECLARE SECTION;
		double d_swift_ra = swiftLastRa;
		double d_swift_dec = swiftLastDec;
		double d_swift_roll = roll;
		double d_swift_time = (long) swiftLastPoint;
		double d_swift_received = (long) last_packet.tv_sec + (double) last_packet.tv_usec / USEC_SEC;
		float d_swift_obstime = obstime;
		varchar d_swift_name[70];
		float d_swift_merit = merit;
	EXEC SQL END DECLARE SECTION;

	strcpy (d_swift_name.arr, obs_name);
	d_swift_name.len = strlen (obs_name);

	EXEC SQL
		INSERT INTO swift
		(
			swift_id,
			swift_ra,
			swift_dec,
			swift_roll,
			swift_time,
			swift_received,
			swift_name,
			swift_obstime,
			swift_merit
		)
		VALUES (
			nextval ('point_id'),
			:d_swift_ra,
			:d_swift_dec,
			:d_swift_roll,
			to_timestamp (:d_swift_time),
			to_timestamp (:d_swift_received),
			:d_swift_name,
			:d_swift_obstime,
			:d_swift_merit
		);
	if (sqlca.sqlcode != 0)
	{
		throw rts2db::SqlError ("cannot insert swift trigger");
	}
	EXEC SQL COMMIT;
	master->updateSwift (swiftLastPoint, swiftLastRa, swiftLastDec);
	return 0;
}

int ConnGrb::addIntegralPoint (double ra, double dec, const time_t *t)
{
	EXEC SQL BEGIN DECLARE SECTION;
		double d_integral_ra = ra;
		double d_integral_dec = dec;
		double d_integral_time = (int) *t;
		double d_integral_received = (long) last_packet.tv_sec + (double) last_packet.tv_usec / USEC_SEC;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
		INSERT INTO
			integral
			(
			integral_id,
			integral_ra,
			integral_dec,
			integral_time,
			integral_received
			) VALUES (
		nextval ('point_id'),
			:d_integral_ra,
			:d_integral_dec,
			to_timestamp (:d_integral_time),
			to_timestamp (:d_integral_received)
			);
	if (sqlca.sqlcode != 0)
	{
		throw rts2db::SqlError ("cannot insert INTEGRAL pointing");
	}
	EXEC SQL COMMIT;
	master->updateIntegral (d_integral_time, ra, dec);
	return 0;
}

void ConnGrb::getGrbBound (int grb_type, int &grb_start, int &grb_end)
{
	if (grb_type <= TYPE_FERMI_POINTDIR)
	{
		if (grb_type >= TYPE_FERMI_GBM_ALERT)
		{
			grb_start = TYPE_FERMI_GBM_ALERT;
			grb_end = TYPE_FERMI_POINTDIR;
		}
		else if (grb_type >= TYPE_AGILE_GRB_WAKEUP)
		{
			grb_start = TYPE_AGILE_GRB_WAKEUP;
			grb_end = TYPE_AGILE_GRB_POS_TEST;
		}
		else if (grb_type >= TYPE_SWIFT_BAT_GRB_ALERT_SRC)
		{
			grb_start = TYPE_SWIFT_BAT_GRB_ALERT_SRC;
			grb_end = TYPE_SWIFT_BAT_SLEW_POS_SRC;
		}
		else if (grb_type >= TYPE_MILAGRO_POS_SRC)
		{
			grb_start = TYPE_MILAGRO_POS_SRC;
			grb_end = TYPE_KONUS_LC_SRC;
		}
		else if (grb_type >= TYPE_INTEGRAL_POINTDIR_SRC)
		{
			grb_start = TYPE_INTEGRAL_POINTDIR_SRC;
			grb_end = TYPE_INTEGRAL_OFFLINE_SRC;
		}
		else if (grb_type >= TYPE_HETE_ALERT_SRC)
		{
			grb_start = TYPE_HETE_ALERT_SRC;
			grb_end = TYPE_GRB_CNTRPART_SRC;
		}
		else if (grb_type == TYPE_IPN_POS_SRC)
		{
			grb_start = TYPE_IPN_POS_SRC;
			grb_end = TYPE_IPN_POS_SRC;
		}
		else
		{
			// all fields counts - unknow type
			logStream (MESSAGE_ERROR) << "ConnGrb::getGrbBound cannot get type for grb_type " << grb_type << sendLog;
			grb_start = 0;
			grb_end = 5000;
		}
	}
	else
	{
		// all fields counts - unknow type
		logStream (MESSAGE_ERROR) << "ConnGrb::getGrbBound cannot get type for grb_type " << grb_type << sendLog;
		grb_start = 0;
		grb_end = 5000;
	}
}

bool ConnGrb::gcnContainsGrbPos (int grb_type)
{
	switch (grb_type)
	{
		case TYPE_INTEGRAL_POINTDIR_SRC:
		case TYPE_INTEGRAL_SPIACS_SRC:
		case TYPE_SWIFT_SCALEDMAP_SRC:
		case TYPE_SWIFT_XRT_CENTROID_SRC:
		case TYPE_SWIFT_UVOT_IMAGE_SRC:
		case TYPE_SWIFT_UVOT_SLIST_SRC:
		case TYPE_SWIFT_UVOT_SLIST_PROC_SRC:
		case TYPE_SWIFT_POINTDIR_SRC:
		case TYPE_SWIFT_UVOT_NACK_POSITION:
		case TYPE_AGILE_POINTDIR:
		case TYPE_FERMI_POINTDIR:
			return false;
		default:
			return true;
	}
}

float ConnGrb::getInstrumentErrorBox (int grb_type)
{
	// rules:
	//  if it's only detection, return FOV of instrument
	//  if it's position, return some conservative instrument everage position error
	switch (grb_type)
	{
		// INTEGRAL FOV
		case TYPE_INTEGRAL_POINTDIR_SRC:
			return 30.0;
			// Swift FOV
		case TYPE_SWIFT_POINTDIR_SRC:
			return 60.0;
			// HETE instrumental error..
		case TYPE_HETE_ALERT_SRC:
		case TYPE_HETE_UPDATE_SRC:
		case TYPE_HETE_FINAL_SRC:
		case TYPE_HETE_GNDANA_SRC:
		case TYPE_HETE_TEST:
		case TYPE_GRB_CNTRPART_SRC:
			// .. is 4 arcmin
			return 4.0 / 60.0;
		case TYPE_INTEGRAL_WAKEUP_SRC:
		case TYPE_INTEGRAL_REFINED_SRC:
		case TYPE_INTEGRAL_OFFLINE_SRC:
			//INTEGRAL instrument error is 4 armin
			return 4.0 / 60.0;
		case TYPE_INTEGRAL_SPIACS_SRC:
			// SPIACS is in fact all sky detector
			return 180.0;
		case TYPE_SWIFT_BAT_GRB_ALERT_SRC:
			// BAT have 60 deg
			return 60.0;
		case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
		case TYPE_SWIFT_BAT_GRB_LC_SRC:
		case TYPE_SWIFT_FOM_2OBSAT_SRC:
		case TYPE_SWIFT_FOSC_2OBSAT_SRC:
		case TYPE_SWIFT_BAT_GRB_LC_PROC_SRC:
		case TYPE_SWIFT_BAT_TRANS:
		case TYPE_SWIFT_BAT_GRB_POS_NACK_SRC:
		case TYPE_SWIFT_SCALEDMAP_SRC:
			// BAT have 4 arcmin
			return 4.0 / 60.0;
		case TYPE_SWIFT_XRT_POSITION_SRC:
		case TYPE_SWIFT_XRT_SPECTRUM_SRC:
		case TYPE_SWIFT_XRT_IMAGE_SRC:
		case TYPE_SWIFT_XRT_LC_SRC:
		case TYPE_SWIFT_XRT_SPECTRUM_PROC_SRC:
		case TYPE_SWIFT_XRT_IMAGE_PROC_SRC:
			// conservative estimate for XRT is 20 arcsec, including uncertanities
			return 20.0 / 3600.0;
		case TYPE_SWIFT_XRT_CENTROID_SRC:
			// XRT FOV
			return 15.0 / 60.0;
		case TYPE_SWIFT_UVOT_SLIST_PROC_SRC:
		case TYPE_SWIFT_UVOT_POS_SRC:
			// that's VERY conservative estimate, we might refine it
			return 19.0 / 3600.0;
		case TYPE_SWIFT_UVOT_SLIST_SRC:
		case TYPE_SWIFT_UVOT_IMAGE_SRC:
		case TYPE_SWIFT_UVOT_IMAGE_PROC_SRC:
			// UVOT FOV
			return 15.0 / 60.0;
		case TYPE_AGILE_GRB_WAKEUP:
		case TYPE_AGILE_GRB_PROMPT:
		case TYPE_AGILE_GRB_REFINED:
		case TYPE_AGILE_TRANS:
			return 3;
		case TYPE_AGILE_POINTDIR:
			return 60;
		case TYPE_FERMI_GBM_ALERT:
		case TYPE_FERMI_GBM_FLT_POS:
		case TYPE_FERMI_GBM_GND_POS:
		case TYPE_FERMI_GBM_LC:
		case TYPE_FERMI_GBM_TRANS:
		case TYPE_FERMI_GBM_POS_TEST:
		case TYPE_FERMI_LAT_POS_INI:
		case TYPE_FERMI_LAT_POS_UPD:
		case TYPE_FERMI_LAT_POS_DIAG:
		case TYPE_FERMI_LAT_TRANS:
		case TYPE_FERMI_OBS_REQ:
		case TYPE_FERMI_SC_SLEW:
			// TODO fill that with FERMI S/C specifications, when it will be
			// on launch pad
			return 3.0;
	}
	logStream (MESSAGE_WARNING) << "ConnGrb::getInstrumentErrorBox unknow type: " << grb_type
		<< ", returning 180.0" << sendLog;
	return 180.0;
}

int ConnGrb::addGcnPoint (int grb_id, int grb_seqn, int grb_type, double grb_ra, double grb_dec, bool grb_is_grb, time_t *grb_date, long grb_date_usec, float grb_errorbox, bool insertOnly, bool enabled)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_tar_id;
	int d_grb_id = grb_id;
	int d_grb_seqn = grb_seqn;
	int d_grb_type = grb_type;
	int d_curr_grb_type = -1;
	double d_grb_ra = grb_ra;
	double d_grb_dec = grb_dec;
	bool d_grb_is_grb = grb_is_grb;
	bool db_was_grb;
	double d_grb_date = *grb_date + (double) grb_date_usec / USEC_SEC;
	double d_grb_update = last_packet.tv_sec + (double) last_packet.tv_usec / USEC_SEC;
	float d_grb_errorbox = grb_errorbox;
	int d_grb_errorbox_ind;
	// used to find correct grb - based on type
	int d_grb_type_start;
	int d_grb_type_end;
	// target stuff
	VARCHAR d_tar_name[150];
	VARCHAR d_tar_comment[2000];
	bool d_tar_enabled = enabled;
	EXEC SQL END DECLARE SECTION;

	int grb_isnew = 0;

	if ((master->getRecordNotVisible () == false) && 
		((master->observer->lat > 0 && grb_dec < (master->observer->lat - 90 ))
		 || (master->observer->lat < 0 && grb_dec > (master->observer->lat + 90 ))
	        )
	)	
	{
		logStream (MESSAGE_INFO) << "not recording GRB at " << LibnovaRaDec (grb_ra, grb_dec) << ", as it is never visible from current location (latitude " << master->observer->lat << ")" << sendLog;
		return 0;
	}

	if (master->getRecordOnlyVisibleTonight () == true)
	{
		// check if target is visible tonight at all..
		struct ln_rst_time rst;
		double JD = ln_get_julian_from_sys ();
		Rts2Night night (JD, master->observer);

		struct ln_equ_posn pos;
		pos.ra = grb_ra;
		pos.dec = grb_dec;

		ln_get_object_next_rst_horizon (night.getJDFrom (), master->observer, &pos, master->getMinGrbAltitute (), &rst);

		if (!((night.getJDFrom () < rst.set && night.getJDTo () > rst.set) || 
			(night.getJDFrom () < rst.rise && night.getJDTo () > rst.rise) || 
			(night.getJDFrom () < rst.transit && night.getJDTo () > rst.transit)))
		{
			logStream (MESSAGE_INFO) << "not recording GRB at " << LibnovaRaDec (grb_ra, grb_dec) << " as it is not durring current night above minimal altitude " << LibnovaDeg90 (master->getMinGrbAltitute ()) << sendLog;
			return 0;
		}
	}

	struct tm grb_broken_time;

	int ret = 0;

	gmtime_r (grb_date, &grb_broken_time);
	d_tar_name.len = snprintf (d_tar_name.arr, 150, "GRB %02d%02d%06.3f GCN #%i",
		grb_broken_time.tm_year % 100, grb_broken_time.tm_mon + 1, grb_broken_time.tm_mday +
		(grb_broken_time.tm_hour * 3600 + grb_broken_time.tm_min * 60 + grb_broken_time.tm_sec) / 86400.0,
		d_grb_id);

	getGrbBound (grb_type, d_grb_type_start, d_grb_type_end);

	EXEC SQL
	SELECT
		tar_id,
		grb_type,
		grb_errorbox,
		grb_is_grb
	INTO
		:d_tar_id,
		:d_curr_grb_type,
		:d_grb_errorbox :d_grb_errorbox_ind,
		:db_was_grb
	FROM
		grb
	WHERE
		grb_id = :d_grb_id
		AND grb_type >= :d_grb_type_start
		AND grb_type <= :d_grb_type_end;

	if (sqlca.sqlcode == ECPG_NOT_FOUND)
	{
		// create new GCN entry..
		d_grb_errorbox = grb_errorbox;
		if (isnan (d_grb_errorbox))
		{
			d_grb_errorbox_ind = -1;
			d_grb_errorbox = 0;
		}
		else
		{
			d_grb_errorbox_ind = 0;
		}
		// do not insert if it's know source and follow transient is false
		if (grb_is_grb == false && rts2core::Configuration::instance()->grbdFollowTransients () == false)
		{
			logStream (MESSAGE_INFO) << "Ignoring know source target creation" << sendLog;
			return 0;
		}
		// insert part..we do care about HETE burst without coordinates
		if (d_grb_ra < -300 && d_grb_dec < -300)
		{
			
			logStream (MESSAGE_DEBUG) << "ConnGrb::addGcnPoint HETE GRB without coords? ra="
				<< d_grb_ra << " dec=" << d_grb_dec << sendLog;
			EXEC SQL ROLLBACK;
			return -1;
		}
		// generate new GRB details
		d_tar_comment.len = sprintf (d_tar_comment.arr, "Generated by GRBD for event %d-%02d-%02dT%02d:%02d:%02d, GCN #%i, type %i",
			grb_broken_time.tm_year + 1900, grb_broken_time.tm_mon + 1, grb_broken_time.tm_mday,
			grb_broken_time.tm_hour, grb_broken_time.tm_min, grb_broken_time.tm_sec, d_grb_id, d_grb_type);

		EXEC SQL
		SELECT
			nextval ('grb_tar_id')
		INTO
			:d_tar_id;

		if (sqlca.sqlcode)
		{
		  	throw rts2db::SqlError ("cannot retrieve next value from grb_tar_id sequence");
		}

		// check and honest create_disabled value
		if (master->getCreateDisabled ())
			d_tar_enabled = false;

		// insert new target
		EXEC SQL
		INSERT INTO
			targets
			(
				tar_id,
				type_id,
				tar_name,
				tar_ra,
				tar_dec,
				tar_enabled,
				tar_comment,
				tar_priority,
				tar_bonus,
				tar_bonus_time
			)
			VALUES
			(
				:d_tar_id,
				'G',
				:d_tar_name,
				:d_grb_ra,
				:d_grb_dec,
				:d_tar_enabled,
				:d_tar_comment,
				100,
				100,
				NULL
			);
		if (sqlca.sqlcode)
		{
			throw rts2db::SqlError ("cannot add GCN coordinates");
		}
		grb_isnew = 1;
		// insert new grb packet
		EXEC SQL
		INSERT INTO
			grb
			(
				tar_id,
				grb_id,
				grb_seqn,
				grb_type,
				grb_ra,
				grb_dec,
				grb_is_grb,
				grb_date,
				grb_last_update,
				grb_errorbox,
				grb_autodisabled
			)
			VALUES
			(
				:d_tar_id,
				:d_grb_id,
				:d_grb_seqn,
				:d_grb_type,
				:d_grb_ra,
				:d_grb_dec,
				:d_grb_is_grb,
				to_timestamp (:d_grb_date),
				to_timestamp (:d_grb_update),
				:d_grb_errorbox :d_grb_errorbox_ind,
				false
			);
		if (sqlca.sqlcode)
		{
			throw rts2db::SqlError ("cannot insert new GCN coordinates");
		}
		else
		{
			logStream (MESSAGE_INFO) << "ConnGrb::addGcnPoint grb created: tar_id: "
				<< d_tar_id
				<< " grb_id: " << d_grb_id
				<< " grb_seqn: " << d_grb_seqn
				<< " ra dec: " << LibnovaRaDec (d_grb_ra, d_grb_dec) 
				<< " grb errorbox: " << d_grb_errorbox
				<< sendLog;
			EXEC SQL COMMIT;
		}
	}
	else if (sqlca.sqlcode)
	{
		throw rts2db::SqlError ("cannot check for GRB coordinates in the database");
	}
	else
	{
		// update know event
		if (insertOnly)
		{
			EXEC SQL ROLLBACK;
			return 1;
		}
		// HETE burst have values -999 in some retraction notices..
		// do updates only when new position is better than old one
		if (
			(d_grb_errorbox_ind < 0
			|| isnan (grb_errorbox)
			|| grb_errorbox <= d_grb_errorbox
			)
			&& d_grb_ra > -300
			&& d_grb_dec > -300)
		{

			// update target informations..
			if (master->getCreateDisabled ())
			{
				// don't update tar_enabled if targets are created disabled
				EXEC SQL
					UPDATE
						targets
					SET
						tar_ra = :d_grb_ra,
						tar_dec = :d_grb_dec
					WHERE
						tar_id = :d_tar_id;
			}
			else
			{
				EXEC SQL
					UPDATE
						targets
					SET
						tar_ra = :d_grb_ra,
						tar_dec = :d_grb_dec,
						tar_enabled = :d_tar_enabled
					WHERE
						tar_id = :d_tar_id;
			}
			if (sqlca.sqlcode)
			{
			  	throw rts2db::SqlError ("cannot update GRB target coordinates");
			}
			if (master->getCreateDisabled ())
				logStream (MESSAGE_INFO) << "Update target #" << d_tar_id << " RA DEC: " << LibnovaRaDec (d_grb_ra, d_grb_dec) << ", target enabled/disabled state not updated" << sendLog;
			else
				logStream (MESSAGE_INFO) << "Update target #" << d_tar_id << " RA DEC: " << LibnovaRaDec (d_grb_ra, d_grb_dec) << ", set state to " << (d_tar_enabled ? "enabled" : "disabled") << sendLog;

			// update grb informations..
			// do updates only when new position is better then old one
			if (gcnContainsGrbPos (d_grb_type)
				&& !isnan(grb_errorbox)
				&& (d_grb_errorbox_ind < 0
				|| grb_errorbox <= d_grb_errorbox)
				)
			{
				EXEC SQL
					UPDATE
						grb
					SET
						grb_seqn = :d_grb_seqn,
						grb_type = :d_grb_type,
						grb_ra = :d_grb_ra,
						grb_dec = :d_grb_dec,
						grb_is_grb = :d_grb_is_grb,
						grb_last_update = to_timestamp (:d_grb_update)
					WHERE
						tar_id = :d_tar_id;

				if (sqlca.sqlcode)
				{
					throw rts2db::SqlError ("cannot update GRB GCN entry");
				}
				else
				{
					EXEC SQL COMMIT;
				}

				d_grb_errorbox = grb_errorbox;
				EXEC SQL
					UPDATE
						grb
					SET
						grb_errorbox = :d_grb_errorbox
					WHERE
						tar_id = :d_tar_id;
				if (sqlca.sqlcode)
				{
					throw rts2db::SqlError ("cannot update GCN error box");
				}
				else
				{
					logStream (MESSAGE_INFO) << "ConnGrb::addGcnPoint grb updated: tar_id: "
						<< d_tar_id << " grb_id: " << d_grb_id << " grb_errorbox: " << d_grb_errorbox << " grb_seqn: " << d_grb_seqn << sendLog;
					EXEC SQL COMMIT;
				}
			}
		}
		else
		{
			logStream (MESSAGE_INFO) << "ConnGrb::addGcnPoint grb update ignored: grb_errorbox "
				<< grb_errorbox
				<< " d_grb_errorbox " << d_grb_errorbox
				<< " d_grb_errorbox_ind " << d_grb_errorbox_ind << sendLog;
			// update grb_is_grb, if that has changed
			if (d_grb_is_grb != db_was_grb)
			{
				EXEC SQL UPDATE
					grb
				SET
					grb_is_grb = :d_grb_is_grb
				WHERE
					tar_id = :d_tar_id;
				if (sqlca.sqlcode)
				{
					throw rts2db::SqlError ("cannot update grb_is_grb");
				}
				else
				{
					EXEC SQL COMMIT;
				}
			}
			// do not update
			d_grb_errorbox_ind = -1;
		}
	}

	addGcnRaw (grb_id, grb_seqn, grb_type);

	// do not follow if it's know transient and FollowTransients is false
	if (grb_is_grb == false && rts2core::Configuration::instance ()->grbdFollowTransients () == false)
	{
		logStream (MESSAGE_INFO) << "Disabling know source." << sendLog;
		EXEC SQL
		UPDATE
			targets
		SET
			tar_enabled = false
		WHERE
			tar_id = :d_tar_id;
		if (sqlca.sqlcode)
		{
			throw rts2db::SqlError ("cannot update tar_enabled");
		}
		EXEC SQL COMMIT;
		return 0;
	}

	if (d_grb_type_start == TYPE_FERMI_GBM_ALERT && gbm_error > 0 && d_grb_errorbox > gbm_error)
	{
		logStream (MESSAGE_INFO) << "only recorded GBM GRB with id " << d_grb_id << ", as it is above gbm_error_limit" << sendLog;
		return ret;
	}

	// test if that's only follow-up
	if (!execFollowups)
	{
		// swift burst
		if (d_grb_type_start == TYPE_SWIFT_BAT_GRB_ALERT_SRC
			&& grb_id < 100000
			&& d_grb_errorbox_ind == -1)
		{
			// and it's only follow-up slew notice without errorbox..don't do anything
			return ret;
		}
	}

	ret = master->newGcnGrb (d_tar_id);

	// last thing is to call some external exe..
	if (addExe)
	{
		int execRet;
		rts2core::ConnFork *execConn = new rts2core::ConnFork (master, addExe, false, false, 100);

		execConn->addArg (d_tar_id);
		execConn->addArg (grb_id);
		execConn->addArg (grb_seqn);
		execConn->addArg (grb_type);
		execConn->addArg (grb_ra);
		execConn->addArg (grb_dec);
		execConn->addArg (grb_is_grb);
		execConn->addArg (*grb_date);
		execConn->addArg (grb_errorbox);
		execConn->addArg (grb_isnew);

		execRet = execConn->init ();
		if (execRet < 0)
		{
			delete execConn;
		}
		else if (execRet == 0)
		{
			master->addConnection (execConn);
		}
	}

	delete[] last_target;
	last_target = new char[d_tar_name.len + 1];
	strcpy (last_target, d_tar_name.arr);
	last_target_time = d_grb_update;
	last_target_type = d_grb_type;

	last_ra = d_grb_ra;
	last_dec = d_grb_dec;

	last_target_errorbox = grb_errorbox;

	return ret;
}

int ConnGrb::addGcnRaw (int grb_id, int grb_seqn, int grb_type)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_grb_id = grb_id;
		int d_grb_seqn = grb_seqn;
		int d_grb_type = grb_type;
		long int d_grb_update = (int) last_packet.tv_sec;
		int d_grb_update_usec = (int) last_packet.tv_usec;

		long d_packet0;
		long d_packet1;
		long d_packet2;
		long d_packet3;
		long d_packet4;
		long d_packet5;
		long d_packet6;
		long d_packet7;
		long d_packet8;
		long d_packet9;
		long d_packet10;
		long d_packet11;
		long d_packet12;
		long d_packet13;
		long d_packet14;
		long d_packet15;
		long d_packet16;
		long d_packet17;
		long d_packet18;
		long d_packet19;
		long d_packet20;
		long d_packet21;
		long d_packet22;
		long d_packet23;
		long d_packet24;
		long d_packet25;
		long d_packet26;
		long d_packet27;
		long d_packet28;
		long d_packet29;
		long d_packet30;
		long d_packet31;
		long d_packet32;
		long d_packet33;
		long d_packet34;
		long d_packet35;
		long d_packet36;
		long d_packet37;
		long d_packet38;
		long d_packet39;
	EXEC SQL END DECLARE SECTION;


	d_packet0 = lbuf[0];
	d_packet1 = lbuf[1];
	d_packet2 = lbuf[2];
	d_packet3 = lbuf[3];
	d_packet4 = lbuf[4];
	d_packet5 = lbuf[5];
	d_packet6 = lbuf[6];
	d_packet7 = lbuf[7];
	d_packet8 = lbuf[8];
	d_packet9 = lbuf[9];

	d_packet10 = lbuf[10];
	d_packet11 = lbuf[11];
	d_packet12 = lbuf[12];
	d_packet13 = lbuf[13];
	d_packet14 = lbuf[14];
	d_packet15 = lbuf[15];
	d_packet16 = lbuf[16];
	d_packet17 = lbuf[17];
	d_packet18 = lbuf[18];
	d_packet19 = lbuf[19];

	d_packet20 = lbuf[20];
	d_packet21 = lbuf[21];
	d_packet22 = lbuf[22];
	d_packet23 = lbuf[23];
	d_packet24 = lbuf[24];
	d_packet25 = lbuf[25];
	d_packet26 = lbuf[26];
	d_packet27 = lbuf[27];
	d_packet28 = lbuf[28];
	d_packet29 = lbuf[29];

	d_packet30 = lbuf[30];
	d_packet31 = lbuf[31];
	d_packet32 = lbuf[32];
	d_packet33 = lbuf[33];
	d_packet34 = lbuf[34];
	d_packet35 = lbuf[35];
	d_packet36 = lbuf[36];
	d_packet37 = lbuf[37];
	d_packet38 = lbuf[38];
	d_packet39 = lbuf[39];

	EXEC SQL
		INSERT INTO
			grb_gcn
			(
			grb_id,
			grb_seqn,
			grb_type,
			grb_update,
			grb_update_usec,
			packet
			) VALUES (
			:d_grb_id,
			:d_grb_seqn,
			:d_grb_type,
			to_timestamp (:d_grb_update),
			:d_grb_update_usec,
			ARRAY[:d_packet0, :d_packet1, :d_packet2, :d_packet3, :d_packet4, :d_packet5, :d_packet6, :d_packet7, :d_packet8, :d_packet9,
				:d_packet10, :d_packet11, :d_packet12, :d_packet13, :d_packet14, :d_packet15, :d_packet16, :d_packet17, :d_packet18, :d_packet19,
				:d_packet20, :d_packet21, :d_packet22, :d_packet23, :d_packet24, :d_packet25, :d_packet26, :d_packet27, :d_packet28, :d_packet29,
				:d_packet30, :d_packet31, :d_packet32, :d_packet33, :d_packet34, :d_packet35, :d_packet36, :d_packet37, :d_packet38, :d_packet39
			]::bigint[]

			);
	if (sqlca.sqlcode)
	{
		throw rts2db::SqlError ("cannot insert raw GCN packet");
	}
	else
	{
		EXEC SQL COMMIT;
		return 0;
	}
}

ConnGrb::ConnGrb (char *in_gcn_hostname, int in_gcn_port, rts2core::ValueBool *in_do_hete_test, char *in_addExe, int in_execFollowups, Grbd *in_master):rts2core::ConnNoSend (in_master)
{
	master = in_master;
	gcn_hostname = new char[strlen (in_gcn_hostname) + 1];
	strcpy (gcn_hostname, in_gcn_hostname);
	gcn_port = in_gcn_port;
	gcn_listen_sock = -1;

	last_packet.tv_sec = 0;
	last_packet.tv_usec = 0;
	last_imalive_sod = -1;

	deltaValue = 0;
	last_target = NULL;
	last_target_type = -1;
	last_target_time = NAN;

	last_ra = NAN;
	last_dec = NAN;

	last_target_errorbox = NAN;

	do_hete_test = in_do_hete_test;

	setConnTimeout (90);
	time (&nextTime);
	nextTime += getConnTimeout ();

	swiftLastPoint = 0;
	swiftLastRa = NAN;
	swiftLastDec = NAN;

	addExe = in_addExe;
	execFollowups = in_execFollowups;

	gcnReceivedBytes = 0;

	gbm_error = 0.25;
	gbm_record_above = true;
	gbm_enable_above = false;
}

ConnGrb::~ConnGrb (void)
{
	delete[] gcn_hostname;
	delete[] last_target;
	if (gcn_listen_sock >= 0)
		close (gcn_listen_sock);
}

int ConnGrb::idle ()
{
	int ret;
	int err;
	socklen_t len = sizeof (err);

	time_t now;
	time (&now);

	switch (getConnState ())
	{
		case CONN_CONNECTING:
			ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
			if (ret)
			{
				logStream (MESSAGE_ERROR) << "ConnGrb::idle getsockopt " << strerror (errno) << sendLog;
				connectionError (-1);
			}
			else if (err)
			{
				logStream (MESSAGE_ERROR) << "ConnGrb::idle getsockopt " << strerror (err) << sendLog;
				connectionError (-1);
			}
			else
			{
				setConnState (CONN_CONNECTED);
			}
			break;
			// kill us when we were in conn_connecting state for to long
		case CONN_BROKEN:
			if (nextTime < now)
			{
				ret = init ();
				if (ret)
				{
					time (&nextTime);
					nextTime += getConnTimeout ();
				}
			}
			break;
		case CONN_CONNECTED:
			if (last_packet.tv_sec + getConnTimeout () < now && nextTime < now)
				connectionError (-1);
			break;
		default:
			break;
	}
	// we don't like to get called upper code with timeouting stuff..
	return 0;
}

int ConnGrb::init_call ()
{
	struct addrinfo hints;
	struct addrinfo *info;
	int ret;

	hints.ai_flags = 0;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	std::ostringstream _os;
	_os << gcn_port;
	ret = getaddrinfo (gcn_hostname, _os.str ().c_str (), &hints, &info);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "NetworkAddress::getAddress getaddrinfor: " << gai_strerror (ret) << sendLog;
		freeaddrinfo (info);
		return -1;
	}
	sock = socket (info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sock == -1)
	{
		freeaddrinfo (info);
		return -1;
	}
	ret = connect (sock, info->ai_addr, info->ai_addrlen);
	freeaddrinfo (info);
	time (&nextTime);
	nextTime += getConnTimeout ();
	if (ret == -1)
	{
		if (errno == EINPROGRESS)
		{
			setConnState (CONN_CONNECTING);
			return 0;
		}
		return -1;
	}
	setConnState (CONN_CONNECTED);
	return 0;
}

int ConnGrb::init_listen ()
{
	int ret;

	if (gcn_listen_sock >= 0)
	{
		close (gcn_listen_sock);
		gcn_listen_sock = -1;
	}

	connectionError (-1);

	gcn_listen_sock = socket (PF_INET, SOCK_STREAM, 0);
	if (gcn_listen_sock == -1)
	{
		logStream (MESSAGE_ERROR) << "ConnGrb::init_listen socket " << strerror (errno) << sendLog;
		return -1;
	}
	const int so_reuseaddr = 1;
	setsockopt (gcn_listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (so_reuseaddr));
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons (gcn_port);
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	ret = bind (gcn_listen_sock, (struct sockaddr *) &server, sizeof (server));
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnGrb::init_listen bind: " << strerror (errno) << sendLog;
		return -1;
	}
	ret = listen (gcn_listen_sock, 1);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "ConnGrb::init_listen listen: " << strerror (errno) << sendLog;
		return -1;
	}
	setConnState (CONN_CONNECTED);
	time (&nextTime);
	nextTime += 2 * getConnTimeout ();
	return 0;
}

int ConnGrb::init ()
{
	if (!strcmp (gcn_hostname, "-"))
		return init_listen ();
	else
		return init_call ();
}

int ConnGrb::add (fd_set * readset, fd_set * writeset, fd_set * expset)
{
	if (gcn_listen_sock >= 0)
	{
		FD_SET (gcn_listen_sock, readset);
		return 0;
	}
	return rts2core::Connection::add (readset, writeset, expset);
}

void ConnGrb::connectionError (int last_data_size)
{
	logStream (MESSAGE_ERROR) << "lost GCN connection - SN=" << getPktSod () << " delta=" << deltaValue << " last_delta=" << (getPktSod () - last_imalive_sod) << sendLog;
	if (sock > 0)
	{
		close (sock);
		sock = -1;
	}
	if (!isConnState (CONN_BROKEN))
	{
		time (&nextTime);
		sock = -1;
		setConnState (CONN_BROKEN);
	}
	gcnReceivedBytes = 0;
}

int ConnGrb::receive (fd_set *set)
{
	int ret = 0;
	struct tm *t;
	if (gcn_listen_sock >= 0 && FD_ISSET (gcn_listen_sock, set))
	{
		// try to accept connection..
		close (sock);			 // close previous connections..we support only one GCN connection
		sock = -1;
		struct sockaddr_in other_side;
		socklen_t addr_size = sizeof (struct sockaddr_in);
		sock = accept (gcn_listen_sock, (struct sockaddr *) &other_side, &addr_size);
		if (sock == -1)
		{
			// bad accept - strange
			logStream (MESSAGE_ERROR) << "ConnGrb::receive accept on gcn_listen_sock: " << strerror (errno) << sendLog;
			connectionError (-1);
		}
		// close listening socket..when we get connection
		close (gcn_listen_sock);
		gcn_listen_sock = -1;
		setConnState (CONN_CONNECTED);
		logStream (MESSAGE_INFO) << "ConnGrb::receive accept gcn_listen_sock from "
			<< inet_ntoa (other_side.sin_addr) << " port " << ntohs (other_side.sin_port) << sendLog;
	}
	else if (sock >= 0 && FD_ISSET (sock, set))
	{
		try
		{
			ret = read (sock, ((char*) nbuf) + gcnReceivedBytes, sizeof (nbuf) - gcnReceivedBytes);
			if (ret == 0 && isConnState (CONN_CONNECTING))
			{
				setConnState (CONN_CONNECTED);
			}
			else if (ret < 0)
			{
				connectionError (ret);
				return -1;
			}
			gcnReceivedBytes += ret;
			// we don't receive full packet..
			if (gcnReceivedBytes < (int) (SIZ_PKT * sizeof(nbuf[0])))
				return ret;
			gcnReceivedBytes = 0;
			successfullRead ();
			gettimeofday (&last_packet, NULL);
			// swap bytes..
			for (int i=0; i < SIZ_PKT; i++)
			{
				lbuf[i] = ntohl (nbuf[i]);
			}
	
			/* Immediately echo back the packet so GCN can monitor:
			 * (1) the actual receipt by the site, and
			 * (2) the roundtrip travel times.
			 * Everything except KILL's get echo-ed back.            */
			if(lbuf[PKT_TYPE] != TYPE_KILL_SOCKET)
			{
				write (sock, (char *)nbuf, sizeof(nbuf));
				successfullSend ();
			}
			t = gmtime (&last_packet.tv_sec);
			here_sod = t->tm_hour*3600 + t->tm_min*60 + t->tm_sec + last_packet.tv_usec / USEC_SEC;
	
			// switch based on packet content
			switch (lbuf[PKT_TYPE])
			{
				case TYPE_TEST_COORDS:
					pr_test ();
					break;
				case TYPE_IM_ALIVE:
					pr_imalive ();
					break;
					// pondtirs messages with history..
				case TYPE_INTEGRAL_POINTDIR_SRC:
					pr_integral_point ();
					break;
									 // 83  // Swift Pointing Direction
				case TYPE_SWIFT_POINTDIR_SRC:
					pr_swift_point ();
					break;
				case TYPE_AGILE_POINTDIR:
					pr_agile_point ();
					break;
				case TYPE_FERMI_POINTDIR:
					pr_fermi_point ();
					break;
				// hete, integral & swift GRB observations
				case TYPE_HETE_ALERT_SRC:
				case TYPE_HETE_UPDATE_SRC:
				case TYPE_HETE_FINAL_SRC:
				case TYPE_HETE_GNDANA_SRC:
				case TYPE_HETE_TEST:
				case TYPE_GRB_CNTRPART_SRC:
					pr_hete ();
					break;
				case TYPE_INTEGRAL_WAKEUP_SRC:
				case TYPE_INTEGRAL_REFINED_SRC:
				case TYPE_INTEGRAL_OFFLINE_SRC:
					pr_integral ();
					break;
					// integral spiacs
				case TYPE_INTEGRAL_SPIACS_SRC:
					pr_integral_spicas ();
					break;
				case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:
				case TYPE_SWIFT_BAT_GRB_LC_SRC:
				case TYPE_SWIFT_FOM_2OBSAT_SRC:
				case TYPE_SWIFT_FOSC_2OBSAT_SRC:
				case TYPE_SWIFT_XRT_POSITION_SRC:
				case TYPE_SWIFT_XRT_SPECTRUM_SRC:
				case TYPE_SWIFT_XRT_IMAGE_SRC:
				case TYPE_SWIFT_XRT_LC_SRC:
				case TYPE_SWIFT_UVOT_SLIST_SRC:
					// processed messages
				case TYPE_SWIFT_BAT_GRB_LC_PROC_SRC:
				case TYPE_SWIFT_XRT_SPECTRUM_PROC_SRC:
				case TYPE_SWIFT_XRT_IMAGE_PROC_SRC:
				case TYPE_SWIFT_UVOT_SLIST_PROC_SRC:
				case TYPE_SWIFT_UVOT_POS_SRC:
					// transient
				case TYPE_SWIFT_BAT_TRANS:
					pr_swift_with_radec ();
					break;
				case TYPE_SWIFT_BAT_GRB_ALERT_SRC:
				case TYPE_SWIFT_BAT_GRB_POS_NACK_SRC:
				case TYPE_SWIFT_SCALEDMAP_SRC:
				case TYPE_SWIFT_XRT_CENTROID_SRC:
				case TYPE_SWIFT_UVOT_IMAGE_SRC:
					// processed messages
				case TYPE_SWIFT_UVOT_IMAGE_PROC_SRC:
				case TYPE_SWIFT_UVOT_NACK_POSITION:
					pr_swift_without_radec ();
					break;
				case TYPE_AGILE_GRB_WAKEUP:
				case TYPE_AGILE_GRB_PROMPT:
				case TYPE_AGILE_GRB_REFINED:
				case TYPE_AGILE_TRANS:
				case TYPE_AGILE_GRB_POS_TEST:
					pr_agile ();
					break;
				case TYPE_FERMI_GBM_ALERT:
					break;
				case TYPE_FERMI_GBM_FLT_POS:
				case TYPE_FERMI_GBM_GND_POS:
				case TYPE_FERMI_GBM_LC:
				case TYPE_FERMI_GBM_TRANS:
					pr_fermi_gbm ();
					break;
				case TYPE_FERMI_LAT_POS_INI:
				case TYPE_FERMI_LAT_POS_UPD:
				case TYPE_FERMI_LAT_POS_DIAG:
				case TYPE_FERMI_LAT_TRANS:
					pr_fermi_lat ();
					break;
				case TYPE_FERMI_OBS_REQ:
				case TYPE_FERMI_SC_SLEW:
					pr_fermi_sc ();
					break;
				case TYPE_KILL_SOCKET:
					connectionError (-1);
					break;
				default:
					logStream (MESSAGE_ERROR) << "ConnGrb::receive unknow packet type: " << lbuf[PKT_TYPE] << sendLog;
					break;
			}
			// enable others to catch-up (FW connections will forward packet to their sockets)
			getMaster ()->postEvent (new rts2core::Event (RTS2_EVENT_GRB_PACKET, nbuf));
		}
		catch (rts2core::Error er)
		{
			logStream (MESSAGE_ERROR) << er << sendLog;
		}
	}
	return ret;
}

double ConnGrb::lastPacket ()
{
	if (last_packet.tv_sec == 0)
		return NAN;
	return last_packet.tv_sec;
}

double ConnGrb::delta ()
{
	return deltaValue;
}

char* ConnGrb::lastTarget ()
{
	return last_target;
}
