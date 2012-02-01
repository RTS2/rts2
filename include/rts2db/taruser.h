/* 
 * User access.
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

#ifndef __RTS2_TARUSER__
#define __RTS2_TARUSER__

#include <ostream>
#include <vector>

#define USER_EMAIL_LEN    200

namespace rts2db
{

class Target;

/**
 * Holds informations about target - users relation,
 * which is used to describe which event should be mailed
 * to which user.
 *
 * User can be entered also in type_user table.
 *
 * @author petr
 */
class UserEvent
{
	public:
		// copy constructor
		UserEvent (const UserEvent & in_user);
		UserEvent (const char *in_usr_email, int in_event_mask);
		virtual ~ UserEvent (void);

		bool haveMask (int in_mask)
		{
			return (in_mask & event_mask);
		}
		std::string & getUserEmail ()
		{
			return usr_email;
		}

		friend bool operator == (UserEvent _user1, UserEvent _user2)
		{
			return (_user1.usr_email == _user2.usr_email);
		}

	private:
		std::string usr_email;
		int event_mask;
};

/**
 * Holds set of UserEvent relations.
 *
 * Provides method to use set of UserEvent relations to distribute events.
 */
class TarUser
{
	public:
		TarUser (int in_target, char in_type_id);
		virtual ~TarUser (void);

		int load ();
		// returns users for given event
		std::string getUsers (int in_event_mask, int &count);

	private:
		//! users which belongs to event
		std::vector < UserEvent > users;
		int tar_id;
		char type_id;
};

}

#endif							 /* !__RTS2_TARUSER__ */
