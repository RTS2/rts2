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

#include "rts2conn.h"
#include "rts2valuearray.h"

#include "utilsfunc.h"

Rts2ValueStringArray::Rts2ValueStringArray (std::string _val_name)
:Rts2Value (_val_name)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_STRING;
}

Rts2ValueStringArray::Rts2ValueStringArray (std::string _val_name, std::string _description, bool writeToFits, int32_t flags)
:Rts2Value (_val_name, _description, writeToFits, flags)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_STRING;
}

int
Rts2ValueStringArray::setValue (Rts2Conn * connection)
{
	value.clear ();

	while (!(connection->paramEnd ()))
	{
		char *nextVal;
		int ret = connection->paramNextString (&nextVal);
		if (ret)
			return -2;
		value.push_back (std::string (nextVal));
	}
	changed ();
	return 0;
}


int
Rts2ValueStringArray::setValueCharArr (const char *_value)
{
	value = SplitStr (std::string (_value), std::string (" "));
	changed ();
	return 0;
}


const char *
Rts2ValueStringArray::getValue ()
{
	_os = std::string ();
	std::vector <std::string>::iterator iter = value.begin ();
	while (iter != value.end ())
	{
		_os += (*iter);
		iter++;
		if (iter == value.end ())
			break;
		_os += std::string (" ");
	}
	return _os.c_str ();
}


void
Rts2ValueStringArray::setFromValue (Rts2Value * newValue)
{
	setValueCharArr (newValue->getValue ());
}


bool
Rts2ValueStringArray::isEqual (Rts2Value *other_val)
{
	return !strcmp (getValue (), other_val->getValue ());
}
