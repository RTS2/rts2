/* 
 * Expression support.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_EXPRESSION__
#define __RTS2_EXPRESSION__

#include "block.h"
#include "error.h"

#include <ostream>

namespace rts2expression
{

typedef enum {OR, AND, XOR, EQU, LT, LEQU, GT, GEQU, NEQ} op_t;

class Expression
{
	public:
		virtual ~Expression () {};
		virtual double evaluate () = 0;

		virtual Expression* add (op_t _op);
		virtual Expression* add (__attribute__ ((unused)) Expression *_exp) { throw rts2core::Error ("missing operand"); }

		/**
		 * Throw error if expression is not complete.
		 */
		virtual void check () {}
};

class ExpressionPair:public Expression
{
	public:
		ExpressionPair (Expression *_exp1, op_t _op, Expression *_exp2) { exp1 = _exp1; op = _op; exp2 = _exp2; }
		virtual ~ExpressionPair () { delete exp1; delete exp2; }
		virtual double evaluate ();

		virtual Expression *add (op_t _op);
		virtual Expression *add (Expression *_exp);

		virtual void check () { if (exp1 == NULL) throw rts2core::Error ("left side is missing"); if (exp2 == NULL) throw rts2core::Error ("right side is missing"); exp1->check (); exp2->check (); }
	private:
		Expression *exp1;
		Expression *exp2;
		op_t op;
};

class ExpressionConst:public Expression
{
	public:
		ExpressionConst (double _val) { val = _val; }
		virtual double evaluate () { return val; }
	private:
		double val;
};

class ExpressionValue:public Expression
{
	public:
		ExpressionValue (const char *_device, const char *_value)
		{
			deviceName = std::string (_device);
			valueName = std::string (_value);
		}
		virtual double evaluate ();
	private:
		std::string deviceName;
		std::string valueName;
};

class ExpressionErrorValueMissing:public rts2core::Error
{
	public:
		ExpressionErrorValueMissing (const char *_device, const char *_value): rts2core::Error() { setMsg (std::string ("missing value ") + _device + "." + _value); }
};

}

rts2expression::Expression * parseExpression (const char *str);
#endif //! __RTS2_EXPRESSION__
