/*
 * Database target from SIMBAD database.
 * Copyright (C) 2005-2016 Petr Kubanek <petr@kubanek.net>
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

#include "simbadtarget.h"
#include "rts2db/simbadtargetdb.h"

using namespace rts2db;

SimbadTargetDb::SimbadTargetDb (const char *in_name):ConstTarget (), rts2core::SimbadTarget ()
{
	setTargetName (in_name);
}

SimbadTargetDb::~SimbadTargetDb (void)
{
}

void SimbadTargetDb::load ()
{
	rts2core::SimbadTarget::resolve (getTargetName ());
	setTargetName (simbadName.c_str ());
	struct ln_equ_posn pos;
	getSimbadPosition (&pos);
	setPosition (pos.ra, pos.dec);
}

void SimbadTargetDb::printExtra (Rts2InfoValStream & _ivs)
{
	ConstTarget::printExtra (_ivs, ln_get_julian_from_sys ());

	if (_ivs.getStream ())
	{
		std::ostream* _os = _ivs.getStream ();
		*_os << "REFERENCED " << references << std::endl;
		int old_prec = _os->precision (2);
		*_os << "PROPER MOTION RA " <<
			(propMotions.ra * 360000.0)
			<< " DEC " << (propMotions.dec * 360000.0) << " (mas/y)";
		*_os << "TYPE " << simbadType << std::endl;
		*_os << "B MAG " << simbadBMag << std::endl;
		_os->precision (old_prec);

		for (std::list < std::string >::iterator alias = aliases.begin ();
			alias != aliases.end (); alias++)
		{
			*_os << "ALIAS " << (*alias) << std::endl;
		}
	}
}
