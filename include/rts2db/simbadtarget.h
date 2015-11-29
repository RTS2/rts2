/*
 * Target from SIMBAD database.
 * Copyright (C) 2005-2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2__SIMBADTARGET__
#define __RTS2__SIMBADTARGET__

#include <string>
#include <libnova/libnova.h>
#include <list>

#include "target.h"

namespace rts2db
{

/**
 * Holds information gathered from Simbad about target with given name
 * or around given location.
 */
class SimbadTarget:public ConstTarget
{
	public:
		SimbadTarget (const char *in_name);
		virtual ~SimbadTarget (void);

		virtual void load ();

		void getPosition (struct ln_equ_posn *pos) { Target::getPosition (pos); }

		virtual void printExtra (Rts2InfoValStream & _os);

		std::list < std::string > getAliases () { return aliases; }

	private:
		// target name
		std::list < std::string > aliases;
		std::string references;
		std::string simbadType;
		struct ln_equ_posn propMotions;
		float simbadBMag;
};

}

/**
 * Return new target object, created from string. String might contain RA DEC pair, MPEC one-line or any Simbad or MPEC name.
 *
 * @param tar_string  String containing target name, RA DEC position, MPEC one-line or anything else that can be usefull to
 *               identify target.
 * @param debug Print debug data.
 *
 * @return new target object. Caller must deallocate target object (delete it).
 */
rts2db::Target *createTargetByString (std::string tar_string, bool debug);

#endif							 /* !__RTS2__SIMBADTARGET__ */
