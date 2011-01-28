/* 
 * Various utility functions.
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

#include "utilsfunc.h"

#include <errno.h>
#include <malloc.h>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

double random_num ()
{
#ifndef sun
	return (double) random () / RAND_MAX;
#else
	return (double) random () / INT_MAX;
#endif
}

int mkpath (const char *path, mode_t mode)
{
	char *cp_path;
	char *start, *end;
	int ret = 0;
	cp_path = strdup (path);
	start = cp_path;
	while (1)
	{
		end = ++start;
		while (*end && *end != '/')
			end++;
		if (!*end)
			break;
		*end = '\0';
		ret = mkdir (cp_path, mode);
		*end = '/';
		start = ++end;
		if (ret)
		{
			if (errno != EEXIST)
				break;
			ret = 0;
		}
	}
	free (cp_path);
	return ret;
}

int parseDate (const char *in_date, struct ln_date *out_time, bool &islocal)
{
	int ret;
	int ret2;
	out_time->hours = out_time->minutes = 0;
	out_time->seconds = 0;
	while (*in_date && isblank (*in_date))
		in_date++;
	islocal = false;	

	if (in_date[0] == '+' || in_date[0] == '-')
	{
		time_t now;
		time (&now);
		now += atoi (in_date);
		ln_get_date_from_timet (&now, out_time);
		return 0;
	}

	ret = sscanf (in_date, "%d-%d-%d%n", &out_time->years, &out_time->months, &out_time->days, &ret2);
	if (ret == 3)
	{
		in_date += ret2;

		// we end with is U for UT, let's check if it contains time..
		if (*in_date == 'T' || *in_date == 'L' || *in_date == 'G' || *in_date == 'U' || isspace (*in_date))
		{
			// localtime
			if (*in_date == 'T' || *in_date == 'L' || isspace (*in_date))
				islocal = true;
			in_date++;
			ret2 = 	sscanf (in_date, "%u:%u:%lf", &out_time->hours,	&out_time->minutes, &out_time->seconds);
			if (ret2 == 3)
				return 0;
			ret2 = 	sscanf (in_date, "%u:%u", &out_time->hours, &out_time->minutes);
			if (ret2 == 2)
				return 0;
			ret2 = sscanf (in_date, "%d", &out_time->hours);
			if (ret2 == 1)
				return 0;
			return -1;
		}
		// only year..
		if (*in_date == '\0')
		{
			islocal = true;  
			return 0;
		}
	}
	return -1;
}

int parseDate (const char *in_date, struct ln_date *out_time)
{
	bool islocal;
	int ret = parseDate (in_date, out_time, islocal);
	if (islocal)
	{
		double JD = ln_get_julian_day (out_time);
		JD += timezone / 86400.0;
		ln_get_date (JD, out_time);
		if (out_time->seconds < 0.001)
			out_time->seconds = 0;
	}
	return ret;
}


int parseDate (const char *in_date, double &JD)
{
	struct ln_date l_date;
	int ret;
	ret = parseDate (in_date, &l_date);
	if (ret)
		return ret;
	JD = ln_get_julian_day (&l_date);
	return 0;
}

int parseDate (const char *in_date, time_t *out_time)
{
	int ret;
	struct ln_date l_date;
	ret = parseDate (in_date, &l_date);
	if (ret)
		return ret;
	ln_get_timet_from_julian (ln_get_julian_day (&l_date), out_time);
	return 0;
}

std::vector<std::string> SplitStr(const std::string& text, const std::string& delimeter)
{
	std::size_t pos = 0;
	std::size_t oldpos = 0;
	std::size_t delimlen = delimeter.length();

	std::vector<std::string> result;
	
	if (text.empty ())
		return result;
	
	while (pos != std::string::npos)
	{
		pos = text.find(delimeter, oldpos);
		if (pos - oldpos == 0)
		{
			// / is the only character..
			if (text.length () == 1)
				break;
			oldpos += delimlen;
			continue;
		}
		result.push_back(text.substr(oldpos, pos - oldpos));
		oldpos = pos + delimlen;
		// last character..
		if (pos == text.length () - 1)
			break;
	}

	return result;
}

std::vector<char> Str2CharVector (std::string text)
{
	std::vector<char> res;
	for (std::string::iterator iter = text.begin(); iter != text.end(); iter++)
	{
		res.push_back (*iter);	
	}
	return res;
}

std::vector <int> parseRange (const char *range_str, int array_size, const char *& endp)
{
	int from = 0;
	int to = 0;
	enum {FROM, TO, ARRAY_LEFT} state = FROM;
	std::vector <int> ret;
	endp = range_str;
	while (*endp)
	{
		if (isdigit (*endp))
		{
			switch (state)
			{
				case ARRAY_LEFT:
					state = FROM;
				case FROM:
					from = from * 10 + (*endp) - '0';
					break;
				case TO:
					to = to * 10 + (*endp) - '0';
					break;
			}
		}
		else if (*endp == ':')
		{
			switch (state)
			{
				case FROM:
				case ARRAY_LEFT:
					state = TO;
					break;
				case TO:
					return ret;
			}
		}
		else if (*endp == ',')
		{
			if (from > 0)
				from--;
			if (to == 0 || to > array_size)
			{
				if (state == TO)
					to = array_size;
				else
					to = to > array_size ? from : from + 1;
			}
			for ( ; from < to; from++)
				ret.push_back (from);
			from = to = 0;
			state = ARRAY_LEFT;
		}
		else
		{
			return ret;
		}
		endp++;
	}
	if (endp == range_str)
	{
		for ( ; from < array_size; from++)
			ret.push_back (from);
	}
	else if (state == TO || from == 0 || (state == FROM && from > 0))
	{
		if (from > 0)
			from--;
		if (to == 0 || to > array_size)
		{
			if (state == TO)
				to = array_size;
			else
				to = to > array_size ? from : from + 1;
		}
		for ( ; from < to; from++)
			ret.push_back (from);
	}
	return ret;
}

int charToBool (const char *in_value, bool &ret)
{
	if (!strcasecmp (in_value, "ON") || !strcasecmp (in_value, "TRUE")
		|| !strcasecmp (in_value, "YES") || !strcmp (in_value, "1"))
		ret = true;
	else if (!strcasecmp (in_value, "OFF") || !strcasecmp (in_value, "FALSE")
		|| !strcasecmp (in_value, "NO") || !strcmp (in_value, "0"))
		ret = false;
	else
		return -1;
	return 0;
}

#ifndef HAVE_ISINF
int isinf(double x)
{
	return !finite(x) && x==x;
}
#endif

#ifndef isfinite
int isfinite(double x)
{
	return finite(x) && !isnan(x);
}
#endif

#ifndef HAVE_STRCASESTR
char * strcasestr(const char * haystack, const char * needle)
{
	const char *p = haystack;
	int len = strlen (needle);
	while (*p)
	{
		if (strncasecmp (haystack, needle, len))
			return (char *) p;
		p++;
	}
	return NULL;
}
#endif // HAVE_STRCASESTR
