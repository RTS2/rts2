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
class Rts2UserEvent
{
	private:
		std::string usr_email;
		int event_mask;
	public:
		// copy constructor
		Rts2UserEvent (const Rts2UserEvent & in_user);
		Rts2UserEvent (const char *in_usr_email, int in_event_mask);
		virtual ~ Rts2UserEvent (void);

		bool haveMask (int in_mask)
		{
			return (in_mask & event_mask);
		}
		std::string & getUserEmail ()
		{
			return usr_email;
		}

		friend bool operator == (Rts2UserEvent _user1, Rts2UserEvent _user2);
};

bool operator == (Rts2UserEvent _user1, Rts2UserEvent _user2);

/**
 * Holds set of Rts2UserEvent relations.
 *
 * Provides method to use set of Rts2UserEvent relations to distribute events.
 */
class Rts2TarUser
{
	private:
		//! users which belongs to event
		std::vector < Rts2UserEvent > users;
		int tar_id;
		char type_id;
	public:
		Rts2TarUser (int in_target, char in_type_id);
		virtual ~ Rts2TarUser (void);

		int load ();
		// returns users for given event
		std::string getUsers (int in_event_mask, int &count);
};

#endif							 /* !__RTS2_TARUSER__ */
