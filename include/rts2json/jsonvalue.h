/* 
 * JSON value encoding.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "value.h"

#include <iomanip>

namespace rts2json
{

/**
 * Display double value as JSON - instead of nan, write null.
 *
 * @author Petr Kubanek
 */
class JsonDouble
{
	public:
		JsonDouble (double _v) { v = _v; }
		friend std::ostream & operator << (std::ostream &os, JsonDouble d)
		{
			if (isnan (d.v) || isinf (d.v))
				os << "null";
			else
				os << std::setprecision (20) << d.v;
			return os;
		}

	private:
		double v;
};

class JsonString
{
	public:
		JsonString (const char *s)
		{
			for (const char *ic = s; *ic; ic++)
			{
				// switch for special characters..
				switch (*ic)
				{
					case '"':
						sv += "\\\"";
						break;
					case '\\':
						sv += "\\\\";
						break;
					case '/':
						sv += "\\/";
						break;
					case '\b':
						sv += "\\b";
						break;
					case '\f':
						sv += "\\f";
						break;
					case '\n':
						sv += "\\n";
						break;
					case '\r':
						sv += "\\r";
						break;
					case '\t':
						sv += "\\t";
						break;
					default:
						if (isgraph (*ic) || isspace (*ic))
							sv += *ic;
				}
			}
		}
		friend std::ostream & operator << (std::ostream &os, JsonString s)
		{
			os << s.sv;
			return os;
		}

	private:
		std::string sv;
};

}

void sendArrayValue (rts2core::Value *value, std::ostringstream &os);

void sendStatValue (rts2core::Value *value, std::ostringstream &os);

void sendRectangleValue (rts2core::Value *value, std::ostringstream &os);

void sendValue (rts2core::Value *value, std::ostringstream &os);

// encode value to JSON
void jsonValue (rts2core::Value *value, bool extended, std::ostringstream & os);
