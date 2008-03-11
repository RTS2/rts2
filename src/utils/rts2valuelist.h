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
};

/**
 * Holds state condition for value.
 */
class Rts2CondValue
{
	private:
		Rts2Value *value;
		int save;
		int stateCondition;
	public:
		Rts2CondValue (Rts2Value * in_value, int in_stateCondition, bool in_save_value)
		{
			value = in_value;
			save = in_save_value ? 0x01 : 0x00;
			stateCondition = in_stateCondition;
		}
		~Rts2CondValue (void)
		{
			delete value;
		}
		int getStateCondition ()
		{
			return stateCondition;
		}
		bool queValueChange (int state)
		{
			return (getStateCondition () & state);
		}
		/**
		 * Returns true if value should be saved before it will be changed.
		 */
		bool saveValue ()
		{
			return save & 0x01;
		}
		/**
		 * Returns true if value needs to be saved.
		 */
		bool needSaveValue ()
		{
			return save == 0x01;
		}
		void setValueSave ()
		{
			save |= 0x02;
		}
		void clearValueSave ()
		{
			save &= ~0x02;
		}
		void setIgnoreSave ()
		{
			save |= 0x04;
		}
		bool ignoreLoad ()
		{
			return save & 0x04;
		}
		void clearIgnoreSave ()
		{
			save &= ~0x04;
		}
		// mark that next operation value loads from que..
		void loadFromQue ()
		{
			save |= 0x08;
		}
		void clearLoadedFromQue ()
		{
			save &= ~0x08;
		}
		bool loadedFromQue ()
		{
			return save & 0x08;
		}
		void setValueSaveAfterLoad ()
		{
		  	save |= 0x10;
		}
		void clearValueSaveAfterLoad ()
		{
		  	save &= ~0x10;
		}
		bool needClearValueSaveAfterLoad ()
		{
			return save & 0x10;
		}
		Rts2Value *getValue ()
		{
			return value;
		}
};

class Rts2CondValueVector:public std::vector < Rts2CondValue * >
{
	public:
		Rts2CondValueVector ()
		{
		}

		~Rts2CondValueVector (void)
		{
			for (Rts2CondValueVector::iterator iter = begin (); iter != end (); iter++)
				delete *iter;
		}
};

/**
 * Holds value changes which cannot be handled by device immediately.
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
