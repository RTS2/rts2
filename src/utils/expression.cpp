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

#include "expression.h"

#include <ctype.h>

using namespace rts2expression;

Expression *Expression::add (op_t _op)
{
	return new ExpressionPair (this, _op, NULL);
}

double ExpressionPair::evaluate ()
{
	switch (op)
	{
		case OR:
			return exp1->evaluate () || exp2->evaluate ();
		case AND:
			return exp1->evaluate () && exp2->evaluate ();
		case XOR:
			return (exp1->evaluate () != 0) xor (exp2->evaluate () != 0);
		case EQU:
			return exp1->evaluate () == exp2->evaluate ();
		case LT:
			return exp1->evaluate () < exp2->evaluate ();
		case LEQU:
			return exp1->evaluate () <= exp2->evaluate ();
		case GT:
			return exp1->evaluate () > exp2->evaluate ();
		case GEQU:
			return exp1->evaluate () >= exp2->evaluate ();
		case NEQ:
			return exp1->evaluate () != exp2->evaluate ();
	}
	throw rts2core::Error ("unknow operand");
}

Expression *ExpressionPair::add (op_t _op)
{
	if (_op < op)
		return new ExpressionPair (this, _op, NULL);
	if (exp2 == NULL)
		throw rts2core::Error ("missing value");
	return exp2->add (_op);
}

Expression *ExpressionPair::add (Expression *_exp)
{
	if (exp2 != NULL)
		return exp2->add (_exp);
	exp2 = _exp;
	return this;
}

double ExpressionValue::evaluate ()
{
	rts2core::Value *val = ((Rts2Block *)getMasterApp ())->getValue (deviceName.c_str (), valueName.c_str ());
	if (val)
		return val->getValueDouble ();
	throw ExpressionErrorValueMissing (deviceName.c_str (), valueName.c_str ());
}

Expression * parseExpression (const char *str)
{
	Expression *new_exp = NULL;
	Expression *root_exp = NULL;
	int op = -1;
	for (int i = 0; str[i] != '\0'; i++)
	{
		if (isspace (str[i]))
			continue;
		else if (isalpha (str[i]))
		{
			int b = i;
			for (; str[i] && !isspace (str[i]); i++);
			// get string
			char val[i - b + 1];
			memcpy (val, str + b, i - b);
			val[i - b] = '\0';
			// keywords..
			if (!strcasecmp (val, "or"))
			{
				op = OR;
			}
			else if (!strcasecmp (val, "and"))
			{
				op = AND;
			}
			else if (!strcasecmp (val, "xor"))
			{
				op = XOR;
			}
			else
			{
				char *sep = strchr (val, '.');
				if (sep == NULL)
					throw rts2core::Error ("cannot find value separator (.)");
				*sep = '\0';
				sep++;
				delete new_exp;
				delete root_exp;
				new_exp = new ExpressionValue (val, sep);
			}
		}
		else if (isdigit (str[i]))
		{
			double v = 0;
			long fraction = 0;
			for (; str[i] && !isspace (str[i]); i++)
			{
				if (str[i] == '.')
				{
					if (fraction != 0)
						throw rts2core::Error ("cannot parse number");
					fraction = 10;
				}
				else if (isdigit (str[i]))
				{
					if (fraction)
					{
						v += (str[i] - '0') / fraction;
						fraction *= 10;
					}
					else
					{
						v = v * 10 + (str[i] - '0');
					}
				}
				else
				{
					delete new_exp;
					delete root_exp;
					throw rts2core::Error ("number contains unallowed characters");
				}
			}
			new_exp = new ExpressionConst (v);
		}
		else if (str[i] == '=' || str[i] == '>' || str[i] == '<' || str[i] == '!')
		{
		  	int b = i;
			i++;
			for (; str[i] && str[i] == '='; i++);
			// get string
			char val[i - b + 1];
			memcpy (val, str + b, i - b);
			val[i - b] = '\0';
			if (!strcmp (val, "=="))
			{
				op = EQU;
			}
			else if (!strcmp (val, "<"))
			{
				op = LT;
			}
			else if (!strcmp (val, "<="))
			{
				op = LEQU;
			}
			else if (!strcmp (val, ">"))
			{
				op = GT;
			}
			else if (!strcmp (val, ">="))
			{
				op = GEQU;
			}
			else if (!strcmp (val, "!="))
			{
				op = NEQ;
			}
			else
			{
				delete new_exp;
				delete root_exp;
				throw rts2core::Error ("invalid operand");
			}
		}

		if (op != -1)
		{
			root_exp = root_exp->add ((op_t) op);
			op = -1;
		}
		else if (root_exp == NULL)
		{
			root_exp = new_exp;
			new_exp = NULL;
		}
		else
		{
			try
			{
				root_exp = root_exp->add (new_exp);
				new_exp = NULL;
			}
			catch (rts2core::Error &er)
			{
				delete root_exp;
				delete new_exp;
				throw er;
			}
		}
	}
	if (root_exp)
	{
		root_exp->check ();
		return root_exp;
	}
	throw rts2core::Error ("empty expression");
}
