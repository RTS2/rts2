/* 
 * Expanding mechanism.
 * Copyright (C) 2007-2012 Petr Kubanek <petr@kubanek.net>
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
#include "configuration.h"
#include "utilsfunc.h"

#include <iomanip>
#include <sstream>

using namespace rts2core;

Expander::Expander ()
{
	epochId = -1;
	num_pos = -1;
	setExpandDate ();
}

Expander::Expander (const struct timeval *tv)
{
	epochId = -1;
	num_pos = -1;
	setExpandDate (tv);
}

Expander::Expander (Expander * in_expander)
{
	epochId = in_expander->epochId;
	num_pos = in_expander->num_pos;
	num_pos_end = in_expander->num_pos_end;
	num_lenght = in_expander->num_lenght;
	num_fill = in_expander->num_fill;
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

std::string Expander::expandVariable (char var, size_t beg, bool &replaceNonAlpha)
{
	std::string ret = "";
	std::ostringstream os;
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
		case 'A':
			return getDateObs (getCtimeSec (), getCtimeUsec ());
		case 'a':
			ret += getYDayString ();
			break;
		case 'C':
			os << getCtimeSec ();
			ret += os.str ();
			break;
		case 'J':
			{
				time_t tim = getCtimeSec ();
				os << std::fixed << std::setprecision (8) << (ln_get_julian_from_timet (&tim) + getCtimeUsec () / USEC_SEC / 86400.0);
				ret += os.str ();
			}
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
			useUtDate ();
			break;
		case 'L':
			useLocalDate ();
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
		case 'u':
			if (num_pos >= 0)
				throw rts2core::Error ("you cannot use multiple n strings");
			num_pos = beg;
			num_lenght = length;
			num_fill = fill;
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
	length = 0;
	if (*iter == '0')
		fill = '0';
	else
		fill = ' ';
	ret.fill (fill);
	// pass numbers and decimal points..
	while (iter != expression.end () && isdigit (*iter))
	{
		length = length * 10 + (*iter - '0');
		iter++;
	}
	if (length > 0)
		ret << std::setw (length);
}

// helper functions to get rid of non-alpha characters
inline bool isAlphaNum (char ch)
{
	return isalnum (ch) || isspace (ch) || strchr ("_+-.|(){}:='\",`|[]", ch);
}

std::string replaceNonAlpha (std::string in, bool onlyAlphaNum)
{
	if (!onlyAlphaNum)
		return in;
	std::string ret;
	for (std::string::iterator iter = in.begin (); iter != in.end (); iter++)
	{
		if (isAlphaNum (*iter))
			ret += *iter;
		else
			ret += '_';
	}
	return ret;
}

std::string Expander::expand (std::string expression, bool onlyAlphaNum)
{
	num_pos = -1;

	std::ostringstream ret;
	std::string exp;

	// iterates through string to expand
	for (std::string::iterator iter = expression.begin (); iter != expression.end (); iter++)
	{
		switch (*iter)
		{
			case '%':
				iter++;
				getFormating (expression, iter, ret);

				// don't expand last %
				if (iter != expression.end ())
				{
					bool rep = onlyAlphaNum;
					std::string ex = expandVariable (*iter, ret.str ().length (), rep);
					if (ex.length () > 0)
						ret << replaceNonAlpha (ex, rep);
					else
						ret << std::setw (0);
				}
				break;
			// that one copy values from image header to expression
			case '@':
				iter++;
				exp = "";
				getFormating (expression, iter, ret);
				for (; iter != expression.end () && (isalnum (*iter) || (*iter) == '_' || (*iter) == '-' || (*iter) == '.' || (*iter == ':')); iter++)
					exp += *iter;
				iter--;
				ret << replaceNonAlpha (expandVariable (exp), onlyAlphaNum);
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

void Expander::setExpandDate (const struct timeval *tv, bool localdate)
{
	expandTv.tv_sec = tv->tv_sec;
	expandTv.tv_usec = tv->tv_usec;

	localtime_r (&expandTv.tv_sec, &localDate);
	gmtime_r (&expandTv.tv_sec, &utDate);
	
	time_t nightT = Configuration::instance ()->getNight (expandTv.tv_sec);
	gmtime_r (&nightT, &nightDate);

	if (localdate)
		expandDate = &localDate;
	else
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
