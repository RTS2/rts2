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
#include <ostream>

#include "target.h"

/**
 * Set of targets.
 *
 * This class holds set of targets. Constructors for filling set from the DB using various criterias are provided.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2TargetSet:public std::list <Target * >
{
	protected:
		void load (std::string in_where, std::string order_by);
		void load (std::list < int >&target_ids);

		void printTypeWhere (std::ostream & _os, const char *target_type);

		struct ln_lnlat_posn *obs;
	public:
		/**
		 * Construct set of all targets.
		 *
		 * @param in_obs Observer location.
		 * @param do_load If true, load all targets details.
		 */
		Rts2TargetSet (struct ln_lnlat_posn *in_obs, bool do_load);

		/**
		 * Construct set of all targets.
		 *
		 * @param in_obs Observer location.
		 */
		Rts2TargetSet (struct ln_lnlat_posn *in_obs = NULL);

		/**
		 * Construct set of targets around given position.
		 *
		 * @param pos RA and DEC of target point.
		 * @param radius Radius in arcdeg of the circle in which search will be performed.
		 * @param in_obs Observer location.
		 */
		Rts2TargetSet (struct ln_equ_posn *pos, double radius, struct ln_lnlat_posn *in_obs = NULL);

		/**
		 * Construct set of targets from list of targets IDs.
		 *
		 * @param tar_ids Id of targets.
		 * @param in_obs Observer location.
		 */
		Rts2TargetSet (std::list < int >&tar_ids, struct ln_lnlat_posn *in_obs = NULL);

		/**
		 * Construct set of targets with given type.
		 *
		 * @param target_type Type(s) of targets.
		 * @param in_obs Observer location.
		 */
		Rts2TargetSet (const char *target_type, struct ln_lnlat_posn *in_obs = NULL);

		virtual ~Rts2TargetSet (void);

		void setTargetEnabled (bool enabled = true, bool logit = false);
		void setTargetPriority (float new_priority);
		void setTargetBonus (float new_bonus);
		void setTargetBonusTime (time_t * new_time);
		void setNextObservable (time_t * time_ch);
		void setTargetScript (const char *device_name, const char *script);

		int save (bool overwrite = true);
		std::ostream &print (std::ostream & _os, double JD);

		std::ostream &printBonusList (std::ostream & _os, double JD);
};

class Rts2TargetSetSelectable:public Rts2TargetSet
{
	public:
		Rts2TargetSetSelectable (struct ln_lnlat_posn *in_obs = NULL);
		Rts2TargetSetSelectable (const char *target_type, struct ln_lnlat_posn *in_obs = NULL);
};

/**
 * Holds calibration targets
 */
class
Rts2TargetSetCalibration:public Rts2TargetSet
{
	public:
		Rts2TargetSetCalibration (Target * in_masterTarget, double JD, double airmdis = rts2_nan ("f"));
};

class TargetGRB;

/**
 * Holds last GRBs
 */
class  Rts2TargetSetGrb:public std::list <TargetGRB *>
{
	private:
		void load ();
	protected:

		struct ln_lnlat_posn *obs;
	public:
		Rts2TargetSetGrb (struct ln_lnlat_posn *in_obs = NULL);
		virtual ~Rts2TargetSetGrb (void);

		void printGrbList (std::ostream & _os);
};

std::ostream & operator << (std::ostream & _os, Rts2TargetSet & tar_set);
#endif							 /* !__RTS2_TARGETSET__ */
