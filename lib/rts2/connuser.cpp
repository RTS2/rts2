/* 
 * Management class for connection users.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include <string.h>

#include "connuser.h"
#include "status.h"

using namespace rts2core;

ConnUser::ConnUser (int in_centralId, const char *in_login)
{
	centralId = in_centralId;
	strncpy (login, in_login, DEVICE_NAME_SIZE);
	login[DEVICE_NAME_SIZE - 1] = '\0';
}


ConnUser::~ConnUser (void)
{
}


int ConnUser::update (int in_centralId, const char *new_login)
{
	if (in_centralId != centralId)
		return -1;
	strncpy (login, new_login, DEVICE_NAME_SIZE);
	login[DEVICE_NAME_SIZE - 1] = '\0';
	return 0;
}
