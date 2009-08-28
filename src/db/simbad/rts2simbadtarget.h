/*
 * Target from SIMBAD database.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "../../utilsdb/target.h"

/**
 * Holds information gathered from Simbad about target with given name
 * or around given location.
 */
class Rts2SimbadTarget:public ConstTarget
{
	private:
		// target name
		std::list < std::string > aliases;
		std::string references;
		std::string simbadType;
		struct ln_equ_posn propMotions;
		float simbadBMag;

		std::ostringstream *simbadOut;

		// holds http_proxy string..
		char *http_proxy;
	public:
		Rts2SimbadTarget (const char *in_name);
		virtual ~Rts2SimbadTarget (void);

		virtual int load ();

		void getPosition (struct ln_equ_posn *pos)
		{
			Target::getPosition (pos);
		}

		virtual void printExtra (Rts2InfoValStream & _os);

		std::list < std::string > getAliases ()
		{
			return aliases;
		}
};
#endif							 /* !__RTS2__SIMBADTARGET__ */
