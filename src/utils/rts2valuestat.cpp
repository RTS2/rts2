/*
 * Variables which do some basic statistics computations.
 * Copyright (C) 2007 - 2008 Petr Kubanek <petr@kubanek.net>
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

#include <algorithm>

#include "rts2valuestat.h"
#include "rts2conn.h"

void
Rts2ValueDoubleStat::clearStat ()
{
	numMes = 0;
	mode = rts2_nan ("f");
	min = rts2_nan ("f");
	max = rts2_nan ("f");
	stdev = rts2_nan ("f");
	valueList.clear ();
}


void
Rts2ValueDoubleStat::calculate ()
{
	if (valueList.size () == 0)
		return;
	std::deque < double >sorted = valueList;
	std::sort (sorted.begin (), sorted.end ());
	min = *(sorted.begin ());
	max = *(--sorted.end ());
	numMes = valueList.size ();
	double sum = 0;
	for (std::deque < double >::iterator iter = sorted.begin ();
		iter != sorted.end (); iter++)
	{
		sum += (*iter);
	}
	setValueDouble (sum / numMes);
	// calculate stdev
	stdev = 0;
	for (std::deque < double >::iterator iter = sorted.begin ();
		iter != sorted.end (); iter++)
	{
		sum = *iter - getValueDouble ();
		sum *= sum;
		stdev += sum;
	}
	stdev = sqrt (stdev / numMes);
	if ((numMes % 2) == 1)
		mode = sorted[numMes / 2];
	else
		mode = (sorted[numMes / 2 - 1] + sorted[numMes / 2]) / 2.0;
}


Rts2ValueDoubleStat::Rts2ValueDoubleStat (std::string in_val_name):Rts2ValueDouble
(in_val_name)
{
	clearStat ();
	rts2Type |= RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE;
}


Rts2ValueDoubleStat::Rts2ValueDoubleStat (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Rts2ValueDouble (in_val_name, in_description, writeToFits,
flags)
{
	clearStat ();
	rts2Type |= RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE;
}


int
Rts2ValueDoubleStat::setValue (Rts2Conn * connection)
{
	if (connection->paramNextDouble (&value)
		|| connection->paramNextInteger (&numMes)
		|| connection->paramNextDouble (&mode)
		|| connection->paramNextDouble (&min)
		|| connection->paramNextDouble (&max)
		|| connection->paramNextDouble (&stdev) || !connection->paramEnd ())
		return -2;
	return 0;
}


const char *
Rts2ValueDoubleStat::getValue ()
{
	sprintf (buf, "%.20le %i %.20le %.20le %.20le %.20le", value, numMes,
		mode, min, max, stdev);
	return buf;
}


const char *
Rts2ValueDoubleStat::getDisplayValue ()
{
	sprintf (buf, "%f %i %f %f %f %f",
		getValueDouble (), numMes, mode, min, max, stdev);
	return buf;
}


void
Rts2ValueDoubleStat::send (Rts2Conn * connection)
{
	if (numMes != (int) valueList.size ())
		calculate ();
	Rts2ValueDouble::send (connection);
}


void
Rts2ValueDoubleStat::setFromValue (Rts2Value * newValue)
{
	Rts2ValueDouble::setFromValue (newValue);
	if (newValue->getValueType () == (RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE))
	{
		numMes = ((Rts2ValueDoubleStat *) newValue)->getNumMes ();
		mode = ((Rts2ValueDoubleStat *) newValue)->getMode ();
		min = ((Rts2ValueDoubleStat *) newValue)->getMin ();
		max = ((Rts2ValueDoubleStat *) newValue)->getMax ();
		stdev = ((Rts2ValueDoubleStat *) newValue)->getStdev ();
		valueList = ((Rts2ValueDoubleStat *) newValue)->getMesList ();
	}
}
