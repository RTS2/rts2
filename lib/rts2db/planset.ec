/*
 * Plan set class.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/planset.h"
#include "rts2db/devicedb.h"
#include "configuration.h"
#include "libnova_cpp.h"

#include <sstream>

using namespace rts2db;

void PlanSet::load ()
{
	if (checkDbConnection ())
		return;

	EXEC SQL BEGIN DECLARE SECTION;
	char *stmp_c;

	int db_plan_id;
	EXEC SQL END DECLARE SECTION;

	std::list <int> plan_ids;
	std::ostringstream os;
	int ret;

	os << "SELECT "
		"plan_id"
	" FROM "
		"plan";
	if (where.length ())
	{
		os << " WHERE " << where;
	}
	os << " ORDER BY plan_start ASC;";

	stmp_c = new char[strlen (os.str().c_str ()) + 1];
	strcpy (stmp_c, os.str().c_str ());

	EXEC SQL PREPARE plan_stmp FROM :stmp_c;

	EXEC SQL DECLARE plan_cur CURSOR FOR plan_stmp;

	EXEC SQL OPEN plan_cur;
	while (1)
	{
		EXEC SQL FETCH next
			FROM plan_cur
			INTO :db_plan_id;
		if (sqlca.sqlcode)
			break;
		plan_ids.push_back (db_plan_id);
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "Plan::load cannot load plan set" << sendLog;
		EXEC SQL CLOSE plan_cur;
		EXEC SQL ROLLBACK;
		delete[] stmp_c;
		return;
	}
	delete[] stmp_c;
	EXEC SQL CLOSE plan_cur;
	EXEC SQL ROLLBACK;

	for (std::list<int>::iterator iter = plan_ids.begin (); iter != plan_ids.end (); iter++)
	{
		Plan plan (*iter);
		ret = plan.load ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Plan cannot load plan with plan_id " << *iter << sendLog;
			continue;
		}
		push_back (plan);
	}
}


PlanSet::PlanSet ():where ("")
{
}

PlanSet::PlanSet (int prop_id)
{
	std::ostringstream os;

	os << " prop_id = " << prop_id;
	where = os.str ();
}

PlanSet::PlanSet (double t_from, double t_to)
{
	planFromTo (t_from, t_to);
}

PlanSet::~PlanSet (void)
{
}

void PlanSet::planFromTo (double t_from, double t_to)
{
	std::ostringstream os;

	os.setf (std::ios_base::fixed, std::ios_base::floatfield);

	os << " EXTRACT (EPOCH FROM plan_start) >= " << t_from;
	if (!std::isnan (t_to))
		os << " AND EXTRACT (EPOCH FROM plan_start) <= " << t_to;
	where = os.str ();
}

PlanSetTarget::PlanSetTarget (int tar_id)
{
	std::ostringstream os;

	os << " tar_id = " << tar_id;
	where = os.str ();
}

PlanSetNight::PlanSetNight (double JD)
{
	Rts2Night night (JD, rts2core::Configuration::instance ()->getObserver ());
	from = *(night.getFrom ());
	to = *(night.getTo ());
	planFromTo (from, to);
}
