/* 
 * Auger cosmic rays showers follow-up target.
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

#include "target_auger.h"
#include "../utils/timestamp.h"
#include "../utils/infoval.h"
#include "../writers/rts2image.h"

TargetAuger::TargetAuger (int in_tar_id, struct ln_lnlat_posn * in_obs, int in_augerPriorityTimeout):ConstTarget (in_tar_id, in_obs)
{
	augerPriorityTimeout = in_augerPriorityTimeout;
}


TargetAuger::~TargetAuger (void)
{

}


int
TargetAuger::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_auger_t3id;
		double d_auger_date;
		int d_auger_npixels;
		double d_auger_ra;
		double d_auger_dec;
	EXEC SQL END DECLARE SECTION;

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;
	double JD = ln_get_julian_from_sys ();

	EXEC SQL DECLARE cur_auger CURSOR FOR
		SELECT
			auger_t3id,
			EXTRACT (EPOCH FROM auger_date),
			auger_npixels,
			auger_ra,
			auger_dec
		FROM
			auger
			ORDER BY
			auger_date desc;

	EXEC SQL OPEN cur_auger;
	while (1)
	{
		EXEC SQL FETCH next FROM cur_auger INTO
				:d_auger_t3id,
				:d_auger_date,
				:d_auger_npixels,
				:d_auger_ra,
				:d_auger_dec;
		if (sqlca.sqlcode)
			break;
		pos.ra = d_auger_ra;
		pos.dec = d_auger_dec;
		ln_get_hrz_from_equ (&pos, observer, JD, &hrz);
		if (hrz.alt > 10)
		{
			t3id = d_auger_t3id;
			auger_date = d_auger_date;
			npixels = d_auger_npixels;
			setPosition (d_auger_ra, d_auger_dec);
			EXEC SQL CLOSE cur_auger;
			EXEC SQL COMMIT;
			return Target::load ();
		}
	}
	logMsgDb ("TargetAuger::load", MESSAGE_ERROR);
	EXEC SQL CLOSE cur_auger;
	EXEC SQL ROLLBACK;
	auger_date = 0;
	return -1;
}


float
TargetAuger::getBonus (double JD)
{
	time_t jd_date;
	ln_get_timet_from_julian (JD, &jd_date);
	// shower too old - observe for 30 minutes..
	if (jd_date > auger_date + augerPriorityTimeout)
		return 1;
	// else return value from DB
	return ConstTarget::getBonus (JD);
}


moveType
TargetAuger::afterSlewProcessed ()
{
	EXEC SQL BEGIN DECLARE SECTION;
		int d_obs_id;
		int d_auger_t3id = t3id;
	EXEC SQL END DECLARE SECTION;

	d_obs_id = getObsId ();
	EXEC SQL
		INSERT INTO
			auger_observation
			(
			auger_t3id,
			obs_id
			) VALUES (
			:d_auger_t3id,
			:d_obs_id
			);
	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetAuger::startSlew SQL error", MESSAGE_ERROR);
		EXEC SQL ROLLBACK;
		return OBS_MOVE_FAILED;
	}
	EXEC SQL COMMIT;
	return OBS_MOVE;
}


int
TargetAuger::considerForObserving (double JD)
{
	load ();
	return ConstTarget::considerForObserving (JD);
}


void
TargetAuger::printExtra (Rts2InfoValStream & _os, double JD)
{
	_os
		<< InfoVal<int> ("T3ID", t3id)
		<< InfoVal<Timestamp> ("DATE", Timestamp(auger_date))
		<< InfoVal<int> ("NPIXELS", npixels);
	ConstTarget::printExtra (_os, JD);
}


void
TargetAuger::writeToImage (Rts2Image * image, double JD)
{
	ConstTarget::writeToImage (image, JD);
	image->setValue ("AGR_T3ID", t3id, "Auger target id");
}
