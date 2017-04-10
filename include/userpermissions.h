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

#include <ostream>
#include <string>
#include <vector>

//* Permission to enable/disable target
#define PERMISSIONS_TARGET_ENABLE      ":target:enabled"

//* Permission to command slew to a target
#define PERMISSIONS_TARGET_SLEW        ":target:slew"

//* Permission to set a target as the next target
#define PERMISSIONS_TARGET_NEXT        ":target:next"

//* Permission to set a target as the current target
#define PERMISSIONS_TARGET_NOW         ":target:now"

//* Permission to edit target script.
#define PERMISSIONS_TARGET_SCRIPTEDIT  ":target:scriptedit"


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

		friend std::ostream & operator << (std::ostream & os, UserPermissions & up)
		{
			if (up.allowedDevices.empty ())
			{
				os << "none";
			}
			else
			{
				for (std::vector<std::string>::iterator iter = up.allowedDevices.begin (); iter != up.allowedDevices.end (); iter++)
				{
					if (iter != up.allowedDevices.begin ())
						os << " ";
					os << *iter;
				}
			}
			return os;
		}

	private:
		std::vector <std::string> allowedDevices;
};

}

#endif // __RTS2_USERPERMISSIONS__
