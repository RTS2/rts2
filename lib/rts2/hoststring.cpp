/* 
 * Hoststring utility class.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "hoststring.h"

#include <stdlib.h>
#include <string.h>

HostString::HostString (const char *hoststring, const char *defaultPort)
{
	const char *ch = strchr (hoststring, ':');
	if (ch != NULL)
	{
		hostname = std::string (hoststring).substr (0, ch - hoststring);
		ch++;
		port = atoi (ch);
	}
	else
	{
		hostname = std::string (hoststring);
		port = atoi (defaultPort);
	}
}
