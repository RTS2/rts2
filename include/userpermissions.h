/* 
 * Class holding user permissions.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_USERPERMISSIONS__
#define __RTS2_USERPERMISSIONS__

#include <string>
#include <vector>

namespace rts2core
{

/**
 * Handles user permissions. Provides method for parsing user permission string,
 * as well as verifying if the user has permissions to change a device or a
 * given variable.
 */
class UserPermissions
{
	public:
		UserPermissions () {}
		
		void parsePermissions (const char *permissionString);

		bool canWriteDevice (const std::string &deviceName);

	private:
		std::vector <std::string> allowedDevices;
};

}

#endif // __RTS2_USERPERMISSIONS__
