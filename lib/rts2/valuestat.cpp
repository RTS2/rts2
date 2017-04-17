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

#include "valuestat.h"
#include "connection.h"
#include "libnova_cpp.h"

using namespace rts2core;

void ValueDoubleStat::clearStat ()
{
	numMes = 0;
	mode = NAN;
	min = NAN;
	max = NAN;
	stdev = NAN;
	valueList.clear ();
	changed ();
}

void ValueDoubleStat::calculate ()
{
	if (valueList.size () == 0)
		return;
	std::deque < double >sorted = valueList;
	std::sort (sorted.begin (), sorted.end ());
	min = *(sorted.begin ());
	max = *(--sorted.end ());
	numMes = valueList.size ();
	double sum = 0;
	for (std::deque < double >::iterator iter = sorted.begin (); iter != sorted.end (); iter++)
	{
		sum += (*iter);
	}
	setValueDouble (sum / numMes);
	// calculate stdev
	stdev = 0;
	for (std::deque < double >::iterator iter = sorted.begin (); iter != sorted.end (); iter++)
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
	changed ();
}

ValueDoubleStat::ValueDoubleStat (std::string in_val_name):ValueDouble (in_val_name)
{
	clearStat ();
	rts2Type |= RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE;
}

ValueDoubleStat::ValueDoubleStat (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):ValueDouble (in_val_name, in_description, writeToFits,flags)
{
	clearStat ();
	rts2Type |= RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE;
}

int ValueDoubleStat::setValue (Connection * connection)
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

const char * ValueDoubleStat::getValue ()
{
	sprintf (buf, "%.20le %i %.20le %.20le %.20le %.20le", value, numMes, mode, min, max, stdev);
	return buf;
}

const char * ValueDoubleStat::getDisplayValue ()
{
	std::ostringstream os;
	switch (getValueDisplayType ())
	{
		case RTS2_DT_DEGREES:
			os << LibnovaDeg (getValueDouble ()) << " " << numMes << " " << LibnovaDeg (mode) << " " << LibnovaDeg (min) << " " << LibnovaDeg (max) << " " << LibnovaDeg (stdev);
			memcpy (buf, os.str ().c_str (), sizeof (buf));
			break;
		case RTS2_DT_DEG_DIST:
			os << LibnovaDegDist (getValueDouble ()) << " " << numMes << " " << LibnovaDegDist (mode) << " " << LibnovaDegDist (min) << " " << LibnovaDegDist (max) << " " << LibnovaDegDist (stdev);
			memcpy (buf, os.str ().c_str (), sizeof (buf));
			break;
		default:
			snprintf (buf, sizeof(buf), "%f %i %f %f %f %f", getValueDouble (), numMes, mode, min, max, stdev);
	}
	return buf;
}

void ValueDoubleStat::send (Connection * connection)
{
	if (numMes != (int) valueList.size ())
		calculate ();
	ValueDouble::send (connection);
}

void ValueDoubleStat::setFromValue (Value * newValue)
{
	ValueDouble::setFromValue (newValue);
	if (newValue->getValueType () == (RTS2_VALUE_STAT | RTS2_VALUE_DOUBLE))
	{
		numMes = ((ValueDoubleStat *) newValue)->getNumMes ();
		mode = ((ValueDoubleStat *) newValue)->getMode ();
		min = ((ValueDoubleStat *) newValue)->getMin ();
		max = ((ValueDoubleStat *) newValue)->getMax ();
		stdev = ((ValueDoubleStat *) newValue)->getStdev ();
		valueList = ((ValueDoubleStat *) newValue)->getMesList ();
	}
}

void ValueDoubleTimeserie::clearStat ()
{
	numMes = 0;
	mode = NAN;
	min = NAN;
	max = NAN;
	stdev = NAN;
	alpha = NAN;
	beta = NAN;
	valueList.clear ();
	changed ();
}

