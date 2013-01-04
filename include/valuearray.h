/* 
 * Array values.
 * Copyright (C) 2008-2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics CR <kubanek@fzu.cz>
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

#include "value.h"
#include <algorithm>

namespace rts2core
{

/**
 * Abstract value array class. Provides interface to basic array operations.\
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueArray: public Value
{
	public:
		ValueArray (std::string _val_name):rts2core::Value (_val_name) {}
		ValueArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0):rts2core::Value (_val_name, _description, writeToFits, flags) {}

		virtual ~ValueArray () {}
		/**
		 * Return size of an array.
		 */
		virtual size_t size () = 0;

		virtual int setValues (std::vector <int> &index, Connection * conn) = 0;
};

/**
 * String array value. Holds unlimited number of strings.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Value
 */
class StringArray: public ValueArray
{
	public:
		StringArray (std::string _val_name);
		StringArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);	

		virtual ~StringArray () {}

		virtual int setValue (Connection * connection);
		virtual int setValues (std::vector <int> &index, Connection * conn);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (rts2core::Value * newValue);
		virtual bool isEqual (rts2core::Value *other_val);

		void setValueArray (std::vector <std::string> _arr);

		/**
		 * Add value to array.
		 *
		 * @param _val Value which will be added.
		 */
		void addValue (std::string _val)
		{
			value.push_back (_val);
			changed ();
		}

		size_t size () { return value.size (); }

		/**
		 * Returns true if given string is present in the array.
		 *
		 * @param _str String which will be searched in the array.
		 *
		 * @return True if string is present in the value array.
		 */
		bool isPresent (std::string _str) { return std::find (value.begin (), value.end (), _str) != value.end (); }

		/**
		 * Remove value from list.
		 *
		 * @param _str String which will be removed (if it exists in array)
		 */
		void remove (std::string _str) { std::vector <std::string>::iterator iter = std::find (value.begin (), value.end (), _str); if (iter != value.end ()) value.erase (iter); }

		std::vector <std::string>::iterator valueBegin () { return value.begin (); }

		std::vector <std::string>::iterator valueEnd () { return value.end (); }

		std::string operator[] (int i) { return value[i]; }

	private:
		std::vector <std::string> value;
		std::string _os;
};

/**
 * Holds array of double values.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Value
 */
class DoubleArray: public ValueArray
{
	public:
		DoubleArray (std::string _val_name);
		DoubleArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);
		
		virtual ~DoubleArray () {}

		virtual int setValue (Connection * connection);
		virtual int setValues (std::vector <int> &index, Connection * conn);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (rts2core::Value *newValue);
		virtual bool isEqual (rts2core::Value *other_val);

		void setValueArray (std::vector <double> _arr);

		/**
		 * Add value to array.
		 *
		 * @param _val Value which will be added.
		 */
		void addValue (double _val)
		{
			value.push_back (_val);
			changed ();
		}

		/**
		 * Calculate index where sum of preceding values is equal to half of the sum.
		 */
		double calculateMedianIndex ();

		std::vector <double>::iterator valueBegin () { return value.begin (); }

		std::vector <double>::iterator valueEnd () { return value.end (); }

		size_t size () { return value.size (); }

		void clear () {
			value.clear ();
			changed ();
		}

		const std::vector <double> & getValueVector () { return value; }

		double operator[] (int i) { return value[i]; }

	protected:
		std::vector <double> value;
		std::string _os;
};

/**
 * Holds times. Can hold nan times.
 */
class TimeArray:public DoubleArray
{
	public:
		TimeArray (std::string in_val_name);
		TimeArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);

		virtual const char *getDisplayValue ();

		virtual void setFromValue (rts2core::Value *newValue);
		virtual bool isEqual (rts2core::Value *other_val);
};

/**
 * Holds array of integer values.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Value
 */
class IntegerArray: public ValueArray
{
	public:
		IntegerArray (std::string _val_name);
		IntegerArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);
		
		virtual ~IntegerArray () {}

		virtual int setValue (Connection * connection);
		virtual int setValues (std::vector <int> &index, Connection * conn);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (rts2core::Value *newValue);
		virtual bool isEqual (rts2core::Value *other_val);

		void setValueInteger (int i, int v) { value[i] = v; }

		void setValueArray (std::vector <int> _arr);

		/**
		 * Add value to array.
		 *
		 * @param _val Value which will be added.
		 */
		void addValue (int _val)
		{
			value.push_back (_val);
			changed ();
		}

		/**
		 * Remove last value.
		 */
		void removeLast ()
		{
			value.pop_back ();
			changed ();
		}

		std::vector <int>::iterator valueBegin () { return value.begin (); }

		std::vector <int>::iterator valueEnd () { return value.end (); }

		size_t size () { return value.size (); }

		int getValueAt (int i) { return value[i]; }

		int operator[] (int i) { return value[i]; }

		const std::vector <int> & getValueVector () { return value; }

	protected:
		std::string _os;
		std::vector <int> value;
};

/**
 * Array for booleans.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class BoolArray: public IntegerArray
{
	public:
		BoolArray (std::string _val_name);
		BoolArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0);

		virtual const char *getDisplayValue ();

		void setValueBool (int i, bool v)
		{
			if (value[i] != v)
			{
				value[i] = v;
				changed ();
			}
		}

		void addValue (bool val)
		{
			value.push_back (val);
			changed ();
		}

		void clear ()
		{
			value.clear ();
			changed ();
		}
		virtual int setValue (Connection * connection);
		virtual int setValues (std::vector <int> &index, Connection * conn);
		virtual int setValueCharArr (const char *_value);

		virtual void setFromValue (rts2core::Value *newValue);
		virtual bool isEqual (rts2core::Value *other_val);

		void setValueArray (std::vector <bool> _arr);

		bool operator[] (int i) { return value[i] ? true : false; }

	private:
		void setFromBoolArray (std::vector <bool> _arr)
		{
			value.clear ();
			for (std::vector <bool>::iterator iter = _arr.begin (); iter != _arr.end (); iter++)
				value.push_back (*iter);
			changed ();
		}
};

}

#endif // ! __RTS2_VALUEARRAY__
