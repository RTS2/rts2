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
