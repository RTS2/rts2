/* 
 * Set of targets.
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

#include "targetset.h"
#include "sqlerror.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"

#include "rts2targetgrb.h"

#include <sstream>

using namespace rts2db;

void TargetSet::printTypeWhere (std::ostream & _os, const char *target_type)
{
	if (target_type == NULL || *target_type == '\0')
	{
		_os << "true";
		return;
	}
	const char *top = target_type;
	_os << "( ";
	while (*top)
	{
		if (top != target_type)
			_os << " or ";
		_os << "type_id = '" << *top << "'";
		top++;
	}
	_os << ")";
}

void TargetSet::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	char *stmp_c;
	int db_tar_id;
	EXEC SQL END DECLARE SECTION;

	std::list <int> targets;

	std::ostringstream _os;

	_os << "SELECT "
		"tar_id"
		" FROM "
		"targets"
		" WHERE " << where << 
		" ORDER BY " << order_by << ";";

	stmp_c = new char[_os.str ().length () + 1];
	strcpy (stmp_c, _os.str ().c_str ());

	EXEC SQL PREPARE tar_stmp FROM :stmp_c;

	delete[] stmp_c;

	EXEC SQL DECLARE tar_cur CURSOR FOR tar_stmp;

	EXEC SQL OPEN tar_cur;

	while (1)
	{
		EXEC SQL FETCH next FROM tar_cur INTO
				:db_tar_id;
		if (sqlca.sqlcode)
			break;
		targets.push_back (db_tar_id);
	}

	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		throw SqlError ();
	}
	EXEC SQL CLOSE tar_cur;
	EXEC SQL ROLLBACK;

	load (targets);
}

void TargetSet::load (std::list<int> &target_ids)
{
	for (std::list<int>::iterator iter = target_ids.begin(); iter != target_ids.end(); iter++)
	{
		Target *tar = createTarget (*iter, obs);
		if (tar != NULL)
			(*this)[*iter] = tar;
	}
}

void TargetSet::load (const char *name)
{
	std::ostringstream os;
	// replace spaces with %..
	std::string n(name);
	for (int l = 0; l < n.length (); l++)
	{
		if (n[l] == ' ')
			n[l] = '%';
	}
	os << "tar_name LIKE '" << n << "'";
	where = os.str ();
	order_by = "tar_id asc";

	load ();
}

void TargetSet::load (std::vector <const char *> &names)
{
	for (std::vector <const char *>::iterator iter = names.begin (); iter != names.end(); iter++)
	{
		TargetSet ts (obs);
		ts.load (*iter);
		if (ts.size () != 1)
			throw SqlError ((std::string ("cannot find unique target for ") + (*iter)).c_str ());
		(*this)[ts.begin ()->first] = ts.begin ()->second;
		ts.clear ();
	}
}

TargetSet::TargetSet (struct ln_lnlat_posn * in_obs): std::map <int, Target *> ()
{
	obs = in_obs;
	if (!obs)
		obs = Rts2Config::instance ()->getObserver ();
	where = std::string ("true");
	order_by = std::string ("tar_id ASC");
}

TargetSet::TargetSet (struct ln_equ_posn *pos, double radius, struct ln_lnlat_posn *in_obs)
{
	std::ostringstream where_os;
	std::ostringstream order_os;
	where_os << "ln_angular_separation (targets.tar_ra, targets.tar_dec, "
		<< pos->ra << ", "
		<< pos->dec << ") ";
	order_os << where_os.str();
	where_os << "<"
		<< radius;
	order_os << " ASC";
	obs = in_obs;
	if (!obs)
		obs = Rts2Config::instance ()->getObserver ();
	where = where_os.str ();
	order_by = order_os.str ();
}

TargetSet::TargetSet (const char *target_type, struct ln_lnlat_posn *in_obs)
{
	obs = in_obs;
	if (!obs)
		obs = Rts2Config::instance ()->getObserver ();

	std::ostringstream os;
	printTypeWhere (os, target_type);
	where = os.str ();
	order_by = std::string ("tar_id ASC");
}

TargetSet::~TargetSet (void)
{
	for (TargetSet::iterator iter = begin (); iter != end (); iter++)
	{
		delete (*iter).second;
	}
	clear ();
}

void TargetSet::addSet (TargetSet &_set)
{
	for (TargetSet::iterator iter = _set.begin (); iter != _set.end (); iter++)
	{
		if (find ((*iter).first) == end ())
			(*this)[(*iter).first] = (*iter).second;
		else
		  	throw SqlError ();
	}
	// clear set so it cannot be used again
	_set.clear ();
}

Target* TargetSet::getTarget (int _id)
{
	TargetSet::iterator iter = find (_id);
	if (iter == end ())
		throw TargetNotFound (_id);
	return (*iter).second;
}

void TargetSet::setTargetEnabled (bool enabled, bool logit)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		(*iter).second->setTargetEnabled (enabled, logit);
	}
}

void TargetSet::setTargetPriority (float new_priority)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		(*iter).second->setTargetPriority (new_priority);
	}
}

void TargetSet::setTargetBonus (float new_bonus)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		(*iter).second->setTargetBonus (new_bonus);
	}
}

void TargetSet::setTargetBonusTime (time_t *new_time)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		(*iter).second->setTargetBonusTime (new_time);
	}
}

void TargetSet::setNextObservable (time_t *time_ch)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		(*iter).second->setNextObservable (time_ch);
	}
}

void TargetSet::setTargetScript (const char *device_name, const char *script)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		(*iter).second->setScript (device_name, script);
	}
}

int TargetSet::save (bool overwrite)
{
	int ret = 0;
	int ret_s;

	for (iterator iter = begin (); iter != end (); iter++)
	{
		ret_s = (*iter).second->save (overwrite);
		if (ret_s)
			ret--;
	}
	// return number of targets for which save failed
	return ret;
}

std::ostream & TargetSet::print (std::ostream & _os, double JD)
{
	for (TargetSet::iterator tar_iter = begin(); tar_iter != end (); tar_iter++)
	{
		(*tar_iter).second->printShortInfo (_os, JD);
		_os << std::endl;
	}
	return _os;
}

std::ostream & TargetSet::printBonusList (std::ostream & _os, double JD)
{
	for (TargetSet::iterator tar_iter = begin(); tar_iter != end (); tar_iter++)
	{
		(*tar_iter).second->printShortBonusInfo (_os, JD);
		_os << std::endl;
	}
	return _os;
}

TargetSetSelectable::TargetSetSelectable (struct ln_lnlat_posn *in_obs) : TargetSet (in_obs)
{
	where = std::string ("tar_enabled = true AND tar_priority + tar_bonus > 0");
	order_by = std::string ("tar_priority + tar_bonus DESC");
}

TargetSetSelectable::TargetSetSelectable (const char *target_type, struct ln_lnlat_posn *in_obs) : TargetSet (in_obs)
{
	std::ostringstream os;
	printTypeWhere (os, target_type);
	os << " AND tar_enabled = true AND tar_priority + tar_bonus > 0";
	where = os.str ();
	order_by = std::string ("tar_id ASC");
}

TargetSetByName::TargetSetByName (const char *name):TargetSet ()
{
	size_t pos;
	where = std::string (name);
	for (pos = where.find_first_of (" "); pos != std::string::npos; pos = where.find_first_of (" "))
		where.replace (pos, 1, "%");
	where = "tar_name like '%" + where + "%'";
	order_by = std::string ("tar_name ASC");
}

TargetSetCalibration::TargetSetCalibration (Target *in_masterTarget, double JD, double airmdis):TargetSet (in_masterTarget->getObserver ())
{
	double airmass = in_masterTarget->getAirmass (JD);
	std::ostringstream os, func, ord;
	func.setf (std::ios_base::fixed, std::ios_base::floatfield);
	func.precision (12);
	func << "ABS (ln_airmass (targets.tar_ra, targets.tar_dec, "
		<< in_masterTarget->getObserver ()->lng << ", "
		<< in_masterTarget->getObserver ()->lat << ", "
		<< JD << ") - " << airmass << ")";
	if (isnan (airmdis))
		airmdis = Rts2Config::instance()->getCalibrationAirmassDistance ();
	os << func.str () << " < " << airmdis
		<< " AND ((type_id = 'c' AND tar_id <> 6) or type_id = 'l') AND tar_enabled = true";
	ord << func.str () << " ASC";
	where = os.str ();
	order_by = ord.str ();
}

TargetSetGrb::TargetSetGrb (struct ln_lnlat_posn * in_obs)
{
	obs = in_obs;
	if (!obs)
		obs = Rts2Config::instance ()->getObserver ();
}

TargetSetGrb::~TargetSetGrb (void)
{
	for (TargetSetGrb::iterator iter = begin (); iter != end (); iter++)
	{
		delete *iter;
	}
	clear ();
}

void TargetSetGrb::load ()
{
	EXEC SQL BEGIN DECLARE SECTION;
	int db_tar_id;
	EXEC SQL END DECLARE SECTION;

	std::list <int> targets;

	EXEC SQL DECLARE grb_tar_cur CURSOR FOR
		SELECT
			tar_id
		FROM
			grb
		ORDER BY
			grb_date DESC;

	EXEC SQL OPEN grb_tar_cur;

	while (1)
	{
		EXEC SQL FETCH next FROM grb_tar_cur INTO
				:db_tar_id;
		if (sqlca.sqlcode)
			break;
		targets.push_back (db_tar_id);
	}
	if (sqlca.sqlcode != ECPG_NOT_FOUND)
	{
		logStream (MESSAGE_ERROR) << "TargetSet::load cannot load targets" << sendLog;
	}
	EXEC SQL CLOSE grb_tar_cur;
	EXEC SQL ROLLBACK;

	for (std::list<int>::iterator iter = targets.begin(); iter != targets.end(); iter++)
	{
		TargetGRB *tar = (TargetGRB *) createTarget (*iter, obs);
		if (tar)
			push_back (tar);
	}
}

void TargetSetGrb::printGrbList (std::ostream & _os)
{
	for (TargetSetGrb::iterator iter = begin (); iter != end (); iter++)
	{
		(*iter)->printGrbList (_os);
	}
}

std::ostream & operator << (std::ostream &_os, TargetSet &tar_set)
{
	tar_set.print (_os, ln_get_julian_from_sys ());
	return _os;
}
