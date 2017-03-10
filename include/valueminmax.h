/* 
 * Min-max value.
 * Copyright (C) 2007-2015 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_VALUE_MINMAX__
#define __RTS2_VALUE_MINMAX__

#include "value.h"

namespace rts2core
{

/**
 * Class holding double value with its limits.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueDoubleMinMax:public ValueDouble
{
	public:
		ValueDoubleMinMax (std::string in_val_name);
		ValueDoubleMinMax (std::string in_val_name, std::string in_description, bool writeToFits = true, int32_t flags = 0);

		virtual int setValue (Rts2Connection * connection);
		virtual int checkNotNull ();
		virtual int doOpValue (char op, Value * old_value);
		virtual const char *getValue ();
		virtual const char *getDisplayValue ();
		virtual void setFromValue (Value * newValue);

		void copyMinMax (ValueDoubleMinMax * from)
		{
			min = from->getMin ();
			max = from->getMax ();
		}

		void setMin (double in_min) { min = in_min; changed (); }

		double getMin () { return min; }

		void setMax (double in_max) { max = in_max; changed (); }

		double getMax () { return max; }

		/**
		 * @return False if new float value is invalid.
		 */
		bool testValue (double in_v) { return (in_v >= getMin () && in_v <= getMax ()); }
	private:
		double min;
		double max;
};

/**
 * Class holding integer value with its limits.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueIntegerMinMax:public ValueInteger
{
	public:
		ValueIntegerMinMax (std::string in_val_name);
		ValueIntegerMinMax (std::string in_val_name, std::string in_description, bool writeToFits = true, int32_t flags = 0);

		virtual int setValue (Rts2Connection * connection);
		virtual int checkNotNull ();
		virtual int doOpValue (char op, Value * old_value);
		virtual const char *getValue ();
		virtual const char *getDisplayValue ();
		virtual void setFromValue (Value * newValue);

		void copyMinMax (ValueIntegerMinMax * from)
		{
			min = from->getMin ();
			max = from->getMax ();
		}

		void setMin (int in_min) { min = in_min; changed (); }

		int getMin () { return min; }

		void setMax (int in_max) { max = in_max; changed (); }

		int getMax () { return max; }

		/**
		 * @return False if new value is invalid.
		 */
		bool testValue (int in_v) { return (in_v >= getMin () && in_v <= getMax ()); }
	private:
		int min;
		int max;
};

}
#endif							 /* !__RTS2_VALUE_MINMAX__ */
