/* 
 * Array values.
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

#ifndef __RTS2_VALUEARRAY__
#define __RTS2_VALUEARRAY__

#include "rts2value.h"
#include <algorithm>

/**
 * String array value. Holds unlimited number of strings.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Value
 */
class Rts2ValueStringArray: public Rts2Value
{
	private:
		std::vector <std::string> value;
		std::string _os;

	public:
		Rts2ValueStringArray (std::string _val_name);
		Rts2ValueStringArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);	

		virtual ~Rts2ValueStringArray ()
		{
		}

		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_val);

		void setValueArray (std::vector <std::string> _arr)
		{
			value = _arr;
		}

		/**
		 * Returns true if given string is present in the array.
		 *
		 * @param _str String which will be searched in the array.
		 *
		 * @return True if string is present in the value array.
		 */
		bool isPresent (std::string _str)
		{
			return std::find (value.begin (), value.end (), _str) != value.end ();
		}

		std::vector <std::string>::iterator valueBegin ()
		{
			return value.begin ();
		}

		std::vector <std::string>::iterator valueEnd ()
		{
			return value.end ();
		}
};

#endif // ! __RTS2_VALUEARRAY__
