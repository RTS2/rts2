/* 
 * Logins for non-DB users.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#include <map>
#include <string>
#include <vector>

namespace rts2core
{

class UserLogins
{
	public:
		UserLogins ();

		/**
		 * Load user configuration from file.
		 *
		 * @param filename file for logins
		 */
		void load (const char *filename);

		void save (const char *filename);

		void listUser (std::ostream &os);

		bool verifyUser (std::string username, std::string pass, bool &executePermission);

		void setUserPassword (std::string username, std::string newpass);

		void deleteUser (std::string username);
	
	private:
		// pair is holding password and allowed devices
		std::map <std::string, std::pair <std::string, std::vector <std::string> > > logins;
};

}
