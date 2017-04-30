/* 
 * Min-max value.
 * Copyright (C) 2007-2010 Petr Kubanek <petr@kubanek.net>
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

#include "valueminmax.h"
#include "connection.h"

using namespace rts2core;

ValueDoubleMinMax::ValueDoubleMinMax (std::string in_val_name):ValueDouble (in_val_name)
{
	min = NAN;
	max = NAN;
	rts2Type |= RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE;
}

ValueDoubleMinMax::ValueDoubleMinMax (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):ValueDouble (in_val_name, in_description, writeToFits, flags)
{
	min = NAN;
	max = NAN;
	rts2Type |= RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE;
}

int ValueDoubleMinMax::setValue (Connection * connection)
{
	double new_val;
	if (connection->paramNextDouble (&new_val))
		return -2;
	// if we set it only with value..
	if (connection->paramEnd ())
	{
		if (new_val < min || new_val > max)
			return -2;
		setValueDouble (new_val);
		return 0;
	}
	if (connection->paramNextDouble (&min) || connection->paramNextDouble (&max) || !connection->paramEnd ())
		return -2;
	if (new_val < min || new_val > max)
		return -2;
	setValueDouble (new_val);
	return 0;
}

int ValueDoubleMinMax::checkNotNull ()
{
	int local_failures = 0;
	if (std::isnan (min))
	{
		local_failures ++;
		logStream (MESSAGE_ERROR) << getName () << " limit (minimum) is not set" << sendLog;
	}
	if (std::isnan (max))
	{
		local_failures ++;
		logStream (MESSAGE_ERROR) << getName () << " limit (maximum) is not set" << sendLog;
	}

	return local_failures + ValueDouble::checkNotNull ();
}

int ValueDoubleMinMax::doOpValue (char op, Value * old_value)
{
	double new_val;
	switch (op)
	{
		case '+':
			new_val = old_value->getValueDouble () + getValueDouble ();
			break;
		case '-':
			new_val = old_value->getValueDouble () - getValueDouble ();
			break;
		case '=':
			new_val = getValueDouble ();
			break;
			// no op
		default:
			return -2;
	}
	if (new_val < min || new_val > max)
		return -2;
	setValueDouble (new_val);
	return 0;
}

const char * ValueDoubleMinMax::getValue ()
{
	sprintf (buf, "%.20le %.20le %.20le", getValueDouble (), getMin (), getMax ());
	return buf;
}

const char * ValueDoubleMinMax::getDisplayValue ()
{
	sprintf (buf, "%g %g %g", getValueDouble (), getMin (), getMax ());
	return buf;
}

void ValueDoubleMinMax::setFromValue (Value * newValue)
{
	ValueDouble::setFromValue (newValue);
	if (newValue->getValueType () == (RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE))
	{
		copyMinMax ((ValueDoubleMinMax *) newValue);
	}
}

ValueIntegerMinMax::ValueIntegerMinMax (std::string in_val_name):ValueInteger (in_val_name)
{
	min = INT_MIN;
	max = INT_MAX;
	rts2Type |= RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE;
}

ValueIntegerMinMax::ValueIntegerMinMax (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):ValueInteger (in_val_name, in_description, writeToFits, flags)
{
	min = INT_MIN;
	max = INT_MAX;
	rts2Type |= RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE;
}

int ValueIntegerMinMax::setValue (Connection * connection)
{
	int new_val;
	if (connection->paramNextInteger (&new_val))
		return -2;
	// if we set it only with value..
	if (connection->paramEnd ())
	{
		if (new_val < min || new_val > max)
			return -2;
		setValueInteger (new_val);
		return 0;
	}
	if (connection->paramNextInteger (&min) || connection->paramNextInteger (&max) || !connection->paramEnd ())
		return -2;
	if (new_val < min || new_val > max)
		return -2;
	setValueInteger (new_val);
	return 0;
}

int ValueIntegerMinMax::checkNotNull ()
{
	int local_failures = 0;
	if (min == INT_MIN)
	{
		local_failures ++;
		logStream (MESSAGE_ERROR) << getName () << " limit (minimum) is not set" << sendLog;
	}
	if (max == INT_MAX)
	{
		local_failures ++;
		logStream (MESSAGE_ERROR) << getName () << " limit (maximum) is not set" << sendLog;
	}
	return local_failures + ValueInteger::checkNotNull ();
}

int ValueIntegerMinMax::doOpValue (char op, Value * old_value)
{
	int new_val;
	switch (op)
	{
		case '+':
			new_val = old_value->getValueInteger () + getValueInteger ();
			break;
		case '-':
			new_val = old_value->getValueInteger () - getValueInteger ();
			break;
		case '=':
			new_val = getValueInteger ();
			break;
			// no op
		default:
			return -2;
	}
	if (new_val < min || new_val > max)
		return -2;
	setValueInteger (new_val);
	return 0;
}

const char * ValueIntegerMinMax::getValue ()
{
	sprintf (buf, "%.20le %.20le %.20le", getValueInteger (), getMin (), getMax ());
	return buf;
}

const char * ValueIntegerMinMax::getDisplayValue ()
{
	sprintf (buf, "%d %d %d", getValueInteger (), getMin (), getMax ());
	return buf;
}

void ValueIntegerMinMax::setFromValue (Value * newValue)
{
	ValueInteger::setFromValue (newValue);
	if (newValue->getValueType () == (RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE))
	{
		copyMinMax ((ValueIntegerMinMax *) newValue);
	}
}
