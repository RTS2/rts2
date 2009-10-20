/* 
 * Auger cosmic rays showers follow-up target.
 * Copyright (C) 2005-2009 Petr Kubanek <petr@kubanek.net>
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

/*****************************************************************/
//Utility functions

/* vector cross product */
void crossProduct(struct rts2targetauger::vec * a, struct rts2targetauger::vec * b, struct rts2targetauger::vec * c)
{
	c->x = a->y * b->z - a->z * b->y;
	c->y = a->z * b->x - a->x * b->z;
	c->z = a->x * b->y - a->y * b->x;
}

/* horizontal coordinates to vector transformation */
void hrz_to_vec (struct ln_hrz_posn * dirh, struct rts2targetauger::vec * dirv)
{
	dirv->x = cos (dirh->alt) * sin (dirh->az) * (-1);
	dirv->y = cos (dirh->alt) * cos (dirh->az) * (-1);
	dirv->z = sin (dirh->alt);
}

/* vector to horizontal coordinates transformation */
void vec_to_hrz (struct rts2targetauger::vec * dirv, struct ln_hrz_posn * dirh)
{
	dirh->alt = asin (dirv->z);
	if (dirv->y > 0.)
		dirh->az = atan (dirv->x / dirv->y) + M_PI;
	if (dirv->y < 0.)
	{
		if (dirv->x < 0.)
			dirh->az = atan (dirv->x / dirv->y);
		else
			dirh->az = atan (dirv->x / dirv->y) + 2 * M_PI;
	}
	if (dirv->y == 0.)
	{
		if (dirv->x < 0.)
			dirh->az = 0.5 * M_PI;
		else
			dirh->az = 1.5 * M_PI;
	}
}

/* transformation to unit length vector */
void vec_unit (struct rts2targetauger::vec * in, struct rts2targetauger::vec * unit)
{
	double length = sqrt (in->x*in->x + in->y*in->y + in->z*in->z);
	unit->x = in->x / length;
	unit->y = in->y / length;
	unit->z = in->z / length;
}

/* rotation of vector "b" around "axis" by "angle" */
rts2targetauger::vec rotateVector(struct rts2targetauger::vec * axis, struct rts2targetauger::vec * b, double angle)
{
	struct rts2targetauger::vec ret;
	struct rts2targetauger::vec cross;
	crossProduct(axis, b, &cross);
	ret.x = b->x * cos(angle) + cross.x * sin(angle);
	ret.y = b->y * cos(angle) + cross.y * sin(angle);
	ret.z = b->z * cos(angle) + cross.z * sin(angle);
	return ret;
}

void dirToEqu (rts2targetauger::vec *dir, struct ln_lnlat_posn *observer, double JD, struct ln_equ_posn *pos)
{
	struct ln_hrz_posn hrz;
	vec_to_hrz (dir, &hrz);
	hrz.alt = ln_rad_to_deg (hrz.alt);
	hrz.az = ln_rad_to_deg (hrz.az);
	ln_get_equ_from_hrz (&hrz, observer, JD, pos);
}

/*****************************************************************/

TargetAuger::TargetAuger (int in_tar_id, struct ln_lnlat_posn * _obs, int in_augerPriorityTimeout):ConstTarget (in_tar_id, _obs)
{
	augerPriorityTimeout = in_augerPriorityTimeout;
	cor.x = nan ("f");
	cor.y = nan ("f");
	cor.z = nan ("f");
}

TargetAuger::TargetAuger (int _auger_t3id, double _auger_date, int _auger_npixels, double _auger_ra, double _auger_dec, double _northing, double _easting, double _altitude, struct ln_lnlat_posn *_obs):ConstTarget (-1, _obs)
{
	augerPriorityTimeout = -1;

	t3id = _auger_t3id;
	auger_date = _auger_date;
	npixels = _auger_npixels;
	setPosition (_auger_ra, _auger_dec);
	cor.x = _easting - 459201;
	cor.y = _northing - 6071873;
	cor.z = _altitude - 1422;
}

TargetAuger::~TargetAuger (void)
{
}

int TargetAuger::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_auger_t3id;
	double d_auger_date;
	int d_auger_npixels;
	double d_auger_ra;
	double d_auger_dec;

	double d_northing;
	double d_easting;
	double d_altitude;
	EXEC SQL END DECLARE SECTION;

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;
	double JD = ln_get_julian_from_sys ();

	EXEC SQL DECLARE cur_auger CURSOR FOR
	SELECT
		auger_t3id,
		EXTRACT (EPOCH FROM auger_date),
		NPix,
		ra,
		dec,
		Northing,
		Easting,
		Altitude
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
				:d_auger_dec,
				:d_northing,
				:d_easting,
				:d_altitude;
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
			cor.x = d_easting - 459201;
			cor.y = d_northing - 6071873;
			cor.z = d_altitude - 1422;
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

