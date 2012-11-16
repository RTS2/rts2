/* 
 * Various value classes.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include <math.h>
#include <stdio.h>
#include <sstream>

#include "libnova_cpp.h"
#include "block.h"
#include "value.h"
#include "timestamp.h"

#include "radecparser.h"

using namespace rts2core;

Value::Value (std::string _val_name)
{
	valueName = _val_name;
	rts2Type = 0;
}

Value::Value (std::string _val_name, std::string _description, bool writeToFits, int32_t flags)
{
	valueName = _val_name;
	rts2Type = 0;
	description = _description;
	if (writeToFits)
		setWriteToFits ();
	setValueFlags (flags);
}

int Value::checkNotNull ()
{
	logStream (MESSAGE_ERROR) << getName () << " not set" << sendLog;
	return 1;
}

int Value::doOpValue (char op, Value * old_value)
{
	switch (op)
	{
		case '=':
			return 0;
		default:
			logStream (MESSAGE_ERROR) << "unknow op '" << op << "' for type " << getValueType () << sendLog;
			return -1;
	}
}

int Value::sendTypeMetaInfo (Connection * connection)
{
	std::ostringstream _os;
	_os << PROTO_METAINFO << " " << rts2Type << " " << '"' << getName () << "\" " << '"' << description << "\" ";
	return connection->sendMsg (_os.str ());
}

int Value::sendMetaInfo (Connection * connection)
{
	int ret;
	ret = sendTypeMetaInfo (connection);
	if (ret < 0)
		return ret;

	send (connection);
	return 0;
}

void Value::send (Connection * connection)
{
	connection->sendValueRaw (getName (), getValue ());
}

ValueString::ValueString (std::string in_val_name): Value (in_val_name)
{
	rts2Type |= RTS2_VALUE_STRING;
}

ValueString::ValueString (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags): Value (in_val_name, in_description, writeToFits, flags)
{
	rts2Type |= RTS2_VALUE_STRING;
}

const char * ValueString::getValue ()
{
	return value.c_str ();
}

int ValueString::setValue (Connection * connection)
{
	char *new_value;
	if (connection->paramNextString (&new_value) || !connection->paramEnd ())
		return -2;
	setValueCharArr (new_value);
	return 0;
}

int ValueString::setValueCharArr (const char *_value)
{
	if (_value == NULL)
	{
		_value = "";
	}
	if (value != std::string (_value))
		changed ();

	value = std::string (_value);
	return 0;
}

int ValueString::setValueInteger (int in_value)
{
	std::ostringstream _os;
	_os << in_value;
	value = _os.str ();
	changed ();
	return 0;
}

void ValueString::send (Connection * connection)
{
	connection->sendValue (getName (), getValue ());
}

void ValueString::setFromValue (Value * newValue)
{
	setValueCharArr (newValue->getValue ());
}

bool ValueString::isEqual (Value *other_value)
{
	if (getValue () == NULL || other_value->getValue () == NULL)
		return getValue () == other_value->getValue ();
	return strcmp (getValue (), other_value->getValue ()) == 0;
}

int ValueString::checkNotNull ()
{
	if (value.length () > 0)
		return 0;
	return Value::checkNotNull ();
}

ValueInteger::ValueInteger (std::string in_val_name): Value (in_val_name)
{
	value = 0;
	rts2Type |= RTS2_VALUE_INTEGER;
}

ValueInteger::ValueInteger (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags): Value (in_val_name, in_description, writeToFits, flags)
{
	value = (flags & RTS2_VALUE_NOTNULL) ? INT_MIN : 0;
	rts2Type |= RTS2_VALUE_INTEGER;
}

const char * ValueInteger::getValue ()
{
	sprintf (buf, "%i", value);
	return buf;
}

int ValueInteger::setValue (Connection * connection)
{
	int new_value;
	if (connection->paramNextInteger (&new_value) || !connection->paramEnd ())
		return -2;
	if (value != new_value)
		changed ();
	value = new_value;
	return 0;
}

int ValueInteger::setValueCharArr (const char *in_value)
{
	return setValueInteger (atoi (in_value));
}

int ValueInteger::doOpValue (char op, Value * old_value)
{
	switch (op)
	{
		case '+':
			return setValueInteger (old_value->getValueInteger () + getValueInteger ());
		case '-':
			return setValueInteger (old_value->getValueInteger () - getValueInteger ());
		case '=':
			return setValueInteger (getValueInteger ());
	}
	return Value::doOpValue (op, old_value);
}

void ValueInteger::setFromValue (Value * newValue)
{
	setValueInteger (newValue->getValueInteger ());
}

bool ValueInteger::isEqual (Value * other_value)
{
	return getValueInteger () == other_value->getValueInteger ();
}

int ValueInteger::checkNotNull ()
{
	if (value != INT_MIN)
		return 0;
	return Value::checkNotNull ();
}

ValueDouble::ValueDouble (std::string in_val_name):Value (in_val_name)
{
	value = NAN;
	rts2Type |= RTS2_VALUE_DOUBLE;
}

ValueDouble::ValueDouble (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags): Value (in_val_name, in_description, writeToFits, flags)
{
	value = NAN;
	rts2Type |= RTS2_VALUE_DOUBLE;
}

const char * ValueDouble::getValue ()
{
	sprintf (buf, "%.20le", value);
	return buf;
}

const char * ValueDouble::getDisplayValue ()
{
	sprintf (buf, "%.20lg", value);
	return buf;
}

int ValueDouble::setValue (Connection * connection)
{
	double new_value;
	if (connection->paramNextDouble (&new_value) || !connection->paramEnd ())
		return -2;
	if (value != new_value)
		changed ();
	value = new_value;
	return 0;
}

int ValueDouble::setValueCharArr (const char *in_value)
{
	setValueDouble (atof (in_value));
	return 0;
}

int ValueDouble::setValueInteger (int in_value)
{
	setValueDouble (in_value);
	return 0;
}

int ValueDouble::doOpValue (char op, Value * old_value)
{
	switch (op)
	{
		case '+':
			setValueDouble (old_value->getValueDouble () + getValueDouble ());
			break;
		case '-':
			setValueDouble (old_value->getValueDouble () - getValueDouble ());
			break;
		default:
			return Value::doOpValue (op, old_value);
	}
	return 0;
}

void ValueDouble::setValueDouble (double in_value)
{
	if (value != in_value)
	{
		changed ();
		if (getValueDisplayType () == RTS2_DT_DEG_DIST_180)
		{
			value = ln_range_degrees (in_value);
			if (value > 180.0)
				value -= 360.0;
		}
		else
		{
			value = in_value;
		}
	}
}

void ValueDouble::setFromValue (Value * newValue)
{
	setValueDouble (newValue->getValueDouble ());
}

bool ValueDouble::isEqual (Value *other_value)
{
	return getValueDouble () == other_value->getValueDouble ();
}

int ValueDouble::checkNotNull ()
{
	if (!isnan (value))
		return 0;
	return Value::checkNotNull ();
}

ValueTime::ValueTime (std::string in_val_name):ValueDouble (in_val_name)
{
	rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_TIME;
}

ValueTime::ValueTime (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags): ValueDouble (in_val_name, in_description, writeToFits, flags)
{
	rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_TIME;
}

const char * ValueTime::getDisplayValue ()
{
	struct timeval infot;
	gettimeofday (&infot, NULL);

	std::ostringstream _os;
	_os << Timestamp (getValueDouble ())
		<< " (" << TimeDiff (infot.tv_sec + (double) infot.tv_usec / USEC_SEC, getValueDouble ()) << ")";

	strncpy (buf, _os.str ().c_str (), VALUE_BUF_LEN - 1);
	return buf;
}

void ValueTime::setNow ()
{
	struct timeval t_val;
	gettimeofday (&t_val, NULL);
	setValueDouble (t_val.tv_sec + ((double) t_val.tv_usec) / USEC_SEC);
}

void ValueTime::getStructTm (struct tm *tm_s, long *usec)
{
	time_t t_val = (time_t) getValueLong ();
	gmtime_r (&t_val, tm_s);
	*usec = (long) ((getValueDouble () - getValueLong ()) * USEC_SEC);
}

void ValueTime::getValueTime (struct timeval &tv)
{
	tv.tv_sec = (time_t) (floor (getValueDouble ()));
	tv.tv_usec = (long) ((getValueDouble () - tv.tv_sec) * USEC_SEC);
}

ValueFloat::ValueFloat (std::string in_val_name): Value (in_val_name)
{
	value = NAN;
	rts2Type |= RTS2_VALUE_FLOAT;
}

ValueFloat::ValueFloat (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags): Value (in_val_name, in_description, writeToFits, flags)
{
	value = NAN;
	rts2Type |= RTS2_VALUE_FLOAT;
}

const char * ValueFloat::getValue ()
{
	sprintf (buf, "%.20e", value);
	return buf;
}

const char * ValueFloat::getDisplayValue ()
{
	double absv = fabs (value);
	if ((absv > 10e-3 && absv < 10e+5) || absv == 0)
		sprintf (buf, "%lf", value);
	else
		sprintf (buf, "%.20le", value);
	return buf;
}

int ValueFloat::setValue (Connection * connection)
{
	float new_value;
	if (connection->paramNextFloat (&new_value) || !connection->paramEnd ())
		return -2;
	if (value != new_value)
		changed ();
	value = new_value;
	return 0;
}

int ValueFloat::setValueCharArr (const char *in_value)
{
	setValueDouble (atof (in_value));
	return 0;
}

int ValueFloat::setValueInteger (int in_value)
{
	setValueDouble (in_value);
	return 0;
}

int ValueFloat::doOpValue (char op, Value * old_value)
{
	switch (op)
	{
		case '+':
			setValueFloat (old_value->getValueFloat () + getValueFloat ());
			break;
		case '-':
			setValueFloat (old_value->getValueFloat () - getValueFloat ());
			break;
		default:
			return Value::doOpValue (op, old_value);
	}
	return 0;
}

void ValueFloat::setFromValue (Value * newValue)
{
	setValueFloat (newValue->getValueFloat ());
}

bool ValueFloat::isEqual (Value * other_value)
{
	return getValueFloat () == other_value->getValueFloat ();
}

ValueBool::ValueBool (std::string in_val_name):ValueInteger (in_val_name)
{
	rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_BOOL;
	setValueInteger (2);
}

ValueBool::ValueBool (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):ValueInteger (in_val_name, in_description, writeToFits, flags)
{
	rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_BOOL;
	setValueInteger (2);
}

int ValueBool::setValue (Connection * connection)
{
	char *new_value;
	int ret;
	if (connection->paramNextString (&new_value) || !connection->paramEnd ())
		return -2;
	ret = setValueCharArr (new_value);
	if (ret)
		return -2;
	return 0;
}

int ValueBool::setValueCharArr (const char *in_value)
{
	bool b;
	int ret = charToBool (in_value, b);
	if (ret)
		return ret;
	setValueBool (b);
	return 0;
}

const char * ValueBool::getDisplayValue ()
{
	if (getValueDisplayType () & RTS2_DT_ONOFF)
		return getValueBool ()? "on" : "off";
	return getValueBool ()? "true" : "false";
}

ValueSelection::ValueSelection (std::string in_val_name):ValueInteger (in_val_name)
{
	rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_SELECTION;
}

ValueSelection::ValueSelection (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):ValueInteger (in_val_name, in_description, writeToFits, flags)
{
	rts2Type = (~RTS2_VALUE_MASK & rts2Type) | RTS2_VALUE_SELECTION;
}

ValueSelection::~ValueSelection ()
{
	values.clear ();
}

void ValueSelection::deleteValues ()
{
	for (std::vector <SelVal>::iterator iter=selBegin (); iter!= selEnd (); iter++)
	{
		delete (*iter).data;
	}
	values.clear ();
}

int ValueSelection::setValue (Connection * connection)
{
	char *new_value;
	if (connection->paramNextString (&new_value) || !connection->paramEnd ())
		return -2;
	// try if it's number
	int ret;
	ret = setValueCharArr (new_value);
	if (ret)
		return -2;
	return 0;
}

int ValueSelection::setValueCharArr (const char *in_value)
{
	char *end;
	int ret = strtol (in_value, &end, 10);
	if (!*end)
	{
		return setValueInteger (ret);
	}
	ret = getSelIndex (std::string (in_value));
	if (ret < 0)
		return -1;
	return setValueInteger (ret);
}

int ValueSelection::doOpValue (char op, Value * old_value)
{
	switch (op)
	{
		int new_val;
		case '+':
			return setValueInteger ((old_value->getValueInteger () + getValueInteger ()) % values.size ());
		case '-':
			new_val = old_value->getValueInteger () - getValueInteger ();
			while (new_val < 0)
				new_val += values.size ();
			return setValueInteger (new_val);
	}
	return ValueInteger::doOpValue (op, old_value);
}

int ValueSelection::getSelIndex (std::string in_val)
{
	int i = 0;
	for (std::vector < SelVal >::iterator iter = selBegin (); iter != selEnd (); iter++, i++)
	{
		if ((*iter).name == in_val)
			return i;
	}
	return -2;
}

void ValueSelection::copySel (ValueSelection * sel)
{
	for (std::vector < SelVal >::iterator iter = sel->selBegin (); iter != sel->selEnd (); iter++)
	{
		addSelVal (*iter);
	}
}

void ValueSelection::addSelVals (const char **vals)
{
	while (*vals != NULL)
	{
		addSelVal (*vals);
		vals++;
	}
}

int ValueSelection::sendSelections (Connection * connection)
{
	// now send selection values..
	std::ostringstream _os;

	// empty PROTO_SELMETAINFO means delete selections
	_os << PROTO_SELMETAINFO << " \"" << getName () << "\"\n";

	for (std::vector < SelVal >::iterator iter = selBegin (); iter != selEnd (); iter++)
	{
		std::string val = (*iter).name;
		_os << PROTO_SELMETAINFO << " \"" << getName () << "\" \"" << val << "\"\n";
	}

	return connection->sendMsg (_os.str ());
}

int ValueSelection::sendTypeMetaInfo (Connection * connection)
{
	int ret;
	ret = ValueInteger::sendTypeMetaInfo (connection);
	if (ret)
		return ret;
	return sendSelections (connection);
}

void ValueSelection::duplicateSelVals (ValueSelection * otherValue)
{
	deleteValues ();
	for (std::vector < SelVal >::iterator iter = otherValue->selBegin (); iter != otherValue->selEnd (); iter++)
	{
		addSelVal (*iter);
	}
}

ValueLong::ValueLong (std::string in_val_name): Value (in_val_name)
{
	value = 0;
	rts2Type |= RTS2_VALUE_LONGINT;
}


ValueLong::ValueLong (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags): Value (in_val_name, in_description, writeToFits, flags)
{
	value = 0;
	rts2Type |= RTS2_VALUE_LONGINT;
}

const char * ValueLong::getValue ()
{
	sprintf (buf, "%li", value);
	return buf;
}

int ValueLong::setValue (Connection * connection)
{
	long int new_value;
	if (connection->paramNextLong (&new_value) || !connection->paramEnd ())
		return -2;
	if (value != new_value)
		changed ();
	value = new_value;
	return 0;
}

int ValueLong::setValueCharArr (const char *in_value)
{
	return setValueLong (atol (in_value));
}

int ValueLong::setValueInteger (int in_value)
{
	return setValueLong (in_value);
}

int ValueLong::doOpValue (char op, Value * old_value)
{
	switch (op)
	{
		case '+':
			return setValueLong (old_value->getValueLong () + getValueLong ());
		case '-':
			return setValueLong (old_value->getValueLong () - getValueLong ());
		case '=':
			return setValueLong (getValueLong ());
	}
	return Value::doOpValue (op, old_value);
}

void ValueLong::setFromValue (Value * newValue)
{
	setValueLong (newValue->getValueLong ());
}

bool ValueLong::isEqual (Value * other_value)
{
	return getValueLong () == other_value->getValueLong ();
}

ValueRaDec::ValueRaDec (std::string in_val_name):Value (in_val_name)
{
	ra = NAN;
	decl = NAN;
	rts2Type |= RTS2_VALUE_RADEC;
}

ValueRaDec::ValueRaDec (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Value (in_val_name, in_description, writeToFits, flags)
{
	ra = NAN;
	decl = NAN;
	rts2Type |= RTS2_VALUE_RADEC;
}

int ValueRaDec::setValue (Connection * connection)
{
	double r, d;
	if (connection->paramNextDMS (&r))
		return -2;
	if (connection->paramEnd ())
	{
		setValueRaDec (r, r);
		return 0;
	}
	if (connection->paramNextDMS (&d) || !connection->paramEnd ())
		return -2;
	setValueRaDec (r, d);
	return 0;
}

int ValueRaDec::setValueCharArr (const char *in_value)
{
	double v_ra, v_dec;
	if (parseRaDec (in_value, v_ra, v_dec))
		return -2;
	setValueRaDec (v_ra, v_dec);
	return 0;
}

void ValueRaDec::setRa (double in_ra)
{
	if (getValueDisplayType () == RTS2_DT_DEG_DIST_180)
	{
		// normalize RA
		in_ra = ln_range_degrees (in_ra);
		if (in_ra > 180.0)
			in_ra -= 360.0;
	}
	if (ra != in_ra)
	{
		ra = in_ra;
		changed ();
	}
}

void ValueRaDec::setDec (double in_dec)
{
	// normalize DEC
	if (getValueDisplayType () == RTS2_DT_DEG_DIST_180)
	{
		in_dec = ln_range_degrees (in_dec);
		if (in_dec > 180.0)
			in_dec -= 360.0;
	}
	if (decl != in_dec)
	{
		decl = in_dec;
		changed ();
	}
}

int ValueRaDec::doOpValue (char op, Value *old_value)
{
	switch (old_value->getValueType ())
	{
		case RTS2_VALUE_RADEC:
			switch (op)
			{
				case '+':
					setValueRaDec (ra + ((ValueRaDec *)old_value)->getRa (), decl + ((ValueRaDec *)old_value)->getDec ());
					return 0;
				case '-':
					setValueRaDec (ra - ((ValueRaDec *)old_value)->getRa (), decl - ((ValueRaDec *)old_value)->getDec ());
					return 0;
				default:
					return Value::doOpValue (op, old_value);
			}
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_FLOAT:
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
			switch (op)
			{
				case '+':
					setValueRaDec (ra + old_value->getValueDouble (), decl + old_value->getValueDouble ());
					return 0;
				case '-':
					setValueRaDec (ra - old_value->getValueDouble (), decl - old_value->getValueDouble ());
					return 0;
				default:
					return Value::doOpValue (op, old_value);
			}
		default:
			logStream (MESSAGE_ERROR) << "Do not know how to handle operation '" << op
				<< "' between RADEC value and " << old_value->getValueType ()
				<< sendLog;
			return -1;
	}
}

const char * ValueRaDec::getValue ()
{
	std::ostringstream _os;
	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	_os.precision (20);
	_os << getRa () << " " << getDec ();

	strcpy (buf, _os.str ().c_str ());

	return buf;
}

void ValueRaDec::setFromValue (Value * newValue)
{
	if (newValue->getValueType () == RTS2_VALUE_RADEC)
	{
		setValueRaDec (((ValueRaDec *)newValue)->getRa (),
			((ValueRaDec *)newValue)->getDec ());
	}
	else
	{
		setValueCharArr (newValue->getValue ());
	}
}

bool ValueRaDec::isEqual (Value *other_value)
{
	if (other_value->getValueType () == RTS2_VALUE_RADEC)
	{
		return getRa () == ((ValueRaDec*)other_value)->getRa ()
			&& getDec () == ((ValueRaDec*)other_value)->getDec ();
	}
	return false;
}

ValueAltAz::ValueAltAz (std::string in_val_name):Value (in_val_name)
{
	alt = NAN;
	az = NAN;
	rts2Type |= RTS2_VALUE_ALTAZ;
}

ValueAltAz::ValueAltAz (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Value (in_val_name, in_description, writeToFits, flags)
{
	alt = NAN;
	az = NAN;
	rts2Type |= RTS2_VALUE_ALTAZ;
}

int ValueAltAz::setValue (Connection * connection)
{
	if (connection->paramNextDouble (&alt))
		return -2;
	if (connection->paramEnd ())
	{
	  	az = alt;
		return 0;
	}
	if (connection->paramNextDouble (&az) || !connection->paramEnd ())
		return -2;

	return 0;
}

int ValueAltAz::setValueCharArr (const char *in_value)
{
	double v_alt, v_az;
	if (parseRaDec (in_value, v_alt, v_az))
		return -2;
	setValueAltAz (v_alt, v_az);
	return 0;
}

int ValueAltAz::doOpValue (char op, Value *old_value)
{
	switch (old_value->getValueType ())
	{
		case RTS2_VALUE_ALTAZ:
			switch (op)
			{
				case '+':
					setValueAltAz (alt + ((ValueAltAz *)old_value)->getAlt (), az + ((ValueAltAz *)old_value)->getAz ());
					return 0;
				case '-':
					setValueAltAz (alt - ((ValueAltAz *)old_value)->getAlt (), az - ((ValueAltAz *)old_value)->getAz ());
					return 0;
				default:
					return Value::doOpValue (op, old_value);
			}
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_FLOAT:
		case RTS2_VALUE_INTEGER:
		case RTS2_VALUE_LONGINT:
			switch (op)
			{
				case '+':
					setValueAltAz (alt + old_value->getValueDouble (), az + old_value->getValueDouble ());
					return 0;
				case '-':
					setValueAltAz (alt - old_value->getValueDouble (), az - old_value->getValueDouble ());
					return 0;
				default:
					return Value::doOpValue (op, old_value);
			}
		default:
			logStream (MESSAGE_ERROR) << "Do not know how to handle operation '" << op
				<< "' between RADEC value and " << old_value->getValueType ()
				<< sendLog;
			return -1;
	}
}

const char * ValueAltAz::getValue ()
{
	std::ostringstream _os;
	_os << getAlt () << " " << getAz ();

	strcpy (buf, _os.str ().c_str ());

	return buf;
}

void ValueAltAz::setFromValue (Value * newValue)
{
	if (newValue->getValueType () == RTS2_VALUE_ALTAZ)
	{
		setValueAltAz (((ValueAltAz *)newValue)->getAlt (),
			((ValueAltAz *)newValue)->getAz ());
	}
	else
	{
		setValueCharArr (newValue->getValue ());
	}
}

bool ValueAltAz::isEqual (Value *other_value)
{
	if (other_value->getValueType () == RTS2_VALUE_ALTAZ)
	{
		return getAlt () == ((ValueAltAz*)other_value)->getAlt ()
			&& getAz () == ((ValueAltAz*)other_value)->getAz ();
	}
	return false;
}

Value *newValue (int rts2Type, std::string name, std::string desc)
{
	switch (rts2Type & RTS2_BASE_TYPE)
	{
		case RTS2_VALUE_STRING:
			return new ValueString (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_INTEGER:
			return new ValueInteger (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_TIME:
			return new ValueTime (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_DOUBLE:
			return new ValueDouble (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_FLOAT:
			return new ValueFloat (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_BOOL:
			return new ValueBool (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_SELECTION:
			return new ValueSelection (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_LONGINT:
			return new ValueLong (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_RADEC:
			return new ValueRaDec (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
		case RTS2_VALUE_ALTAZ:
			return new ValueAltAz (name, desc, rts2Type & RTS2_VALUE_FITS, rts2Type);
	}
	logStream (MESSAGE_ERROR) << "unknow value name: " << name << " type: " << rts2Type << sendLog;
	return NULL;
}
