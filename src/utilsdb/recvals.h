/* 
 * Utility classes for record values manipulations.
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

#include <list>
#include <string>

namespace rts2db
{

/**
 * Class representing a record value.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Recval
{
	private:
		int recval_id;
		std::string device_name;
		std::string value_name;
		time_t from;
		time_t to;
		int numvals;
	public:
		Recval (int _recval_id, const char* _device_name, const char* _value_name, time_t _from, time_t _to, int _numvals)
		{
			recval_id = _recval_id;
			device_name = std::string(_device_name);
			value_name = std::string(_value_name);
			from = _from;
			to = _to;
			numvals = _numvals;
		}

		int getId () { return recval_id; }
		std::string getDevice () { return device_name; }
		std::string getValueName () { return value_name; }
		time_t getFrom () { return from; };
		time_t getTo () { return to; };
		int getNumRecs () { return numvals; }
};

/**
 * Class with value records.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class RecvalsSet: public std::list <Recval>
{
	public:
		RecvalsSet ()
		{
		}

		void load ();
};


}
