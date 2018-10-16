/*
 * Target observing proposals.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_PROP__
#define __RTS2_PROP__

#include "rts2db/target.h"
#include "rts2db/planset.h"

#include <iostream>

namespace rts2db
{

class Rts2Prop
{
	public:
		Rts2Prop ();
		Rts2Prop (int in_prop_id);
		virtual ~ Rts2Prop (void);

		int load ();
		int save ();

		Target *getTarget ();
		PlanSet *getPlanSet ();

		friend std::ostream & operator << (std::ostream & _os, __attribute__ ((unused)) Rts2Prop prop) { return _os; }

	private:
		int prop_id;
		int tar_id;
		int user_id;

		Target *target;
		PlanSet *planSet;
		void init ();
};

}

#endif							 /* !__RTS2_PROP__ */
