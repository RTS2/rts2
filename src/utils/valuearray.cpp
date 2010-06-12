/* 
 * Array values.
 * Copyright (C) 2008,2009 Petr Kubanek <petr@kubanek.net>
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
#include "valuearray.h"

#include "utilsfunc.h"

using namespace rts2core;

StringArray::StringArray (std::string _val_name)
:Rts2Value (_val_name)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_STRING;
}

StringArray::StringArray (std::string _val_name, std::string _description, bool writeToFits, int32_t flags)
:Rts2Value (_val_name, _description, writeToFits, flags)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_STRING;
}

int
StringArray::setValue (Rts2Conn * connection)
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
StringArray::setValueCharArr (const char *_value)
{
	value = SplitStr (std::string (_value), std::string (" "));
	changed ();
	return 0;
}


const char *
StringArray::getValue ()
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
StringArray::setFromValue (Rts2Value * newValue)
{
	setValueCharArr (newValue->getValue ());
}


bool
StringArray::isEqual (Rts2Value *other_val)
{
	return !strcmp (getValue (), other_val->getValue ());
}

DoubleArray::DoubleArray (std::string _val_name)
:Rts2Value (_val_name)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_DOUBLE;
}

DoubleArray::DoubleArray (std::string _val_name, std::string _description, bool writeToFits, int32_t flags)
:Rts2Value (_val_name, _description, writeToFits, flags)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_DOUBLE;
}

int
DoubleArray::setValue (Rts2Conn * connection)
{
	value.clear ();

	while (!(connection->paramEnd ()))
	{
		double nextVal;
		int ret = connection->paramNextDouble (&nextVal);
		if (ret)
			return -2;
		value.push_back (nextVal);
	}
	changed ();
	return 0;
}


int
DoubleArray::setValueCharArr (const char *_value)
{
	std::vector <std::string> sv = SplitStr (std::string (_value), std::string (" "));
	for (std::vector <std::string>::iterator iter = sv.begin (); iter != sv.end (); iter++)
	{
		value.push_back (atof ((*iter).c_str ()));
	}
	changed ();
	return 0;
}


const char *
DoubleArray::getValue ()
{
	std::ostringstream oss;
	std::vector <double>::iterator iter = value.begin ();
	oss.setf (std::ios_base::fixed, std::ios_base::floatfield);
	while (iter != value.end ())
	{
		oss << (*iter);
		iter++;
		if (iter == value.end ())
			break;
		oss << std::string (" ");
	}
	_os = oss.str ();
	return _os.c_str ();
}


void
DoubleArray::setFromValue (Rts2Value * newValue)
{
	if (newValue->getValueType () == (RTS2_VALUE_ARRAY | RTS2_VALUE_DOUBLE))
	{
		value.clear ();
		DoubleArray *nv = (DoubleArray *) newValue;
		for (std::vector <double>::iterator iter = nv->valueBegin (); iter != nv->valueEnd (); iter++)
			value.push_back (*iter);
		changed ();
	}
	else
	{
		setValueCharArr (newValue->getValue ());
	}
}


bool
DoubleArray::isEqual (Rts2Value *other_val)
{
	if (other_val->getValueType () == (RTS2_VALUE_ARRAY | RTS2_VALUE_DOUBLE))
	{
		DoubleArray *ov = (DoubleArray *) other_val;
		if (ov->size () != value.size ())
			return false;
		
		std::vector <double>::iterator iter1;
		std::vector <double>::iterator iter2;
		for (iter1 = valueBegin (), iter2 = ov->valueBegin (); iter1 != valueEnd () && iter2 != ov->valueEnd (); iter1++, iter2++)
		{
			if (*iter1 != *iter2)
				return false;
		}
		return true;
	}
	return false;
}


IntegerArray::IntegerArray (std::string _val_name)
:Rts2Value (_val_name)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_INTEGER;
}

IntegerArray::IntegerArray (std::string _val_name, std::string _description, bool writeToFits, int32_t flags)
:Rts2Value (_val_name, _description, writeToFits, flags)
{
	rts2Type |= RTS2_VALUE_ARRAY | RTS2_VALUE_INTEGER;
}

int
IntegerArray::setValue (Rts2Conn * connection)
{
	value.clear ();

	while (!(connection->paramEnd ()))
	{
		int nextVal;
		int ret = connection->paramNextInteger (&nextVal);
		if (ret)
			return -2;
		value.push_back (nextVal);
	}
	changed ();
	return 0;
}


int
IntegerArray::setValueCharArr (const char *_value)
{
	std::vector <std::string> sv = SplitStr (std::string (_value), std::string (" "));
	for (std::vector <std::string>::iterator iter = sv.begin (); iter != sv.end (); iter++)
	{
		value.push_back (atoi ((*iter).c_str ()));
	}
	changed ();
	return 0;
}


const char *
IntegerArray::getValue ()
{
	std::ostringstream oss;
	std::vector <int>::iterator iter = value.begin ();
	oss.setf (std::ios_base::fixed, std::ios_base::floatfield);
	while (iter != value.end ())
	{
		oss << (*iter);
		iter++;
		if (iter == value.end ())
			break;
		oss << std::string (" ");
	}
	_os = oss.str ();
	return _os.c_str ();
}


void
IntegerArray::setFromValue (Rts2Value * newValue)
{
	if (newValue->getValueType () == (RTS2_VALUE_ARRAY | RTS2_VALUE_INTEGER))
	{
		value.clear ();
		IntegerArray *nv = (IntegerArray *) newValue;
		for (std::vector <int>::iterator iter = nv->valueBegin (); iter != nv->valueEnd (); iter++)
			value.push_back (*iter);
		changed ();
	}
	else
	{
		setValueCharArr (newValue->getValue ());
	}
}


bool
IntegerArray::isEqual (Rts2Value *other_val)
{
	if (other_val->getValueType () == (RTS2_VALUE_ARRAY | RTS2_VALUE_INTEGER))
	{
		IntegerArray *ov = (IntegerArray *) other_val;
		if (ov->size () != value.size ())
			return false;
		
		std::vector <int>::iterator iter1;
		std::vector <int>::iterator iter2;
		for (iter1 = valueBegin (), iter2 = ov->valueBegin (); iter1 != valueEnd () && iter2 != ov->valueEnd (); iter1++, iter2++)
		{
			if (*iter1 != *iter2)
				return false;
		}
		return true;
	}
	return false;
}
