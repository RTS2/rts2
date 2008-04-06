/* 
 * User set.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_USERSET__
#define __RTS2_USERSET__

#include "rts2user.h"
#include <list>
#include <ostream>

/**
 * Set of Rts2User classes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2UserSet: public std::list <Rts2User>
{
	private:
		/**
		 * Load user list from database.
		 */
		int load ();

	public:
		/**
		 * Create user set from database.
		 */
		Rts2UserSet ();
		~Rts2UserSet (void);
};

std::ostream & operator << (std::ostream & _os, Rts2UserSet & userSet);
#endif							 /* !__RTS2_USERSET__ */
