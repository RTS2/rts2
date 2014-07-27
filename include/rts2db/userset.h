/* 
 * User set.
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

#ifndef __RTS2_USERSET__
#define __RTS2_USERSET__

#include "user.h"
#include <list>
#include <ostream>

namespace rts2db
{

/**
 * Set of User classes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class UserSet: public std::list <User>
{
	private:
		/**
		 * Load user list from database.
		 */
		int load ();

	public:
		/**
		 * Create user set from database.
		 */
		UserSet ();
		~UserSet (void);

		friend std::ostream & operator << (std::ostream & _os, UserSet & userSet)
		{
			for (UserSet::iterator iter = userSet.begin (); iter != userSet.end (); iter++)
				_os << (*iter);
			return _os;
		}
};

}

/**
 * Create new user.
 *
 * @param login     User login.
 * @param password  User password.
 * @param email     User email.
 * @return -1 on error, 0 on sucess.
 */
int createUser (std::string login, std::string password, std::string email, std::string allowed_devices = "");

/**
 * Delete user from database.
 *
 * @param login     username of user to be deleted
 */
int removeUser (std::string login);

#endif							 /* !__RTS2_USERSET__ */
