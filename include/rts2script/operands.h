/*
 * Scripting operands.
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

#ifndef __RTS2_OPERANDS__
#define __RTS2_OPERANDS__

#include "error.h"
#include "utilsfunc.h"
#include "block.h"

#include <vector>
#include <ostream>

namespace rts2operands
{

typedef enum { MUL_ANGLE, MUL_TIME } mulType_t;

/**
 * Abstract class for an operand.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Operand
{
	public:
		Operand () {};
		virtual ~Operand () {}

		virtual std::ostream & writeTo (std::ostream &_os) = 0;

		// return as number..
		virtual double getDouble ();

		// return as string..

		friend std::ostream & operator << (std::ostream &_os, Operand &_op)
		{
			return _op.writeTo (_os);
		}
};

/**
 * Use as operand constant number.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Number:public Operand
{
	public:
		Number (double _val) { val = _val; }
		virtual double getDouble () { return val; }
		virtual std::ostream & writeTo (std::ostream &_os) { _os << getDouble (); return _os; }
	private:
		double val;
};


/**
 * System value. This can be for example CCD temperature.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class SystemValue:public Operand
{
	public:
		SystemValue (rts2core::Block *_master, std::string _device, std::string _value):Operand () { master = _master; device = _device; value = _value; }
		virtual double getDouble ();
		virtual std::ostream & writeTo (std::ostream &_os) { _os << getDouble (); return _os; }
	private:
		rts2core::Block *master;
		std::string device;
		std::string value;
};


/**
 * String constant.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class String:public Operand
{
	public:
		String(std::string  _str): str(_str) {}
		
		virtual std::ostream & writeTo (std::ostream &_os) { _os << str; return _os; }
	private:
		std::string str;
};

/**
 * Operand is random number in a given range.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class RandomNumber:public Operand
{
	public:
		/**
		 * @param _from  Interval lower limit.
		 * @param _top   Interval upper limit.
		 */
		RandomNumber(Operand *_from, Operand *_to) { from = _from; to = _to; }
		~RandomNumber () { delete from; delete to; }

		virtual double getDouble () { double f = from->getDouble (); return f + (to->getDouble () - f) * random_num (); }

		virtual std::ostream & writeTo (std::ostream &_os) { _os << getDouble (); return _os; }
	
	private:
		Operand *from;
		Operand *to;
};

/**
 * Set of operands, forming paramateres for command.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OperandsSet:public std::vector <Operand*>
{
	public:
		OperandsSet () {};
		~OperandsSet () { for (OperandsSet::iterator iter = begin (); iter != end (); iter ++) delete (*iter); }

		Operand *parseOperand (std::string str, mulType_t mulType);

		/**
		 * Parse given string, fill operands with Operand childs.
		 *
		 * @param str String which will be parsed.
		 */
		void parse (std::string str);

		friend std::ostream & operator  << (std::ostream &_os, OperandsSet &_operands)
		{
			for (OperandsSet::iterator iter = _operands.begin (); iter != _operands.end (); iter++)
			{
				if (iter != _operands.begin ())
					_os << " ";
				_os << *(*iter);
			}
			return _os;
		}
};

/**
 * Allowed comparators.
 */
enum comparators { CMP_EQUAL, CMP_LESS, CMP_LESS_EQU, CMP_GREAT_EQU, CMP_GREAT };

/**
 * Equation - either 
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OperandsLREquation:public Operand
{
	public:
		OperandsLREquation (Operand *_l, comparators _cmp, Operand *_r):Operand () { l = _l; cmp = _cmp; r = _r; }
		/**
		 * Evaluate equation. Return 0 if it false, otherwise 1.
		 */
		virtual double getDouble ();
		
		virtual std::ostream & writeTo (std::ostream &_os) { _os << l->getDouble () << getCmpSymbol () << r->getDouble (); return _os; }
	private:
		Operand *l;
		comparators cmp;
		Operand *r;

		const char *getCmpSymbol ();
};

}

#endif // !__RTS2_OPERANDS__
