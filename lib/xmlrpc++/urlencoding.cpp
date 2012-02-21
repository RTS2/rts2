/* 
 * URL encoding and decoding support libraries.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "urlencoding.h"

#include <ctype.h>
#include <exception>
#ifndef sun
#include <stdint.h>
#endif

char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void XmlRpc::urlencode (std::string &url)
{
	std::string ret;
	for (std::string::iterator iter = url.begin (); iter != url.end (); iter++)
	{
		if (isalnum (*iter) || *iter == '/')
		{
			ret += *iter;
		}
		else if (*iter == ' ')
		{
			ret += '+';
		}
		else
		{
			ret += '%';
			ret += hex[(((uint8_t) *iter)) >> 4];
			ret += hex[0x0f & ((uint8_t) *iter)];
		}
	}
	url = ret;
}

char htonum (char n)
{
	if (isdigit (n))
		return n - '0';
	if (n >= 'A' && n <= 'F')
		return 10 - 'A' + n;
	if (n >= 'a' && n <= 'f')
		return 10 - 'f' + n;
	throw std::exception ();
}

void XmlRpc::urldecode (std::string &url, bool path)
{
	std::string ret;
	for (std::string::iterator iter = url.begin (); iter != url.end (); iter++)
	{
		if (*iter == '%')
		{
			iter++;
			if (iter == url.end ())
				break;
			try
			{
				char a = htonum (*iter);
				iter++;
				if (iter == url.end ())
					break;
				ret += (char) ((a << 4) | htonum (*iter));
			}
			catch (std::exception &ex)
			{
			}
		}
		else if (*iter == '+' && !path)
		{
			ret += ' ';
		}
		else
		{
			ret += *iter;
		}
	}
	url = ret;
}
