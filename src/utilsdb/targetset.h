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

#ifndef __RTS2_TARGETSET__
#define __RTS2_TARGETSET__

#include <libnova/libnova.h>
#include <list>
#include <map>
#include <vector>
#include <ostream>

#include "../utils/utilsfunc.h"

namespace rts2db
{

class Target;
class TargetGRB;

class Constraints;

/**
 * Error class for addSet operation.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class NotDisjunct
{
	public:
		/**
		 * Construct not disjunct exception.
		 *
		 * @param _targetId ID of target which is not disjunt in the set.
		 */
		NotDisjunct (int _targetId) { targetId = _targetId; }
	private:
		int targetId;
};

/**
 * Error class when target with given ID is not found.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TargetNotFound
{
	public:
		/**
		 * Construct the TargetNotFound execption.
		 *
		 * @param _id Id of target which was not found in the set.
		 */
		TargetNotFound (int _id) { id = _id; }
	private:
		int id;
};

/**
 * Set of targets.
 *
 * This class holds set of targets. Constructors for filling set from the DB
 * using various criterias are provided. The load () method must be called to
 * actually load targets.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TargetSet:public std::map <int, Target * >
{
	public:
		/**
		 * Construct set of all targets.
		 *
		 * @param in_obs Observer location.
		 */
		TargetSet (struct ln_lnlat_posn *in_obs = NULL);

		/**
		 * Construct set of targets around given position.
		 *
		 * @param pos RA and DEC of target point.
		 * @param radius Radius in arcdeg of the circle in which search will be performed.
		 * @param in_obs Observer location.
		 */
		TargetSet (struct ln_equ_posn *pos, double radius, struct ln_lnlat_posn *in_obs = NULL);

		/**
		 * Construct set of targets with given type.
		 *
		 * @param target_type Type(s) of targets.
		 * @param in_obs Observer location.
		 */
		TargetSet (const char *target_type, struct ln_lnlat_posn *in_obs = NULL);

		virtual ~TargetSet (void);

		/**
		 * Load target set from database.
		 *
		 * @throw SqlError if target set cannot be loaded.
		 */
		virtual void load ();

		/**
		 * Create set from given target ids.
		 *
		 * @throw SqlError when some target cannot be created.
		 */
		void load (std::list < int >&target_ids);

		/**
		 * Load targets with name matching pattern.
		 *
		 * @param name  target name. Spaces in names are ignored
		 *
		 * @throw SqlError if no target is found.
		 */
		void load (const char *name);

		void load (std::vector <const char *> &names, TargetSet::iterator const (*multiple_resolver) (TargetSet *ts) = NULL);

		/**
		 * Add to target set targets from the other set.
		 *
		 * @param _set Other set.
		 *
		 * @throw NotDisjunct(target_id) if sets have not empty join set. Parameter is id of target which is in both.
		 */
		void addSet (TargetSet &_set);

		/**
		 * Finds in set target with a given id.
		 *
		 * @param _id Id to find.
		 *
		 * @return Target with a given id.
		 *
		 * @throw TargetNotFound error if not found.
		 */
		Target *getTarget (int _id);

		void setTargetEnabled (bool enabled = true, bool logit = false);
		void setTargetPriority (float new_priority);
		void setTargetBonus (float new_bonus);
		void setTargetBonusTime (time_t * new_time);
		void setNextObservable (time_t * time_ch);
		void setTargetScript (const char *device_name, const char *script);

		/**
		 * Set targets constraints.
		 *
		 * @throw  rts2core::Error if output file cannot be created.
		 */
		void setConstraints (Constraints &cons);

		int save (bool overwrite = true);
		std::ostream &print (std::ostream & _os, double JD);

		std::ostream &printBonusList (std::ostream & _os, double JD);

	protected:
		void printTypeWhere (std::ostream & _os, const char *target_type);

		struct ln_lnlat_posn *obs;

		// values for load operation
		std::string where;
		std::string order_by;
};

class TargetSetSelectable:public TargetSet
{
	public:
		TargetSetSelectable (struct ln_lnlat_posn *in_obs = NULL);
		TargetSetSelectable (const char *target_type, struct ln_lnlat_posn *in_obs = NULL);
};

/**
 * Create list of targets with similar name. The search is done by:
 *  - replacing all spaces in name with %
 *  - prefix and suffix name with %
 *  - select from target database every target, which name is like name created.
 * Like operator works by searching for string which match given string. % represent any character.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TargetSetByName:public TargetSet
{
	public:
		TargetSetByName (const char *name);
};

/**
 * Holds calibration targets
 */
class TargetSetCalibration:public TargetSet
{
	public:
		TargetSetCalibration (Target * in_masterTarget, double JD, double airmdis = rts2_nan ("f"));
};

/**
 * Holds last GRBs.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class  TargetSetGrb:public std::vector <TargetGRB *>
{
	public:
		TargetSetGrb (struct ln_lnlat_posn *in_obs = NULL);
		virtual ~TargetSetGrb (void);

		virtual void load ();

		void printGrbList (std::ostream & _os);

	protected:
		struct ln_lnlat_posn *obs;
};


/**
 * Singleton which holds list of all targets.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TargetSetSingleton
{
	public:
		TargetSetSingleton () {}

		static TargetSet *instance ()
		{
			static TargetSet *pInstance;
			if (pInstance == NULL)
			{
				pInstance = new TargetSet ();
				pInstance->load ();
			}
			return pInstance; 
		}
};

}

std::ostream & operator << (std::ostream & _os, rts2db::TargetSet & tar_set);

#endif							 /* !__RTS2_TARGETSET__ */
