/* 
 * Utility classes for record values.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DB_RECORDS__
#define __RTS2_DB_RECORDS__

#include <list>
#include <string>

#include "../utils/error.h"
#include "../utils/value.h"
#include "../utils/utilsfunc.h"

namespace rts2db
{

/**
 * Class representing a record (value).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Record
{
	public:
		Record (double _rectime, double _val)
		{
			rectime = _rectime;
			val = _val;
		}

		double getRecTime () { return rectime; };
		double getValue () { return val; };

	private:
		double rectime;
		double val;
};

/**
 * Class with value records.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class RecordsSet: public std::list <Record>
{
	public:
		RecordsSet (int _recval_id)
		{
			recval_id = _recval_id;
			value_type = -1;

			min = max = rts2_nan ("f");
		}

		/**
		 * @throw SqlError on errror.
		 */
		void load (double t_from, double t_to);

		double getMin () { return min; };
		double getMax () { return max; };

	private:
		int recval_id;
		int value_type;

		// get value type..
		int getValueType ();

		// get base type
		int getValueBaseType () { return getValueType () & RTS2_BASE_TYPE; }

		void loadState (double t_from, double t_to);
		void loadDouble (double t_from, double t_to);
		void loadBoolean (double t_from, double t_to);

		// minmal and maximal values..
		double min;
		double max;
};

}

#endif /* !__RTS2_DB_RECORDS__ */
