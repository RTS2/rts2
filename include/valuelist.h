/* 
 * List of values.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_VALUELIST__
#define __RTS2_VALUELIST__

#include <vector>

#include "value.h"
#include "app.h"

namespace rts2core
{

/**
 * Represent set of Values. It's used to store values which shall
 * be reseted when new script starts etc..
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueVector:public std::vector < Value * >
{
	public:
		ValueVector ()
		{
		}
		~ValueVector (void)
		{
			for (ValueVector::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}

		/**
		 * Returns iterator reference for value with given name.
		 *
		 * @param value_name Name of the value.
		 *
		 * @return Interator reference of the value with given name.
		 */
		ValueVector::iterator getValueIterator (const char *value_name)
		{
			ValueVector::iterator val_iter;
			for (val_iter = begin (); val_iter != end (); val_iter++)
			{
				Value *val;
				val = (*val_iter);
				if (val->isValue (value_name))
					return val_iter;
			}
			return val_iter;
		}

		/**
		 * Search for value by value name.
		 *
		 * @param value_name  Name of the searched value.
		 *
		 * @return  Value object of value with given name, or NULL if value with this name does not exists.
		 */
		Value *getValue (const char *value_name)
		{
			ValueVector::iterator val_iter = getValueIterator (value_name);
			if (val_iter == end ())
				return NULL;
			return (*val_iter);
		}

		/**
		 * Remove value with given name from the list.
		 *
		 * @param value_name  Name of the value.
		 */
		ValueVector::iterator removeValue (const char *value_name)
		{
			ValueVector::iterator val_iter = getValueIterator (value_name);
			if (val_iter == end ())
				return val_iter;
			delete (*val_iter);
			val_iter = erase (val_iter);
			return val_iter;
		}
};

/**
 * Holds state condition for value.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class CondValue
{
	public:
		CondValue (Value * in_value, int in_stateCondition)
		{
			value = in_value;
			stateCondition = in_stateCondition;
			save = 0;
		}
		~CondValue (void) { delete value; }

		int getStateCondition () { return stateCondition; }
		void setStateCondition (int new_cond) { stateCondition = new_cond; }

		bool queValueChange (int state) { return (getStateCondition () & state); }
		// mark that next operation value loads from que..
		void loadFromQue () { save |= 0x08; }
		void clearLoadedFromQue () { save &= ~0x08; }
		bool loadedFromQue () { return save & 0x08; }

		Value *getValue () { return value; }

	private:
		Value *value;
		int stateCondition;
		int save;
};

/**
 * Holds cond values.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class CondValueVector:public std::vector < CondValue * >
{
	public:
		CondValueVector () {}

		~CondValueVector (void)
		{
			for (CondValueVector::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}
};

/**
 * Holds value changes which cannot be handled by device immediately.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueQue
{
	public:
		ValueQue (CondValue * in_old_value, char in_operation, Value * in_new_value)
		{
			old_value = in_old_value;
			operation = in_operation;
			new_value = in_new_value;
		}
		~ValueQue (void)
		{
		}
		int getStateCondition ()
		{
			return old_value->getStateCondition ();
		}
		bool queValueChange (int state)
		{
			return (getStateCondition () & state);
		}
		CondValue *getCondValue ()
		{
			return old_value;
		}
		Value *getOldValue ()
		{
			return old_value->getValue ();
		}
		char getOperation ()
		{
			return operation;
		}
		Value *getNewValue ()
		{
			return new_value;
		}
	private:
		char operation;
		CondValue *old_value;
		Value *new_value;
};

/**
 * Holds ValueQue.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValueQueVector:public std::vector < ValueQue * >
{
	public:
		ValueQueVector ()
		{
		}
		~ValueQueVector (void)
		{
			for (ValueQueVector::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}

		/**
		 * Query if vector contain given value.
		 *
		 * @parameter value Value which we are looking for.
		 *
		 * @return True if que value vector contain given value.
		 */
		bool contains (Value *value)
		{
			for (ValueQueVector::iterator iter = begin (); iter != end (); iter++)
				if ((*iter)->getOldValue () == value)
					return true;
			return false;
		}
};

}
#endif							 /* !__RTS2_VALUELIST__ */
