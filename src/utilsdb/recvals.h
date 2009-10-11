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

#ifndef __RTS2_DB_RECVALS__
#define __RTS2_DB_RECVALS__

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
		int value_type;
		time_t from;
		time_t to;
	public:
		Recval (int _recval_id, const char* _device_name, const char* _value_name, int _value_type, time_t _from, time_t _to)
		{
			recval_id = _recval_id;
			device_name = std::string(_device_name);
			value_name = std::string(_value_name);
			value_type = _value_type;
			from = _from;
			to = _to;
		}

		int getId () { return recval_id; }
		int getType () { return value_type; }
		std::string getDevice () { return device_name; }
		std::string getValueName () { return value_name; }
		time_t getFrom () { return from; };
		time_t getTo () { return to; };
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

		/**
		 * Find recval with given name.
		 *
		 * @return NULL if given name cannot be found, otherwise Recval pointer.
		 */
		Recval *searchByName (const char *_device_name, const char *_valuse_name);
};


}

#endif /* !__RTS2_DB_RECVALS__ */
