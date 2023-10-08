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

#include "rts2script/script.h"
#include "rts2script/operands.h"

using namespace rts2operands;

double Operand::getDouble ()
{
	throw rts2script::ParsingError ("Operand does not support conversion to double");
}

double SystemValue::getDouble ()
{
	rts2core::Connection *conn = master->getOpenConnection (device.c_str ());
	if (conn == NULL)
		throw rts2script::ParsingError ("Cannot find device");
	return conn->getValueDouble (value.c_str ());
}

Operand *OperandsSet::parseOperand (std::string str, mulType_t mulType)
{
	// let' see what we have as an operand..
	std::string::iterator iter = str.begin ();
	while (iter != str.end () && isspace (*iter))
		iter++;
	if (iter == str.end ())
	  	throw rts2script::ParsingError ("Empty string");
	// start as number..
	if (isdigit(*iter) || *iter == '-' || *iter == '+' || *iter == '.')
	{
		// parse as string..
		double op = 0;
		std::string::iterator it = str.begin ();
		while (true)
		{
		  	// current number
			double mul = 1;

		  	double cn = 0;
			bool dec_seen = false;
			int sign = 1;

			if (*it == '-')
			{
			  	sign = -1;
				it++;
			}
			else if (*it == '+')
			{
			  	it++;
			}

			while (it != str.end () && (isdigit(*it) || *it == '.'))
			{
				if (*it == '.')
				{
				  	if (dec_seen == true)
					  	throw rts2script::ParsingError ("multiple decimal points");
				  	dec_seen = true;
					mul = 0.1;
				}
				else
				{
					if (dec_seen)
					{
						cn += (*it - '0') * mul;
						mul /= 10;
					}
					else
					{
				  		cn = cn * 10 + (*it - '0');
					}
				}
				it++;
			}

			// get the number
			if (it == str.end ())
			  	mul = 1;
			else switch (mulType)
			{
				case MUL_ANGLE:
					switch (*it)
					{
						case 'd':
							mul = 1;
							break;
						case 'h':
							mul = 15;
							break;
						case 'm':
						case '\'':
			  				mul = 1/60.0;
							break;
						case 's':
						case '\"':
							mul = 1/3600.0;
							break;
						default:
			  				throw rts2script::UnknowOperantMultiplier (*it);
					}
					break;
				case MUL_TIME:
					switch (*it)
					{
						case 'd':
			  				mul = 86400;
							break;
						case 'h':
			  				mul = 3600;
							break;
						case 'w':
			  				mul = 7 * 86400;
							break;
						default:
			  				throw rts2script::UnknowOperantMultiplier (*it);
					}
					break;
			}
			// eats units specifications
			op += sign * cn * mul;
			if (it == str.end ())
				break;
			it++;
		}
		return new Number (op);
	}
	else
	{
		int start = iter - str.begin ();
		while (iter != str.end() && (isalnum (*iter) || *iter == '_' || *iter == '+' || *iter == '-' || *iter == '.'))
			iter++;
		// see what we get..
		std::string name = str.substr (start, iter - str.begin () - start);
		if (name == "rand")
		{
			// get two parameters as operands..
			std::string ops = str.substr (start + 4);
			OperandsSet twoOps;
			twoOps.parse (ops);
			if (twoOps.size () != 2)
				throw rts2script::ParsingError ("Invalid number of parameters - expecting two:" + ops);
			Operand *ret = new RandomNumber (twoOps[0], twoOps[1]);
			// do not delete operands!
			twoOps.clear ();
			return ret;
		}
		else
		{
			if (iter != str.end ())
				throw rts2script::ParsingError ("Cannot find function with name " + name);
			return new String (name);
		}
	}
}

void OperandsSet::parse (std::string str)
{
	// find operators separators..
	bool bracked = false;
	int simple_braces = 0;
	int curved_braces = 0;
	enum {NONE, SIMPLE, DOUBLE} quotes = NONE;
	int start = 0;
	for (std::string::iterator iter = str.begin(); iter != str.end(); iter++)
	{
		if (quotes != NONE)
		{
			if (*iter == '\'' && quotes == SIMPLE)
				quotes = NONE;
			if (*iter == '"' && quotes == DOUBLE)
				quotes = NONE;
		}
		else
		{
			if (*iter == '(')
			{
				if (iter == str.begin ())
				{
					start++;
					bracked = true;
				}
				simple_braces++;
			}
			if (*iter == ')')
			{
				if (simple_braces > 0)
					simple_braces --;
				else
					throw rts2script::ParsingError ("too many closing simple braces - )");
			}
			if (*iter == '{')
			  	curved_braces++;
			if (*iter == '}')
			{
				if (curved_braces > 0)
					curved_braces --;
				else
					throw rts2script::ParsingError ("too many closing curved braces - }");
			}
			if (*iter == '\'')
				quotes = SIMPLE;
			if (*iter == '"')
				quotes = DOUBLE;
			if (curved_braces == 0 && quotes == NONE &&
			  	((*iter == ',' && simple_braces == 1) || (simple_braces == 0 && bracked)))
			{
				std::string ops = str.substr (start, iter - str.begin () - start);
				push_back (parseOperand (ops, MUL_ANGLE));
				start = iter - str.begin () + 1;
			}
		}
	}
	if (bracked == false)
	{
		// push back single operator
		push_back (parseOperand (str, MUL_ANGLE));
	}
}

double OperandsLREquation::getDouble ()
{
	double lv = l->getDouble ();
	double rv = r->getDouble ();
	switch (cmp)
	{
		case CMP_EQUAL:
			return lv == rv;
		case CMP_LESS:
			return lv < rv;
		case CMP_LESS_EQU:
			return lv <= rv;
		case CMP_GREAT_EQU:
			return lv >= rv;
		case CMP_GREAT:
			return lv > rv;
	}
	std::ostringstream _os;
	_os << "Unknown comparator " << cmp;
	throw rts2script::ParsingError (_os.str ());
}

const char* OperandsLREquation::getCmpSymbol ()
{
	const char *cmp_sym[] = { "==", "<", "<=", ">=", ">" };
	return cmp_sym[cmp];
}
