/* 
 * Expanding mechanism.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#include "rts2block.h"
#include "rts2expander.h"
#include "rts2config.h"

#include <iomanip>
#include <sstream>

Rts2Expander::Rts2Expander ()
{
	epochId = -1;
	setExpandDate ();
}


Rts2Expander::Rts2Expander (const struct timeval *tv)
{
	epochId = -1;
	setExpandDate (tv);
}


Rts2Expander::Rts2Expander (Rts2Expander * in_expander)
{
	epochId = in_expander->epochId;
	setExpandDate (in_expander->getExpandDate ());
}


Rts2Expander::~Rts2Expander (void)
{
}


std::string Rts2Expander::getEpochString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (3) << epochId;
	return _os.str ();
}


std::string Rts2Expander::getYearString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (4) << getYear ();
	return _os.str ();
}


std::string Rts2Expander::getMonthString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getMonth ();
	return _os.str ();
}


std::string Rts2Expander::getDayString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getDay ();
	return _os.str ();
}


std::string Rts2Expander::getYDayString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (3) << getYDay ();
	return _os.str ();
}


std::string Rts2Expander::getHourString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getHour ();
	return _os.str ();
}


std::string Rts2Expander::getMinString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getMin ();
	return _os.str ();
}


std::string Rts2Expander::getSecString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getSec ();
	return _os.str ();
}


std::string Rts2Expander::getMSecString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (3) << ((int) (expandTv.tv_usec / 1000.0));
	return _os.str ();
}


std::string Rts2Expander::expandVariable (char var)
{
	std::string ret = "";
	switch (var)
	{
		case '%':
			return "%";
			break;
		case 'y':
			ret += getYearString ();
			break;
		case 'D':
			ret += getYDayString ();
			break;
		case 'm':
			ret += getMonthString ();
			break;
		case 'd':
			ret += getDayString ();
			break;
		case 'e':
			ret += getEpochString ();
			break;
		case 'H':
			ret += getHourString ();
			break;
		case 'M':
			ret += getMinString ();
			break;
		case 'S':
			ret += getSecString ();
			break;
		case 's':
			ret += getMSecString ();
			break;
		case 'U':
			expandDate = &utDate;
			break;
		case 'L':
			expandDate = &localDate;
			break;
		case 'Z':
			ret += tzname[(expandDate->tm_isdst > 0) ? 1 : 0];
			break;
		default:
			ret += '%';
			ret += var;
	}
	return ret;
}


std::string Rts2Expander::expandVariable (std::string expression)
{
	std::string ret;
	ret = '@';
	ret += expression;
	return ret;
}


std::string Rts2Expander::expand (std::string expression)
{
	std::string ret = "";
	std::string exp;
	for (std::string::iterator iter = expression.begin (); iter != expression.end (); iter++)
	{
		switch (*iter)
		{
			case '%':
				iter++;
				if (iter != expression.end ())
				{
					ret += expandVariable (*iter);
				}
				break;
				// that one enables to copy values from image header to expr
			case '@':
				exp = "";
				for (iter++; iter != expression.end () && (isalnum (*iter) || (*iter) == '_' || (*iter) == '-'); iter++)
					exp += *iter;
				iter--;
				ret += expandVariable (exp);
				break;
			default:
				ret += *iter;
		}
	}
	return ret;
}


void
Rts2Expander::setExpandDate ()
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	setExpandDate (&tv);
}


void
Rts2Expander::setExpandDate (const struct timeval *tv)
{
	expandTv.tv_sec = tv->tv_sec;
	expandTv.tv_usec = tv->tv_usec;

	localtime_r (&expandTv.tv_sec, &localDate);
	gmtime_r (&expandTv.tv_sec, &utDate);

	expandDate = &utDate;
}


double
Rts2Expander::getExpandDateCtime ()
{
	return expandTv.tv_sec + ((double) expandTv.tv_usec / USEC_SEC);
}


const struct timeval *
Rts2Expander::getExpandDate ()
{
	return &expandTv;
}
