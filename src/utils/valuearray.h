/* 
 * Array values.
 * Copyright (C) 2008-2010 Petr Kubanek <petr@kubanek.net>
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

namespace rts2core
{

/**
 * Abstract value array class. Provides interface to basic array operations.\
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueArray: public Rts2Value
{
	public:
		ValueArray (std::string _val_name):Rts2Value (_val_name) {}
		ValueArray (std::string _val_name, std::string _description, bool writeToFits = true, int32_t flags = 0):Rts2Value (_val_name, _description, writeToFits, flags) {}

		virtual ~ValueArray () {}
		/**
		 * Return size of an array.
		 */
		virtual size_t size () = 0;
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

		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (Rts2Value * newValue);
		virtual bool isEqual (Rts2Value *other_val);

		void setValueArray (std::vector <std::string> _arr)
		{
			value = _arr;
			changed ();
		}

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

		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (Rts2Value *newValue);
		virtual bool isEqual (Rts2Value *other_val);

		void setValueArray (std::vector <double> _arr)
		{
			value = _arr;
			changed ();
		}

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

		std::vector <double>::iterator valueBegin () { return value.begin (); }

		std::vector <double>::iterator valueEnd () { return value.end (); }

		size_t size () { return value.size (); }

		void clear () { value.clear (); }

		const std::vector <double> & getValueVector () { return value; }

	private:
		std::vector <double> value;
		std::string _os;
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

		virtual int setValue (Rts2Conn * connection);
		virtual int setValueCharArr (const char *_value);
		virtual const char *getValue ();
		virtual void setFromValue (Rts2Value *newValue);
		virtual bool isEqual (Rts2Value *other_val);

		void setValueArray (std::vector <int> _arr)
		{
			value = _arr;
			changed ();
		}

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

		std::vector <int>::iterator valueBegin () { return value.begin (); }

		std::vector <int>::iterator valueEnd () { return value.end (); }

		size_t size () { return value.size (); }

		int getValueAt (int i) { return value[i]; }

		int operator[] (int i) { return value[i]; }

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

		void setValueBool (int i, bool v) { value[i] = v; }

		void addValue (bool val)
		{
			value.push_back (val);
			changed ();
		}

		virtual void setFromValue (Rts2Value *newValue);
		virtual bool isEqual (Rts2Value *other_val);

		bool operator[] (int i) { return value[i] ? true : false; }
};

}

#endif // ! __RTS2_VALUEARRAY__