void ValueDoubleTimeserie::calculate ()
{
	if (valueList.size () == 0)
		return;
	std::deque < std::pair <double, double> > sorted = valueList;
	std::sort (sorted.begin (), sorted.end ());
	min = sorted.begin ()->first;
	max = (--sorted.end ())->first;
	numMes = valueList.size ();
	double sum = 0;
	double sum2 = 0;
	for (std::deque < std::pair <double, double> >::iterator iter = sorted.begin (); iter != sorted.end (); iter++)
	{
		sum += iter->first;
		sum2 += iter->second;
	}
	setValueDouble (sum / numMes);

	alpha = 0;
	beta = 0;

	sum2 /= numMes;

	double std = 0;
	double alpha2 = 0;
	double sx = 0;

	// calculate stdev, alpha, beta
	stdev = 0;
	for (std::deque < std::pair <double, double> >::iterator iter = sorted.begin (); iter != sorted.end (); iter++)
	{
		std = iter->first - getValueDouble ();
		std *= std;
		stdev += std;

		double x = (iter->second - sum2);
		sx += x;
		alpha += x * iter->first;
		alpha2 += x * x;
	}
	stdev = sqrt (stdev / numMes);

	beta = (alpha - sx * sum / numMes) / (alpha2 - sx * sx / numMes);
	// transform alpha to median value of X
	alpha = (sum - beta * alpha) / numMes;

	if ((numMes % 2) == 1)
		mode = sorted[numMes / 2].first;
	else
		mode = (sorted[numMes / 2 - 1].first + sorted[numMes / 2].first) / 2.0;
	changed ();
}

ValueDoubleTimeserie::ValueDoubleTimeserie (std::string in_val_name):ValueDouble (in_val_name)
{
	clearStat ();
	rts2Type |= RTS2_VALUE_TIMESERIE | RTS2_VALUE_DOUBLE;
}

ValueDoubleTimeserie::ValueDoubleTimeserie (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):ValueDouble (in_val_name, in_description, writeToFits,flags)
{
	clearStat ();
	rts2Type |= RTS2_VALUE_TIMESERIE | RTS2_VALUE_DOUBLE;
}

int ValueDoubleTimeserie::setValue (Connection * connection)
{
	if (connection->paramNextDouble (&value)
		|| connection->paramNextInteger (&numMes)
		|| connection->paramNextDouble (&mode)
		|| connection->paramNextDouble (&min)
		|| connection->paramNextDouble (&max)
		|| connection->paramNextDouble (&stdev)
		|| connection->paramNextDouble (&alpha)
		|| connection->paramNextDouble (&beta)
		|| !connection->paramEnd ())
		return -2;
	return 0;
}

const char * ValueDoubleTimeserie::getValue ()
{
	sprintf (buf, "%.20le %i %.20le %.20le %.20le %.20le %.20le %.20le", value, numMes, mode, min, max, stdev, alpha, beta);
	return buf;
}

const char * ValueDoubleTimeserie::getDisplayValue ()
{
	sprintf (buf, "%f %i %f %f %f %f %f %f", getValueDouble (), numMes, mode, min, max, stdev, alpha, beta);
	return buf;
}

void ValueDoubleTimeserie::send (Connection * connection)
{
	if (numMes != (int) valueList.size ())
		calculate ();
	ValueDouble::send (connection);
}

void ValueDoubleTimeserie::setFromValue (Value * newValue)
{
	ValueDouble::setFromValue (newValue);
	if (newValue->getValueType () == (RTS2_VALUE_TIMESERIE | RTS2_VALUE_DOUBLE))
	{
		numMes = ((ValueDoubleTimeserie *) newValue)->getNumMes ();
		mode = ((ValueDoubleTimeserie *) newValue)->getMode ();
		min = ((ValueDoubleTimeserie *) newValue)->getMin ();
		max = ((ValueDoubleTimeserie *) newValue)->getMax ();
		stdev = ((ValueDoubleTimeserie *) newValue)->getStdev ();
		alpha = ((ValueDoubleTimeserie *) newValue)->getAlpha ();
		beta = ((ValueDoubleTimeserie *) newValue)->getBeta ();
		valueList = ((ValueDoubleTimeserie *) newValue)->getMesList ();
	}
}
