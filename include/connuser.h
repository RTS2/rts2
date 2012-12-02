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

#ifndef __RTS2_CONN_USER__
#define __RTS2_CONN_USER__

#include "status.h"

#include <string>

namespace rts2core
{

/**
 * Holder class for users on Connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnUser
{
	public:
		ConnUser (int in_centralId, const char *in_login, const char *in_name);
		virtual ~ ConnUser ();
		int update (int in_centralId, const char *new_login, const char *new_name);

		const char *getName () { return name.c_str (); }
		int getCentraldId () { return centralId; }

	private:
		int centralId;
		std::string login;
		std::string name;
};

}
#endif							 /* !__RTS2_CONN_USER__ */
