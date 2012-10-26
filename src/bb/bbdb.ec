/**
 * RTS2 BB Database API
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include "bbdb.h"
#include "rts2db/sqlerror.h"

#include <math.h>

using namespace rts2bb;

/***
 * Register new target mapping into BB database.
 *
 * @param observatory_id  ID of observatory requesting the change
 * @param tar_id          ID of target in central database
 * @param obs_tar_id      ID of target in observatory database
 */
void rts2bb::createMapping (int observatory_id, int tar_id, int obs_tar_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	int db_tar_id = tar_id;
	int db_obs_tar_id = obs_tar_id;
	EXEC SQL END DECLARE SECTION;
	
	EXEC SQL INSERT INTO targets_observatories
		(observatory_id, tar_id, obs_tar_id) 
	VALUES
		(:db_observatory_id, :db_tar_id, :db_obs_tar_id);

	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;
		throw rts2db::SqlError ();
	}
	EXEC SQL COMMIT;
}

void rts2bb::reportObservation (int observatory_id, int obs_id, int tar_id, double obs_ra, double obs_dec, double obs_slew, double obs_start, double obs_end, double onsky, int good_images, int bad_images)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_observatory_id = observatory_id;
	int db_obs_id = obs_id;
	int db_tar_id = tar_id;
	double db_obs_ra = obs_ra;
	double db_obs_dec = obs_dec;
	double db_obs_slew = obs_slew;
	double db_obs_start = obs_start;
	double db_obs_end = obs_end;
	double db_onsky = onsky;
	int db_good_images = good_images;
	int db_bad_images = bad_images;

	int db_obs_slew_ind = 0;
	int db_obs_start_ind = 0;
	int db_obs_end_ind = 0;
	EXEC SQL END DECLARE SECTION;

	if (isnan (obs_slew))
	{
		db_obs_slew = NAN;
		db_obs_slew_ind = -1;
	}

	if (isnan (obs_start))
	{
		db_obs_start = NAN;
		db_obs_start_ind = -1;
	}

	if (isnan (obs_end))
	{
		db_obs_end = NAN;
		db_obs_end_ind = -1;
	}

	EXEC SQL UPDATE observatory_observations SET
		tar_id = :db_tar_id,
		obs_ra = :db_obs_ra,
		obs_dec = :db_obs_dec,
		obs_slew = to_timestamp (:db_obs_slew :db_obs_slew_ind),
		obs_start = to_timestamp (:db_obs_start :db_obs_start_ind),
		obs_end = to_timestamp (:db_obs_end :db_obs_end_ind),
		onsky = :db_onsky,
		good_images = :db_good_images,
		bad_images = :db_bad_images
	WHERE
		observatory_id = :db_observatory_id AND obs_id = :db_obs_id;
	
	if (sqlca.sqlcode)
	{
		EXEC SQL ROLLBACK;

		EXEC SQL INSERT INTO observatory_observations
			(observatory_id, obs_id, tar_id, obs_ra, obs_dec, obs_slew, obs_start, obs_end, onsky, good_images, bad_images)
		VALUES
			(:db_observatory_id, :db_obs_id, :db_tar_id, :db_obs_ra, :db_obs_dec,
			to_timestamp (:db_obs_slew :db_obs_slew_ind),
			to_timestamp (:db_obs_start :db_obs_start_ind),
			to_timestamp (:db_obs_end :db_obs_end_ind),
			:db_onsky, :db_good_images, :db_bad_images);

		if (sqlca.sqlcode)
		{
			EXEC SQL ROLLBACK;
			throw rts2db::SqlError ();
		}
	}
	EXEC SQL COMMIT;
}
