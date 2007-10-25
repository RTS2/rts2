#include "rts2valueminmax.h"
#include "rts2conn.h"

Rts2ValueDoubleMinMax::Rts2ValueDoubleMinMax (std::string in_val_name):Rts2ValueDouble
(in_val_name)
{
	min = nan ("f");
	max = nan ("f");
	rts2Type |= RTS2_VALUE_DOUBLE_MMAX;
}


Rts2ValueDoubleMinMax::Rts2ValueDoubleMinMax (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Rts2ValueDouble (in_val_name, in_description, writeToFits,
flags)
{
	min = nan ("f");
	max = nan ("f");
	rts2Type |= RTS2_VALUE_DOUBLE_MMAX;
}


int
Rts2ValueDoubleMinMax::setValue (Rts2Conn * connection)
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
	if (connection->paramNextDouble (&min)
		|| connection->paramNextDouble (&max) || !connection->paramEnd ())
		return -2;
	if (new_val < min || new_val > max)
		return -2;
	setValueDouble (new_val);
	return 0;
}


int
Rts2ValueDoubleMinMax::doOpValue (char op, Rts2Value * old_value)
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


const char *
Rts2ValueDoubleMinMax::getValue ()
{
	sprintf (buf, "%.20le %.20le %.20le", getValueDouble (), getMin (),
		getMax ());
	return buf;
}


const char *
Rts2ValueDoubleMinMax::getDisplayValue ()
{
	sprintf (buf, "%f %f %f", getValueDouble (), getMin (), getMax ());
	return buf;
}


void
Rts2ValueDoubleMinMax::setFromValue (Rts2Value * newValue)
{
	Rts2ValueDouble::setFromValue (newValue);
	if (newValue->getValueType () == RTS2_VALUE_DOUBLE_MMAX)
	{
		copyMinMax ((Rts2ValueDoubleMinMax *) newValue);
	}
}
