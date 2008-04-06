/* 
 * User management.
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

#ifndef __RTS2_USER__
#define __RTS2_USER__

#include <string>
#include <ostream>
#include <list>

/**
 * Represents type which user have subscribed for receiving events.
 */
class Rts2TypeUser
{
	private:
		char type;
		int eventMask;
	public:
		Rts2TypeUser (char type, int eventMask);
		~Rts2TypeUser (void);

		friend std::ostream & operator << (std::ostream & _os, Rts2TypeUser & usr);
};

std::ostream & operator << (std::ostream & _os, Rts2TypeUser & usr);

class Rts2TypeUserSet: public std::list <Rts2TypeUser>
{
	private:
		int load (int usr_id);
	public:
		/**
		 * Load type-user map for given user ID.
		 */
		Rts2TypeUserSet (int usr_id);
		~Rts2TypeUserSet (void);
};

std::ostream & operator << (std::ostream & _os, Rts2TypeUserSet & usr);

/**
 * Represents user from database.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2User
{
	private:
		int id;
		std::string login;
		std::string email;

		Rts2TypeUserSet *types;

	public:
		/**
		 * Construct user from database entry.
		 *
		 * @param in_id     User ID.
		 * @param in_login  User login.
		 * @param in_email  User email.
		 */
		Rts2User (int in_id, std::string in_login, std::string in_email);
		~Rts2User (void);

		/**
		 * Load types which belongs to given user id.
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		int loadTypes ();

		friend std::ostream & operator << (std::ostream & _os, Rts2User & user);
};

std::ostream & operator << (std::ostream & _os, Rts2User & user);

/**
 * Verify username and password combination.
 *
 * @param username   User login name.
 * @param pass       User password.
 *
 * @return True if login and password is correct, false otherwise.
 */
bool verifyUser (std::string username, std::string pass);
#endif							 /* !__RTS2_USER__ */
