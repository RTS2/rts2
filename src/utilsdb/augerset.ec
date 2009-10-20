/* 
 * Sets of Auger targets.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "augerset.h"
#include "sqlerror.h"

#include "../utils/rts2config.h"

using namespace rts2db;

void AugerSet::load (double _from, double _to)
{
	EXEC SQL BEGIN DECLARE SECTION;
	double d_from = _from;
	double d_to = _to;

	int d_auger_t3id;
	double d_auger_date;
	int d_auger_npixels;
	double d_auger_ra;
	double d_auger_dec;

	double d_northing;
	double d_easting;
	double d_altitude;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL DECLARE cur_augerset CURSOR FOR
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
	WHERE
		auger_date BETWEEN to_timestamp (:d_from) and to_timestamp (:d_to)
	ORDER BY
		auger_date asc;
	
	EXEC SQL OPEN cur_augerset;

	while (1)
	{
		EXEC SQL FETCH next FROM cur_augerset INTO
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
		(*this)[d_auger_t3id] = TargetAuger (d_auger_t3id, d_auger_date, d_auger_npixels, d_auger_ra, d_auger_dec, d_northing, d_easting, d_altitude, Rts2Config::instance ()->getObserver ());
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE cur_augerset;
		EXEC SQL ROLLBACK;
		throw SqlError ();
	}

	EXEC SQL CLOSE cur_augerset;
	EXEC SQL ROLLBACK;
}

void AugerSet::printHTMLTable (std::ostringstream &_os)
{
	double JD = ln_get_julian_from_sys ();\

	for (AugerSet::iterator iter = begin (); iter != end (); iter++)
	{
		iter->second.printHTMLRow (_os, JD);
	}
}

void AugerSetDate::load (int year, int month, int day, int hour, int minutes)
{
	EXEC SQL BEGIN DECLARE SECTION;
	int d_value;
	int d_c;
	char *stmp_c;
	EXEC SQL END DECLARE SECTION;

	const char *group_by;
	std::ostringstream _where;

	std::ostringstream _os;
	if (year == -1)
	{
		group_by = "year";
	}
	else
	{
		_where << "EXTRACT (year FROM auger_date) = " << year;
		if (month == -1)
		{
			group_by = "month";
		}
		else
		{
			_where << " AND EXTRACT (month FROM auger_date) = " << month;
			if (day == -1)
			{
				group_by = "day";
			}
			else
			{
				_where << " AND EXTRACT (day FROM auger_date) = " << day;
				if (hour == -1)
				{
				 	group_by = "hour";
				}
				else
				{
					_where << " AND EXTRACT (hour FROM auger_date) = " << hour;
					if (minutes == -1)
					{
					 	group_by = "minute";
					}
					else
					{
						_where << " AND EXTRACT (minutes FROM auger_date) = " << minutes;
						group_by = "second";
					}
				}
			}
		}
	}

	_os << "SELECT EXTRACT (" << group_by << " FROM auger_date) as value, count (*) as c FROM auger ";
	
	if (_where.str ().length () > 0)
		_os << "WHERE " << _where.str ();

	_os << " GROUP BY value;";

	stmp_c = new char[_os.str ().length () + 1];
	memcpy (stmp_c, _os.str().c_str(), _os.str ().length () + 1);
	EXEC SQL PREPARE augerset_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE augersetdate_cur CURSOR FOR augerset_stmp;

	EXEC SQL OPEN augersetdate_cur;

	while (1)
	{
		EXEC SQL FETCH next FROM augersetdate_cur INTO
			:d_value,
			:d_c;
		if (sqlca.sqlcode)
			break;
		(*this)[d_value] = d_c;
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		EXEC SQL CLOSE augersetdate_cur;
		EXEC SQL ROLLBACK;

		throw SqlError ();
	}

	EXEC SQL CLOSE augersetdate_cur;
	EXEC SQL ROLLBACK;
}