int TargetAuger::load (int auger_id)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_auger_t3id = auger_id;
	double d_auger_date;
	int d_auger_npixels;
	double d_auger_ra;
	double d_auger_dec;

	double d_northing;
	double d_easting;
	double d_altitude;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT
		auger_t3id,
		EXTRACT (EPOCH FROM auger_date),
		NPix,
		ra,
		dec,
		Northing,
		Easting,
		Altitude
	INTO
		:d_auger_t3id,
		:d_auger_date,
		:d_auger_npixels,
		:d_auger_ra,
		:d_auger_dec,
		:d_northing,
		:d_easting,
		:d_altitude
	FROM
		auger
	WHERE
		auger_t3id = :d_auger_t3id;

	if (sqlca.sqlcode)
	{
		logMsgDb ("TargetAuger::load", MESSAGE_ERROR);
		EXEC SQL CLOSE cur_auger;
		EXEC SQL ROLLBACK;
		auger_date = 0;
		return -1;
	}

	t3id = d_auger_t3id;
	auger_date = d_auger_date;
	npixels = d_auger_npixels;
	setPosition (d_auger_ra, d_auger_dec);
	cor.x = d_easting - 459201;
	cor.y = d_northing - 6071873;
	cor.z = d_altitude - 1422;
	EXEC SQL CLOSE cur_auger;
	EXEC SQL COMMIT;
	return Target::load ();
}

void TargetAuger::updateShowerFields ()
{
	time_t aug_date = auger_date;
	double JD = ln_get_julian_from_timet (&aug_date);

	struct ln_equ_posn pos;
	struct ln_hrz_posn hrz;

	getPosition (&pos, JD);
	ln_get_hrz_from_equ (&pos, observer, JD, &hrz);

	hrz.az = ln_deg_to_rad (hrz.az);
	hrz.alt = ln_deg_to_rad (hrz.alt);

	struct rts2targetauger::vec cortmp, dir, axis, unit;

	// shower direction vector
	hrz_to_vec (&hrz, &dir);

	// core position vector
	vec_unit (&cor, &cortmp);
	cor = cortmp;

	// vector perpendicular to SDP - axis of rotation
	crossProduct (&dir, &cor, &axis);
	vec_unit (&axis, &unit);
	axis = unit;

	// current and previous equatorial positions
	struct ln_equ_posn equ,prev_equ;

	dirToEqu (&dir, observer, JD, &equ);
	equ.ra -= pos.ra;
	equ.dec -= pos.dec;
	showerOffsets.push_back (equ);

	rts2targetauger::vec prev_dir = dir;
	prev_equ = equ;

	double angle = 160 * M_PI / (180 * 60);

	dir = rotateVector (&axis, &dir, angle);

	while (dir.z > 0.)
	{
		dirToEqu (&dir, observer, JD, &equ);
		equ.ra -= pos.ra;
		equ.dec -= pos.dec;

		if (dir.z <= 0.5 && prev_dir.z > 0.5 && prev_dir.z < 0.520016128)
			showerOffsets.push_back (prev_equ);
        	if (dir.z <= 0.5 && dir.z >= 0.034899497)
			showerOffsets.push_back (equ);
        	if (prev_dir.z >= 0.034899497 && dir.z > 0.011635266 && dir.z < 0.034899497)
			showerOffsets.push_back (equ);
		
		prev_dir = dir;
		prev_equ = equ;

		dir = rotateVector (&axis, &dir, angle);
	}
}

int TargetAuger::getScript (const char *device_name, std::string &buf)
{
	if (showerOffsets.size () == 0)
		updateShowerFields ();

	if (!strcmp (device_name, "WF"))
	{
		std::ostringstream _os;
		for (std::vector <struct ln_equ_posn>::iterator iter = showerOffsets.begin (); iter != showerOffsets.end (); iter++)
		{
			_os << "PARA.WOFFS=(" << (iter->ra) << "," << (iter->dec) << ") E 10 ";
		}
		buf = _os.str ();
		return 0;
	}
	buf = std::string ("E 5");
	return 0;
}

float TargetAuger::getBonus (double JD)
{
	time_t jd_date;
	ln_get_timet_from_julian (JD, &jd_date);
	// shower too old - observe for 30 minutes..
	if (jd_date > auger_date + augerPriorityTimeout)
		return 1;
	// else return value from DB
	return ConstTarget::getBonus (JD);
}

moveType TargetAuger::afterSlewProcessed ()
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

int TargetAuger::considerForObserving (double JD)
{
	load ();
	return ConstTarget::considerForObserving (JD);
}

void TargetAuger::printExtra (Rts2InfoValStream & _os, double JD)
{
	_os
		<< InfoVal<int> ("T3ID", t3id)
		<< InfoVal<Timestamp> ("DATE", Timestamp(auger_date))
		<< InfoVal<int> ("NPIXELS", npixels)
		<< InfoVal<double> ("CORE_X", cor.x)
		<< InfoVal<double> ("CORE_Y", cor.y)
		<< InfoVal<double> ("CORE_Z", cor.z)
		<< InfoVal<double> ("EASTING", cor.x + 459201)
		<< InfoVal<double> ("NORTHING", cor.y + 6071873)
		<< InfoVal<double> ("ALTITUDE", cor.z + 1422);

	ConstTarget::printExtra (_os, JD);
}

void TargetAuger::writeToImage (Rts2Image * image, double JD)
{
	ConstTarget::writeToImage (image, JD);
	image->setValue ("AGR_T3ID", t3id, "Auger target id");
}
