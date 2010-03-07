/*
 * Variables which do some basic statistics computations.
 * Copyright (C) 2007 - 2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_VALUESTAT__
#define __RTS2_VALUESTAT__

#include "rts2value.h"

#include <deque>

/**
 * This class holds values which were obtained from series
 * of measurements.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ValueDoubleStat:public Rts2ValueDouble
{
	public:
		Rts2ValueDoubleStat (std::string in_val_name);
		Rts2ValueDoubleStat (std::string in_val_name, std::string in_description, bool writeToFits = true, int32_t flags = 0);

		/**
		 * Clear values in value list.
		 */
		void clearStat ();

		/**
		 * Calculate values statistics.
		 */
		void calculate ();

		virtual int setValue (Rts2Conn * connection);
		virtual const char *getValue ();
		virtual const char *getDisplayValue ();
		virtual void send (Rts2Conn * connection);
		virtual void setFromValue (Rts2Value * newValue);

		int getNumMes () { return numMes; }

		double getMode () { return mode; }

		/**
		 * Return minimal value.
		 *
		 * @return Minimal value.
		 */
		double getMin () { return min; }

		/**
		 * Return maximal value.
		 *
		 * @return Maximal value.
		 */
		double getMax () { return max; }

		double getStdev () { return stdev; }

		std::deque < double >&getMesList () { return valueList; }

		/**
		 * Add value to the measurement values.
		 *
		 * @param in_val Value which will be added.
		 */
		void addValue (double in_val)
		{
			valueList.push_back (in_val);
			changed ();
		}

		/**
		 * Add value to the measurement values. If que size is greater then
		 * maxQueSize, delete first entry.
		 *
		 * @param in_val        Value which will be added.
		 * @param maxQueSize    Maximal que size.
		 */
		void addValue (double in_val, size_t maxQueSize)
		{
			while (valueList.size () >= maxQueSize)
				valueList.pop_front ();
			addValue (in_val);
			changed ();
		}
		std::deque <double>::iterator valueBegin () { return valueList.begin (); }
		std::deque <double>::iterator valueEnd () { return valueList.end (); }
	private:
		int numMes;
		double mode;
		double min;
		double max;
		double stdev;
		std::deque < double >valueList;
};
#endif							 /* !__RTS2_VALUESTAT__ */
