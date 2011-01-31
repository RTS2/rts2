/* 
 * Expanding mechanism.
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

#include "block.h"
#include "expander.h"
#include "rts2config.h"

#include <iomanip>
#include <sstream>

using namespace rts2core;

Expander::Expander ()
{
	epochId = -1;
	setExpandDate ();
}

Expander::Expander (const struct timeval *tv)
{
	epochId = -1;
	setExpandDate (tv);
}

Expander::Expander (Expander * in_expander)
{
	epochId = in_expander->epochId;
	setExpandDate (in_expander->getExpandDate ());
}

Expander::~Expander (void)
{
}

std::string Expander::getEpochString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (3) << epochId;
	return _os.str ();
}

std::string Expander::getYearString (int year)
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (4) << year;
	return _os.str ();
}

std::string Expander::getShortYearString (int year)
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << (year % 100);
	return _os.str ();
}

std::string Expander::getMonthString (int month)
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << month;
	return _os.str ();
}


std::string Expander::getDayString (int day)
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << day;
	return _os.str ();
}


std::string Expander::getYDayString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (3) << getYDay ();
	return _os.str ();
}


std::string Expander::getHourString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getHour ();
	return _os.str ();
}


std::string Expander::getMinString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getMin ();
	return _os.str ();
}


std::string Expander::getSecString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << getSec ();
	return _os.str ();
}


std::string Expander::getMSecString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (3) << ((int) (expandTv.tv_usec / 1000.0));
	return _os.str ();
}


std::string Expander::getNightString ()
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (4) << getNightYear ()
		<< std::setw (2) << getNightMonth ()
		<< std::setw (2) << getNightDay ();
	return _os.str ();
}


std::string Expander::expandVariable (char var)
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
		case 'x':
			ret += getShortYearString ();
			break;
		case 'a':
			ret += getYDayString ();
			break;
		case 'N':
			ret += getNightString ();
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
		case 'Y':
			ret += getYearString (getNightYear ());
			break;
		case 'X':
			ret += getShortYearString (getNightYear ());
			break;
		case 'O':
			ret += getMonthString (getNightMonth ());
			break;
		case 'D':
			ret += getDayString (getNightDay ());
                        break;
		default:
			ret += '%';
			ret += var;
	}
	return ret;
}


std::string Expander::expandVariable (std::string expression)
{
	std::string ret;
	ret = '@';
	ret += expression;
	return ret;
}

void Expander::getFormating (const std::string &expression, std::string::iterator &iter, std::ostringstream &ret)
{
	int length = 0;
	if (*iter == '0')
		ret.fill ('0');
	else
	  	ret.fill (' ');
	// pass numbers and decimal points..
	while (iter != expression.end () && isdigit (*iter))
	{
		length = length * 10 + (*iter - '0');
		iter++;
	}
	if (length > 0)
		ret << std::setw (length);
}

std::string Expander::expand (std::string expression)
{
	std::ostringstream ret;
	int length;
	std::string exp;
	for (std::string::iterator iter = expression.begin (); iter != expression.end (); iter++)
	{
		length = 0;
		switch (*iter)
		{
			case '%':
				iter++;
				getFormating (expression, iter, ret);
				if (iter != expression.end ())
				{
					ret << expandVariable (*iter);
				}
				break;
				// that one enables to copy values from image header to expr
			case '@':
				iter++;
				exp = "";
				getFormating (expression, iter, ret);
				for (; iter != expression.end () && (isalnum (*iter) || (*iter) == '_' || (*iter) == '-' || (*iter) == '.'); iter++)
					exp += *iter;
				iter--;
				ret << expandVariable (exp);
				break;
			default:
				ret << *iter;
		}
	}
	return ret.str ();
}

void Expander::setExpandDate ()
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	setExpandDate (&tv);
}

void Expander::setExpandDate (const struct timeval *tv)
{
	expandTv.tv_sec = tv->tv_sec;
	expandTv.tv_usec = tv->tv_usec;

	localtime_r (&expandTv.tv_sec, &localDate);
	gmtime_r (&expandTv.tv_sec, &utDate);
	
	time_t nightT = Rts2Config::instance ()->getNight (expandTv.tv_sec);
	gmtime_r (&nightT, &nightDate);

	expandDate = &utDate;
}

double Expander::getExpandDateCtime ()
{
	return expandTv.tv_sec + ((double) expandTv.tv_usec / USEC_SEC);
}

const struct timeval * Expander::getExpandDate ()
{
	return &expandTv;
}
