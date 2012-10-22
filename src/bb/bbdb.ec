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

using namespace rts2bb;

/***
 * Register new target mapping into BB database.
 *
 * @param observatory_id  ID of observatory requesting the change
 * @param tar_id          ID of target in central database
 * @param obs_tar_id      ID of target in observatory database
 */
void createMapping (int observatory_id, int tar_id, int obs_tar_id)
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
