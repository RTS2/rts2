/* 
 * Set of targets.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

#include "rts2db/targetset.h"
#include "rts2db/sqlerror.h"

#include "configuration.h"
#include "libnova_cpp.h"

#include "rts2db/targetgrb.h"

#include <sstream>

using namespace rts2db;

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
	  	load (*iter);
}

void TargetSet::load (int id)
{
	Target *tar = createTarget (id, obs, obs_altitude);
	(*this)[id] = tar;
}

void TargetSet::loadByName (const char *name, bool approxName, bool ignoreCase)
{
	std::ostringstream os;
	// replace spaces with %..
	std::string n(name);
	if (approxName)
	{
		for (size_t l = 0; l < n.length (); l++)
		{
			if (n[l] == ' ')
				n[l] = '%';
		}
		if (ignoreCase)
		{
			os << "lower(tar_name) LIKE lower('" << n << "')";
		}
		else
		{
			os << "tar_name LIKE '" << n << "'";
		}
	}
	else
	{
		if (ignoreCase)
		{
			os << "lower(tar_name) = lower('" << n << "')";
		}
		else
		{
			os << "tar_name = '" << n << "'";
		}
	}
	where = os.str ();
	order_by = "tar_id asc";

	load ();
}

void TargetSet::loadByLabel (const char *label)
{
	int d_label_id;
	
	Labels lb;
	for (int i = LABEL_PI; i <= LABEL_PROGRAM; i++)
	{
		try
		{
			d_label_id = lb.getLabel (label, i);
			loadByLabelId (d_label_id);
		}
		catch (rts2core::Error &er)
		{
			// ignore error  
		}
	}
}

void TargetSet::loadByLabelId (int label_id)
{
	std::ostringstream os;
	
	os << "EXISTS (SELECT * FROM target_labels where target_labels.tar_id = targets.tar_id and label_id = " << label_id << ")";

	where = os.str ();
	order_by = "tar_id asc";

	load ();
}

void TargetSet::load (const char * name, TargetSet::iterator const (*multiple_resolver) (TargetSet *ts), bool approxName, resolverType resType)
{
	if (resType == NAME_ID || resType == ID_ONLY)
	{
		char *endp;
		int tid = strtol (name, &endp, 10);
		if (*endp == '\0')
		{
			// numeric target
			try
			{
				Target *tar = createTarget (tid, obs, obs_altitude);
				(*this)[tid] = tar;
				return;
			}
			catch (SqlError err)
			{
				if (resType == ID_ONLY)
					throw UnresolvedTarget (name);
			}
		}
		else
		{
			// ID_ONLY with non-numeric ID
			if (resType == ID_ONLY)
				throw UnresolvedTarget (name);
		}
	}
	TargetSet ts (obs);
	ts.loadByName (name, approxName);
	if (ts.size () == 0)
	{
		throw UnresolvedTarget (name);
	}
	if (ts.size () > 1)
	{
		if (multiple_resolver == NULL)
				throw SqlError ((std::string ("cannot find unique target for ") + (name)).c_str ());
		TargetSet::iterator res = multiple_resolver (&ts);
		if (res != ts.end ())
		{
			(*this)[res->first] = res->second;
			ts.erase (res);
		}
		else
		{
			insert (ts.begin (), ts.end ());
			ts.clear ();
		}
	}
	else if (ts.size () == 1)
	{
		(*this)[ts.begin ()->first] = ts.begin ()->second;
		ts.clear ();
	}
}

void TargetSet::load (std::vector <const char *> &names, TargetSet::iterator const (*multiple_resolver) (TargetSet *ts), bool approxName, resolverType resType)
{
	for (std::vector <const char *>::iterator iter = names.begin (); iter != names.end(); iter++)
	{
		load (*iter, multiple_resolver, approxName, resType);
	}
}

TargetSet::TargetSet (struct ln_lnlat_posn * in_obs, double in_altitude): std::map <int, Target *> ()
{
	obs = in_obs;
	if (!obs)
		obs = rts2core::Configuration::instance ()->getObserver ();
	obs_altitude = in_altitude;
	if (isnan (obs_altitude))
		obs_altitude = rts2core::Configuration::instance ()->getObservatoryAltitude ();
	where = std::string ("true");
	order_by = std::string ("tar_id ASC");
}

TargetSet::TargetSet (struct ln_equ_posn *pos, double radius, struct ln_lnlat_posn *in_obs, double in_altitude)
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
		obs = rts2core::Configuration::instance ()->getObserver ();
	obs_altitude = in_altitude;
	if (isnan (obs_altitude))
		obs_altitude = rts2core::Configuration::instance ()->getObservatoryAltitude ();
	where = where_os.str ();
	order_by = order_os.str ();
}

TargetSet::TargetSet (const char *target_type, struct ln_lnlat_posn *in_obs, double in_altitude)
{
	obs = in_obs;
	if (!obs)
		obs = rts2core::Configuration::instance ()->getObserver ();
	obs_altitude = in_altitude;
	if (isnan (obs_altitude))
		obs_altitude = rts2core::Configuration::instance ()->getObservatoryAltitude ();

	std::ostringstream os;
	printTypeWhere (os, target_type);
	where = os.str ();
	order_by = std::string ("tar_id ASC");
}

TargetSet::~TargetSet (void)
{
	for (TargetSet::iterator iter = begin (); iter != end (); iter++)
	{
		delete iter->second;
	}
	clear ();
}

void TargetSet::addSet (TargetSet &_set)
{
	for (TargetSet::iterator iter = _set.begin (); iter != _set.end (); iter++)
	{
		if (find (iter->first) == end ())
			(*this)[iter->first] = iter->second;
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
	return iter->second;
}

void TargetSet::setTargetEnabled (bool enabled, bool logit)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setTargetEnabled (enabled, logit);
	}
}

void TargetSet::setTargetPriority (float new_priority)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setTargetPriority (new_priority);
	}
}

void TargetSet::setTargetBonus (float new_bonus)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setTargetBonus (new_bonus);
	}
}

void TargetSet::setTargetBonusTime (time_t *new_time)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setTargetBonusTime (new_time);
	}
}

void TargetSet::setNextObservable (time_t *time_ch)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setNextObservable (time_ch);
	}
}

void TargetSet::setTargetScript (const char *device_name, const char *script)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setScript (device_name, script);
	}
}

void TargetSet::setTargetProperMotion (struct ln_equ_posn *pm)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		switch (iter->second->getTargetType ())
		{
			case TYPE_CALIBRATION:
			case TYPE_OPORTUNITY:
				((ConstTarget *) (iter->second))->setProperMotion (pm);
				break;
			default:
				logStream (MESSAGE_ERROR) << "cannot set proper motion for non-constant target " << iter->second->getTargetID () << sendLog;
				break;
		}
	}
}

void TargetSet::setTargetPIName (const char *pi)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setPIName (pi);
	}
}

void TargetSet::setTargetProgramName (const char *program)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setProgramName (program);
	}
}

void TargetSet::deleteTargets ()
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->deleteTarget ();
	}
}

void TargetSet::setConstraints (Constraints &cons)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->setConstraints (cons);
	}
}

void TargetSet::appendConstraints (Constraints &cons)
{
	for (iterator iter = begin (); iter != end (); iter++)
	{
		iter->second->appendConstraints (cons);
	}
}

int TargetSet::save (bool overwrite, bool clean)
{
	int ret = 0;
	int ret_s;

	for (iterator iter = begin (); iter != end (); iter++)
	{
		ret_s = iter->second->save (overwrite);
		if (ret_s)
			ret--;
		if (clean)
		{
			delete iter->second;
			iter->second = NULL;
		}
	}
	// return number of targets for which save failed
	return ret;
}

std::ostream & TargetSet::print (std::ostream & _os, double JD)
{
	for (TargetSet::iterator tar_iter = begin(); tar_iter != end (); tar_iter++)
	{
		tar_iter->second->printShortInfo (_os, JD);
		_os << std::endl;
	}
	return _os;
}

std::ostream & TargetSet::printBonusList (std::ostream & _os, double JD)
{
	for (TargetSet::iterator tar_iter = begin(); tar_iter != end (); tar_iter++)
	{
		tar_iter->second->printShortBonusInfo (_os, JD);
		_os << std::endl;
	}
	return _os;
}

TargetSetSelectable::TargetSetSelectable (struct ln_lnlat_posn *in_obs) : TargetSet (in_obs)
{
	where = std::string ("tar_enabled = true");
	order_by = std::string ("tar_priority + tar_bonus DESC");
}

TargetSetSelectable::TargetSetSelectable (const char *target_type, struct ln_lnlat_posn *in_obs) : TargetSet (in_obs)
{
	std::ostringstream os;
	printTypeWhere (os, target_type);
	os << " AND tar_enabled = true";
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
		airmdis = rts2core::Configuration::instance()->getCalibrationAirmassDistance ();
	os << func.str () << " < " << airmdis
		<< " AND ((type_id = 'c' AND tar_id <> 6) or type_id = 'l') AND tar_enabled = true";
	ord << func.str () << " ASC";
	where = os.str ();
	order_by = ord.str ();
}

TargetSetGrb::TargetSetGrb (struct ln_lnlat_posn * in_obs, double in_altitude)
{
	obs = in_obs;
	if (!obs)
		obs = rts2core::Configuration::instance ()->getObserver ();
	obs_altitude = in_altitude;
	if (isnan (obs_altitude))
		obs_altitude = rts2core::Configuration::instance ()->getObservatoryAltitude ();
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
		TargetGRB *tar = (TargetGRB *) createTarget (*iter, obs, obs_altitude);
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

sortByAltitude::sortByAltitude (struct ln_lnlat_posn *_obs, double _jd)
{
	if (!_obs)
		observer = _obs;
	else
		observer = rts2core::Configuration::instance ()->getObserver ();
	if (isnan (_jd))
		JD = ln_get_julian_from_sys ();
	else
		JD = _jd;
}

bool sortByAltitude::doSort (Target *tar1, Target *tar2)
{
	struct ln_hrz_posn hr1, hr2;
	tar1->getAltAz (&hr1, JD, observer);
	tar2->getAltAz (&hr2, JD, observer);
	return hr1.alt > hr2.alt;
}

sortWestEast::sortWestEast (struct ln_lnlat_posn *_obs, double _jd)
{
	if (!_obs)
		observer = _obs;
	else
		observer = rts2core::Configuration::instance ()->getObserver ();
	if (isnan (_jd))
		JD = ln_get_julian_from_sys ();
	else
		JD = _jd;
}

bool sortWestEast::doSort (Target *tar1, Target *tar2)
{
	struct ln_hrz_posn hr1, hr2;
	tar1->getAltAz (&hr1, JD, observer);
	tar2->getAltAz (&hr2, JD, observer);
	bool above1 = tar1->isAboveHorizon (&hr1);
	bool above2 = tar2->isAboveHorizon (&hr2);
	if (above1 != above2)
		return above1 == true;
	// compare hour angles
	double ha1 = tar1->getHourAngle (JD, observer);
	double ha2 = tar2->getHourAngle (JD, observer);
	// ha1 on west, ha2 on east - ha1 is winner
	return ha1 > ha2;
		
}

TargetSet::iterator const rts2db::resolveAll (TargetSet *ts)
{
	return ts->end ();
}

TargetSet::iterator const rts2db::consoleResolver (TargetSet *ts)
{
	std::cout << "Please make a selection (or ctrl+c for end):" << std::endl << "  0) all" << std::endl;
	size_t i = 1;
	TargetSet::iterator iter = ts->begin ();
	for (; iter != ts->end (); i++, iter++)
	{
		std::cout << std::setw (3) << i << ")" << SEP;
		iter->second->printShortInfo (std::cout);
		std::cout << std::endl;
	}
	std::ostringstream os;
	os << "Your selection (0.." << (i - 1 ) << ")";
	int ret;
	while (true)
	{
		ret = -1;
		getMasterApp ()->askForInt (os.str ().c_str (), ret);
		if (ret >= 0 && ret <= (int) ts->size ())
			break;
	}
	if (ret == 0)
		return ts->end ();
	iter = ts->begin ();
	while (ret > 1)
	{
		iter++;
		ret--;
	}
	return iter;
}
