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

#include "rts2value.h"
#include "rts2app.h"

/**
 * Represent set of Rts2Values. It's used to store values which shall
 * be reseted when new script starts etc..
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ValueVector:public std::vector < Rts2Value * >
{
	public:
		Rts2ValueVector ()
		{
		}
		~Rts2ValueVector (void)
		{
			for (Rts2ValueVector::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}

		/**
		 * Returns iterator reference for value with given name.
		 *
		 * @param value_name Name of the value.
		 *
		 * @return Interator reference of the value with given name.
		 */
		Rts2ValueVector::iterator getValueIterator (const char *value_name)
		{
			Rts2ValueVector::iterator val_iter;
			for (val_iter = begin (); val_iter != end (); val_iter++)
			{
				Rts2Value *val;
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
		Rts2Value *getValue (const char *value_name)
		{
			Rts2ValueVector::iterator val_iter = getValueIterator (value_name);
			if (val_iter == end ())
				return NULL;
			return (*val_iter);
		}

		/**
		 * Remove value with given name from the list.
		 *
		 * @param value_name  Name of the value.
		 */
		void removeValue (const char *value_name)
		{
			Rts2ValueVector::iterator val_iter = getValueIterator (value_name);
			if (val_iter == end ())
				return;
			delete (*val_iter);
			erase (val_iter);
		}
};

/**
 * Holds state condition for value.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2CondValue
{
	private:
		Rts2Value *value;
		int stateCondition;
		int save;
	public:
		Rts2CondValue (Rts2Value * in_value, int in_stateCondition)
		{
			value = in_value;
			stateCondition = in_stateCondition;
			save = 0;
		}
		~Rts2CondValue (void) { delete value; }

		int getStateCondition () { return stateCondition; }

		bool queValueChange (int state) { return (getStateCondition () & state); }
		// mark that next operation value loads from que..
		void loadFromQue () { save |= 0x08; }
		void clearLoadedFromQue () { save &= ~0x08; }
		bool loadedFromQue () { return save & 0x08; }

		Rts2Value *getValue () { return value; }
};

/**
 * Holds cond values.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2CondValueVector:public std::vector < Rts2CondValue * >
{
	public:
		Rts2CondValueVector () {}

		~Rts2CondValueVector (void)
		{
			for (Rts2CondValueVector::iterator iter = begin (); iter != end (); iter++)
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
class Rts2ValueQue
{
	private:
		char operation;
		Rts2CondValue *old_value;
		Rts2Value *new_value;
	public:
		Rts2ValueQue (Rts2CondValue * in_old_value, char in_operation,
			Rts2Value * in_new_value)
		{
			old_value = in_old_value;
			operation = in_operation;
			new_value = in_new_value;
		}
		~Rts2ValueQue (void)
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
		Rts2CondValue *getCondValue ()
		{
			return old_value;
		}
		Rts2Value *getOldValue ()
		{
			return old_value->getValue ();
		}
		char getOperation ()
		{
			return operation;
		}
		Rts2Value *getNewValue ()
		{
			return new_value;
		}
};

/**
 * Holds Rts2ValueQue.
 *
 * @ingroup RTS2Value
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ValueQueVector:public std::vector < Rts2ValueQue * >
{
	public:
		Rts2ValueQueVector ()
		{
		}
		~Rts2ValueQueVector (void)
		{
			for (Rts2ValueQueVector::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}

		/**
		 * Query if vector contain given value.
		 *
		 * @parameter value Value which we are looking for.
		 *
		 * @return True if que value vector contain given value.
		 */
		bool contains (Rts2Value *value)
		{
			for (Rts2ValueQueVector::iterator iter = begin (); iter != end (); iter++)
				if ((*iter)->getOldValue () == value)
					return true;
			return false;
		}
};
#endif							 /* !__RTS2_VALUELIST__ */
