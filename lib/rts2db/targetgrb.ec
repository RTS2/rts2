/*
 * Class for GRB target.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/targetgrb.h"

#include "configuration.h"
#include "libnova_cpp.h"
#include "infoval.h"

#include "rts2db/sqlerror.h"
#include "rts2fits/image.h"

using namespace rts2db;

TargetGRB::TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs, int in_maxBonusTimeout, int in_dayBonusTimeout, int in_fiveBonusTimeout):ConstTarget (in_tar_id, in_obs)
{
	shouldUpdate = 1;
	grb_is_grb = true;
	gcnPacketType = -1;
	maxBonusTimeout = in_maxBonusTimeout;
	dayBonusTimeout = in_dayBonusTimeout;
	fiveBonusTimeout = in_fiveBonusTimeout;

	grb.ra = NAN;
	grb.dec = NAN;
	errorbox = NAN;
	autodisabled = false;
}

void TargetGRB::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	double  db_grb_date;
	double  db_grb_last_update;
	int db_grb_type;
	int db_grb_id;
	bool db_grb_is_grb;
	int db_tar_id = getTargetID ();
	double db_grb_ra;
	double db_grb_dec;
	double db_grb_errorbox;
	int db_grb_errorbox_ind;
	bool db_grb_autodisabled;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL
	SELECT
		EXTRACT (EPOCH FROM grb_date),
		EXTRACT (EPOCH FROM grb_last_update),
		grb_type,
		grb_id,
		grb_is_grb,
		grb_ra,
		grb_dec,
		grb_errorbox,
		grb_autodisabled
	INTO
		:db_grb_date,
		:db_grb_last_update,
		:db_grb_type,
		:db_grb_id,
		:db_grb_is_grb,
		:db_grb_ra,
		:db_grb_dec,
		:db_grb_errorbox :db_grb_errorbox_ind,
		:db_grb_autodisabled
	FROM
		grb
	WHERE
		tar_id = :db_tar_id;
	if (sqlca.sqlcode)
	{
	  	std::ostringstream err;
		err << "cannot load GRB data for target ID " << db_tar_id;
	  	throw SqlError (err.str ().c_str ());
	}
	grbDate = db_grb_date;
	// we don't expect grbDate to change much during observation,
	// so we will not update that in beforeMove (or somewhere else)
	lastUpdate = db_grb_last_update;
	gcnPacketType = db_grb_type;
	// switch of packet type - packet class
	if (gcnPacketType >= 40 && gcnPacketType <= 45)
	{
		// HETE burst
		gcnPacketMin = 40;
		gcnPacketMax = 45;
	}
	else if (gcnPacketType >= 50 && gcnPacketType <= 55)
	{
		// INTEGRAL burst
		gcnPacketMin = 50;
		gcnPacketMax = 55;
	}
	else if (gcnPacketType >= 60 && gcnPacketType <= 90)
	{
		// SWIFT burst
		gcnPacketMin = 60;
		gcnPacketMax = 90;
	}
	else if (gcnPacketType >= 100 && gcnPacketType < 110)
	{
		// AGILE
		gcnPacketMin = 100;
		gcnPacketMax = 109;
	}
	else if (gcnPacketType >= 110 && gcnPacketType <= 150)
	{
		// GLAST
		gcnPacketMin = 110;
		gcnPacketMax = 150;
	}
	else
	{
		// UNKNOW
		gcnPacketMin = 0;
		gcnPacketMax = 1000;
	}

	gcnGrbId = db_grb_id;
	grb_is_grb = db_grb_is_grb;
	grb.ra = db_grb_ra;
	grb.dec = db_grb_dec;
	if (db_grb_errorbox_ind)
		errorbox = NAN;
	else
		errorbox = db_grb_errorbox;
	shouldUpdate = 0;
	autodisabled = db_grb_autodisabled;

	// check if we are still valid target
	checkValidity ();

	ConstTarget::load ();
}

void TargetGRB::getPosition (struct ln_equ_posn *pos, double JD)
{
	time_t now;
	ln_get_timet_from_julian (JD, &now);
	// it's SWIFT ALERT, ra&dec are S/C ra&dec
	if (gcnPacketType == 60 && (now - lastUpdate < 3600))
	{
		struct ln_hrz_posn hrz;
		getAltAz (&hrz, JD);
		if (hrz.alt > -20 && hrz.alt < 35)
		{
			hrz.alt = 35;
			ln_get_equ_from_hrz (&hrz, observer, JD, pos);
			return;
		}
	}
	ConstTarget::getPosition (pos, JD);
}

int TargetGRB::compareWithTarget (Target *in_target, double in_sep_limit)
{
	// when it's SWIFT GRB and we are currently observing Swift burst, don't interrupt us
	// that's good only for WF instruments which observe Swift FOV
	if (in_target->getTargetID () == TARGET_SWIFT_FOV && gcnPacketType >= 60 && gcnPacketType <= 90)
	{
		return ConstTarget::compareWithTarget (in_target, 4 * in_sep_limit);
	}
	return ConstTarget::compareWithTarget (in_target, in_sep_limit);
}

double TargetGRB::getPostSec ()
{
	time_t now;
	time (&now);
	return now - (time_t) getGrbDate ();
}

void TargetGRB::checkValidity ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id = getTargetID ();
	EXEC SQL END DECLARE SECTION;
	// do not auto disable already disabled targets
	if (autodisabled == true)
		return;

	bool autodisable = false;
	if (isGrb () == false && rts2core::Configuration::instance()->grbdFollowTransients () == false)
	{
		if (getTargetEnabled () == true)
		{
 			logStream (MESSAGE_INFO) << "disabling GRB target " << getTargetName () << " (#" << getObsTargetID () << ") as it is fake GRB." << sendLog;
			autodisable = true;
		}
	}
	if (rts2core::Configuration::instance()->grbdValidity () > 0 && getPostSec () > rts2core::Configuration::instance()->grbdValidity ())
	{
		if (getTargetEnabled () == true)
		{
			logStream (MESSAGE_INFO) << "disabling GRB target " << getTargetName () << " (#" << getObsTargetID () << ") because it is too late after GRB." << sendLog;
			autodisable = true;
		}
	}

	if (autodisable)
	{
		setTargetEnabled (false);
		EXEC SQL UPDATE targets SET tar_enabled = false WHERE tar_id = :db_tar_id;
		EXEC SQL UPDATE grb SET grb_autodisabled = true WHERE tar_id = :db_tar_id;
		if (sqlca.sqlcode)
			throw SqlError ();
	}
}

void TargetGRB::getDBScript (const char *camera_name, std::string &script)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_post_sec;
	int d_pos_ind;
	VARCHAR sc_script[2000];
	VARCHAR d_camera_name[8];
	EXEC SQL END DECLARE SECTION;

	// use target script, if such exist..
	try
	{
		ConstTarget::getDBScript (camera_name, script);
		return;
	}
	catch (rts2core::Error &er)
	{
	}

	d_camera_name.len = strlen (camera_name);
	strncpy (d_camera_name.arr, camera_name, d_camera_name.len);

	// grb_script_script is NOT NULL

	EXEC SQL DECLARE find_grb_script CURSOR FOR
		SELECT
			grb_script_end,
			grb_script_script
		FROM
			grb_script
		WHERE
			camera_name = :d_camera_name
			ORDER BY
			grb_script_end ASC;
	EXEC SQL OPEN find_grb_script;
	while (1)
	{
		EXEC SQL FETCH next FROM find_grb_script INTO
				:d_post_sec :d_pos_ind,
				:sc_script;
		if (sqlca.sqlcode)
			break;
		if (getPostSec () < d_post_sec || d_pos_ind < 0)
		{
			sc_script.arr[sc_script.len] = '\0';
			script = std::string (sc_script.arr);
			break;
		}
	}
	if (sqlca.sqlcode)
	{
		EXEC SQL CLOSE find_grb_script;
		EXEC SQL ROLLBACK;
		throw rts2db::SqlError ();
	}
	sc_script.arr[sc_script.len] = '\0';
	script  = std::string (sc_script.arr);
	EXEC SQL CLOSE find_grb_script;
	EXEC SQL COMMIT;
}

bool TargetGRB::getScript (const char *deviceName, std::string &buf)
{
	time_t now;

	// try to find values first in DB..
	try
	{
		getDBScript (deviceName, buf);
		return false;
	}
	catch (rts2core::Error &er)
	{
	}

	time (&now);

	// switch based on time after burst..
	// that one has to be hard-coded, there is no way how to
	// specify it from config (there can be some, but it will be rather
	// horible way)

	if (now - (time_t) grbDate < 1000)
	{
		buf = std::string ("for 4 { E 10 } ");
	}
	else
	{
		buf = std::string ("for 4 { E 20 } ");
	}
	return false;
}

int TargetGRB::beforeMove ()
{
	// update our position..
	if (shouldUpdate)
		endObservation (-1);
	load ();
	return 0;
}

float TargetGRB::getBonus (double JD)
{
	// time from GRB
	time_t now;
	float retBonus;
	ln_get_timet_from_julian (JD, &now);
	// special SWIFT handling..
	// last packet we have is SWIFT_ALERT..
	switch (gcnPacketType)
	{
		case 60:
			// we don't get ACK | position within 1 hour..drop our priority back to some mimimum value
			if (now - (time_t) grbDate > 3600)
				return 1;
			break;
		case 62:
			if (now - lastUpdate > 1800)
				return -1;
			break;
	}
	if (now - (time_t) grbDate < maxBonusTimeout)
	{
		// < 1 hour fixed boost to be sure to not miss it
		retBonus = ConstTarget::getBonus (JD) + 2000;
	}
	else if (now - (time_t) grbDate < dayBonusTimeout)
	{
		// < 24 hour post burst
		retBonus = ConstTarget::getBonus (JD)
			+ 1000.0 * cos ((double)((double)(now - (time_t) grbDate) / (2.0*86400.0)))
			+ 10.0 * cos ((double)((double)(now - lastUpdate) / (2.0*86400.0)));
	}
	else if (now - (time_t) grbDate < fiveBonusTimeout)
	{
		// < 5 days post burst - add some time after last observation
		retBonus = ConstTarget::getBonus (JD)
			+ 10.0 * sin (getLastObsTime () / 3600.0);
	}
	else
	{
		// default
		retBonus = ConstTarget::getBonus (JD);
	}
	// if it was not GRB..
	if (grb_is_grb == false)
		retBonus /= 2;
	return retBonus;
}

int TargetGRB::isContinues ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id = getTargetID ();
	double db_tar_ra;
	double db_tar_dec;
	EXEC SQL END DECLARE SECTION;

	struct ln_equ_posn pos;

	// once we detect need to update, we need to update - stop the observation and start new
	if (shouldUpdate)
		return 0;

	EXEC SQL SELECT
			tar_ra,
			tar_dec
		INTO
			:db_tar_ra,
			:db_tar_dec
		FROM
			grb,
			targets
		WHERE
			targets.tar_id = :db_tar_id
		AND targets.tar_id = grb.tar_id;

	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetGRB::isContinues", MESSAGE_ERROR);
		return -1;
	}
	getPosition (&pos, ln_get_julian_from_sys ());
	if (pos.ra == db_tar_ra && pos.dec == db_tar_dec)
		return 1;
	// we get big update, lets synchronize again..
	shouldUpdate = 1;
	return 0;
}

double TargetGRB::getFirstPacket ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int db_grb_id = gcnGrbId;
		int db_grb_type_min = gcnPacketMin;
		int db_grb_type_max = gcnPacketMax;
		long db_grb_update;
		long db_grb_update_usec;
	EXEC SQL END DECLARE SECTION;
	EXEC SQL DECLARE cur_grb_first_packet CURSOR FOR
		SELECT
			EXTRACT (EPOCH FROM grb_update),
			grb_update_usec
		FROM
			grb_gcn
		WHERE
			grb_id = :db_grb_id
		AND grb_type BETWEEN :db_grb_type_min AND :db_grb_type_max
			ORDER BY
			grb_update asc,
			grb_update_usec asc;
	EXEC SQL OPEN cur_grb_first_packet;
	EXEC SQL FETCH next FROM cur_grb_first_packet INTO
			:db_grb_update,
			:db_grb_update_usec;
	if (sqlca.sqlcode)
	{
		EXEC SQL CLOSE cur_grb_first_packet;
		EXEC SQL ROLLBACK;
		return NAN;
	}
	EXEC SQL CLOSE cur_grb_first_packet;
	EXEC SQL ROLLBACK;
	return db_grb_update + (double) db_grb_update_usec / USEC_SEC;
}

const char * TargetGRB::getSatelite ()
{
	// get satelite
	if (gcnPacketMin == 40)
		return "HETE BURST ";
	else if (gcnPacketMin == 50)
		return "INTEGRAL BURST ";
	else if (gcnPacketMin == 60)
	{
		if (gcnPacketType >= 81)
			return "SWIFT BURST (UVOT)";
		if (gcnPacketType >= 67)
			return "SWIFT BURST (XRT)";
		return "SWIFT BURST (BAT)";
	}
	else if (gcnPacketMin == 100)
	{
		return "AGILE";
	}
	else if (gcnPacketMin == 110)
	{
		if (gcnPacketType >= 120)
			return "FERMI (LAT)";
		return "FERMI (GBM)";
	}
	return "Unknow type";
}

void TargetGRB::printExtra (Rts2InfoValStream &_os, double JD)
{
	double firstPacket = getFirstPacket ();
	double firstObs = getFirstObs ();
	ConstTarget::printExtra (_os, JD);
	_os
		<< getSatelite () << std::endl
		<< InfoVal<int> ("TYPE", gcnPacketType)
		<< (isGrb () ? "IS GRB flag is set" : "not GRB - is grb flag is not set") << std::endl
		<< InfoVal<Timestamp> ("GRB DATE", Timestamp (grbDate))
		<< InfoVal<Timestamp> ("GCN FIRST PACKET", Timestamp (firstPacket))
		<< InfoVal<TimeDiff> ("GRB->GCN", TimeDiff (grbDate, firstPacket))
		<< InfoVal<Timestamp> ("GCN LAST UPDATE", Timestamp (lastUpdate))
		<< InfoVal<LibnovaRaJ2000> ("GRB RA", LibnovaRaJ2000 (grb.ra))
		<< InfoVal<LibnovaDecJ2000> ("GRB DEC", LibnovaDecJ2000 (grb.dec))
		<< InfoVal<LibnovaDegDist> ("GRB ERR", LibnovaDegDist (errorbox))
		<< InfoVal<bool> ("AUTODISABLED", autodisabled)
		<< std::endl;
	// get information about obsering time..
	if (isnan (firstObs))
	{
		_os << "GRB without observation";
	}
	else
	{
		_os
			<< InfoVal<Timestamp> ("FIRST OBSERVATION", Timestamp (firstObs))
			<< InfoVal<TimeDiff> ("GRB delta", TimeDiff (grbDate, firstObs))
			<< InfoVal<TimeDiff> ("GCN delta", TimeDiff (firstPacket, firstObs));
	}
	_os << std::endl;
}

void TargetGRB::printDS9Reg (std::ostream & _os, double JD)
{
	struct ln_equ_posn pos;
	getPosition (&pos, JD);
	_os << "circle(" << pos.ra << "," << pos.dec << "," << errorbox << ")" << std::endl;
}

void TargetGRB::printGrbList (std::ostream & _os)
{
	struct ln_equ_posn pos;
	double firstObs = getFirstObs ();
	getPosition (&pos, ln_get_julian_from_sys ());
	_os
		<< std::setw(5) << getTargetID () << SEP
		<< LibnovaRa (pos.ra) << SEP
		<< LibnovaDec (pos.dec) << SEP
		<< Timestamp (grbDate) << SEP
		<< std::setw(15) << TimeDiff (grbDate, firstObs) << SEP
		<< std::setw(15) << TimeDiff (getFirstPacket (), firstObs) << SEP
		<< Timestamp (getLastObs ())
		<< std::endl;
}

void TargetGRB::writeToImage (rts2image::Image * image, double JD)
{
	ConstTarget::writeToImage (image, JD);
	image->setValue ("GRB_RA", grb.ra, "GRB RA know at time of exposure");
	image->setValue ("GRB_DEC", grb.dec, "GRB DEC know at time of exposure");
	image->setValue ("GRB_ERR", errorbox, "GRB errorbox diameter");
}
