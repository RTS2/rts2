/* 
 * Constraints.
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

#include "target.h"

namespace rts2db
{

/**
 * Abstract class for constraints.
 */
class Constraint
{
	public:
		Constraint () {};
		
		virtual bool satisfy (Target *tar, double JD) = 0;
};

class ConstraintTimeInterval:public Constraint
{
	public:
		ConstraintTimeInterval (double _from, double _to) { from = _from; to = _to; }

		virtual bool satisfy (Target *tar, double JD);
	private:
		double from;
		double to;
};

class ConstraintDoubleInterval:public Constraint
{
	public:
		ConstraintDoubleInterval (double _lower, double _upper) { lower = _lower; upper = _upper; }
	protected:
		double getLower () { return lower; }
		double getUpper () { return upper; }
	private:
		double lower;
		double upper;
};

class ConstraintAirmass:public ConstraintDoubleInterval
{
	public:
		virtual bool satisfy (Target *tar, double JD);
};

class Constraints:public std::vector <Constraints *>
{
	public:
		Constraints () {}
		bool satisfy (Target *tar, double JD);
};

}
