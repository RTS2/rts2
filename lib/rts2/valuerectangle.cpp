/* 
 * Class for rectangle.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "valuerectangle.h"
#include "connection.h"

using namespace rts2core;

void ValueRectangle::createValues ()
{
	x = newValue (rts2Type & RTS2_BASE_TYPE, getName () + ".X", getDescription () + " X");
	y = newValue (rts2Type & RTS2_BASE_TYPE, getName () + ".Y", getDescription () + " Y");
	w = newValue (rts2Type & RTS2_BASE_TYPE, getName () + ".WIDTH", getDescription () + " WIDTH");
	h = newValue (rts2Type & RTS2_BASE_TYPE, getName () + ".HEIGHT", getDescription () + " HEIGHT");
}

void ValueRectangle::checkChange ()
{
	if (x->wasChanged () || y->wasChanged () || w->wasChanged () || h->wasChanged ())
		changed ();
}

ValueRectangle::ValueRectangle (std::string in_val_name, int32_t baseType):Value (in_val_name)
{
	rts2Type = RTS2_VALUE_RECTANGLE | baseType;
	createValues ();
}

ValueRectangle::ValueRectangle (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Value (in_val_name, in_description, writeToFits, flags)
{
	rts2Type = RTS2_VALUE_RECTANGLE | flags;
	createValues ();
}

ValueRectangle::~ValueRectangle (void)
{
	delete x;
	delete y;
	delete w;
	delete h;
}

void ValueRectangle::setInts (int in_x, int in_y, int in_w, int in_h)
{
	x->setValueInteger (in_x);
	y->setValueInteger (in_y);
	w->setValueInteger (in_w);
	h->setValueInteger (in_h);
	checkChange ();
}

int ValueRectangle::setValue (Connection *connection)
{
	char *val;
	if (connection->paramNextString (&val) || x->setValueCharArr (val))
		return -2;
	if (connection->paramNextString (&val) || y->setValueCharArr (val))
		return -2;
	if (connection->paramNextString (&val) || w->setValueCharArr (val))
		return -2;
	if (connection->paramNextString (&val) || h->setValueCharArr (val))
		return -2;
	checkChange ();
	return connection->paramEnd () ? 0 : -2;
}

int ValueRectangle::setValueCharArr (const char *in_value)
{
	std::vector <std::string> spl = SplitStr (std::string (in_value), std::string (":"));
	if (spl.size () != 4)
		return -1;

	x->setValueCharArr (spl[0].c_str ());
	y->setValueCharArr (spl[1].c_str ());
	w->setValueCharArr (spl[2].c_str ());
	h->setValueCharArr (spl[3].c_str ());
	checkChange ();
	return 0;
}

int ValueRectangle::setValueInteger (__attribute__ ((unused)) int in_value)
{
	return -1;
}

const char * ValueRectangle::getValue ()
{
	sprintf (buf, "%s %s %s %s", x->getValue (), y->getValue (), w->getValue (), h->getValue ());
	return buf;
}

void ValueRectangle::setFromValue(Value * new_value)
{
	if (new_value->getValueExtType () == RTS2_VALUE_RECTANGLE)
	{
		x->setFromValue (((ValueRectangle *)new_value)->getX ());
		y->setFromValue (((ValueRectangle *)new_value)->getY ());
		w->setFromValue (((ValueRectangle *)new_value)->getWidth ());
		h->setFromValue (((ValueRectangle *)new_value)->getHeight ());
		checkChange ();
	}
}

bool ValueRectangle::isEqual (Value *other_value)
{
	if (other_value->getValueExtType () == RTS2_VALUE_RECTANGLE)
	{
		return x->isEqual (((ValueRectangle *)other_value)->getX ())
			&& y->isEqual (((ValueRectangle *)other_value)->getY ())
			&& w->isEqual (((ValueRectangle *)other_value)->getWidth ())
			&& h->isEqual (((ValueRectangle *)other_value)->getHeight ());
	}
	return false;
}

const char *ValueRectangle::getDisplayValue ()
{
	switch (getValueBaseType ())
	{
		case RTS2_VALUE_INTEGER:
			snprintf (buf, VALUE_BUF_LEN, "[%d:%d,%d:%d]", x->getValueInteger (), w->getValueInteger (), y->getValueInteger (), h->getValueInteger ());
			break;
		case RTS2_VALUE_LONGINT:
			snprintf (buf, VALUE_BUF_LEN, "[%ld:%ld,%ld:%ld]", x->getValueLong (), w->getValueLong (), y->getValueLong (), h->getValueLong ());
			break;
		case RTS2_VALUE_FLOAT:
		case RTS2_VALUE_DOUBLE:
			snprintf (buf, VALUE_BUF_LEN, "[%g:%g,%g:%g]", x->getValueDouble (), w->getValueDouble (), y->getValueDouble (), h->getValueDouble ());
			break;
	}
	return buf;
}

const char *ValueRectangle::getDisplaySubValue (const char *subv)
{
	if (strcasecmp (subv, "x") == 0)
		return x->getDisplayValue ();
	else if (strcasecmp (subv, "y") == 0)
		return y->getDisplayValue ();
	else if (strcasecmp (subv, "w") == 0)
		return w->getDisplayValue ();
	else if (strcasecmp (subv, "h") == 0)
		return h->getDisplayValue ();
	return getDisplayValue ();
}

void ValueRectangle::resetValueChanged ()
{
	x->resetValueChanged ();
	y->resetValueChanged ();
	w->resetValueChanged ();
	h->resetValueChanged ();
	Value::resetValueChanged ();
}
