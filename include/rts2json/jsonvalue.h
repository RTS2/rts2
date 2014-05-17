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

#ifndef __RTS2_JSONVALUE__
#define __RTS2_JSONVALUE__

#include "value.h"
#include "connection.h"
#include "httpreq.h"

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

/**
 * JSON String representation. Escapes string ", \n etc so the string will be
 * safe to transport in JSON as value.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class JsonString
{
	public:
		JsonString (std::string s)
		{
			setChar (s.c_str ());
		}

		JsonString (const char *s)
		{
			setChar (s);
		}

		friend std::ostream & operator << (std::ostream &os, JsonString s)
		{
			os << s.sv;
			return os;
		}

	private:
		void setChar (const char *s)
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

		std::string sv;
};

/**
 * Support class/interface for operations needed by XmlDevClient and XmlDevClientCamera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DevInterface
{
	public:
		DevInterface ():changedTimes () {}

		double getValueChangedTime (rts2core::Value *value);

	protected:
		// value change times
		std::map <rts2core::Value *, double> changedTimes;
};

void sendArrayValue (rts2core::Value *value, std::ostringstream &os);

void sendStatValue (rts2core::Value *value, std::ostringstream &os);

void sendRectangleValue (rts2core::Value *value, std::ostringstream &os);

void sendValue (rts2core::Value *value, std::ostringstream &os);

// encode value to JSON
void jsonValue (rts2core::Value *value, bool extended, std::ostringstream & os);

/**
 * Send connection values as JSON string to the client.
 *
 * @param time from which changed values will be reported. nan means that all values will be reported.
 */
void sendConnectionValues (std::ostringstream &os, rts2core::Connection * conn, XmlRpc::HttpParams *params, double from = NAN, bool extended = false);
}

#endif // !__RTS2_JSONVALUE__
