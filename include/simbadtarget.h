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

#include "rts2target.h"

namespace rts2core
{

/**
 * Holds information gathered from Simbad about target with given name
 * or around given location.
 */
class SimbadTarget
{
	public:
		SimbadTarget ();
		virtual ~SimbadTarget (void);

		void resolve (const char *tarname);

		void getSimbadPosition (struct ln_equ_posn *pos) { pos->ra = position.ra; pos->dec = position.dec; }

	protected:
		std::string references;
		struct ln_equ_posn propMotions;
		std::string simbadName;
		std::string simbadType;
		float simbadBMag;
		std::list < std::string > aliases;

	private:

		struct ln_equ_posn position;
};

}

#endif // !__RTS2__SIMBADTARGET__
