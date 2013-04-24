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

#include "configuration.h"
#include "error.h"
#include "userlogins.h"
#include "utilsfunc.h"

#include <unistd.h>
#include <errno.h>
#include <crypt.h>

using namespace rts2core;

UserLogins::UserLogins ()
{
}

void UserLogins::load (const char *filename)
{
	logins.clear ();

	std::ifstream ifs (filename);
	try
	{
		ifs.exceptions (std::ifstream::badbit);
	}
	catch (std::ios_base::failure &f)
	{
		return;
	}
	int ln = 0;
	while (!ifs.eof ())
	{
		std::string line;
		getline (ifs, line);
		if (ifs.eof())
			break;
		if (ifs.fail ())
		{
			std::ostringstream os;
			os << "cannot read from file " << filename << " line " << ln << ": " << strerror (errno);
			throw rts2core::Error (os.str ());
		}
		ln++;
		std::vector <std::string> fields = SplitStr (line, ":");
		if (fields.size () == 2 || fields.size () == 3)
		{
			logins[fields[0]] = std::pair <std::string, std::vector <std::string> > ();
			logins[fields[0]].first = fields[1];
			if (logins.size () == 3)
				logins[fields[0]].second = SplitStr (fields[2], " ");
		}
		{
			std::ostringstream os;
			os << "invalid line in logins " << filename << "file on line " << ln << ", expected two entries separated with :, got " << fields.size () << " entries";
			throw rts2core::Error (os.str ());
		}
	}
}

void UserLogins::save (const char *filename)
{
	std::ofstream ofs (filename);
	ofs.exceptions (std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);

	for (std::map <std::string, std::pair <std::string, std::vector <std::string> > >::iterator iter = logins.begin (); iter != logins.end (); iter++)
	{
		ofs << iter->first << ':' << iter->second.first;
		for (std::vector <std::string>::iterator diter = iter->second.second.begin (); diter != iter->second.second.end(); diter++)
		{
			if (diter == iter->second.second.begin ())
				ofs << ':';
			else
				ofs << ' ';
			ofs << *diter;
		}
	}

	ofs.close ();
}

void UserLogins::listUser (std::ostream &os)
{
	for (std::map <std::string, std::pair <std::string, std::vector <std::string> > >::iterator iter = logins.begin (); iter != logins.end (); iter++)
		os << iter->first << std::endl;
}

bool UserLogins::verifyUser (std::string username, std::string pass, bool &executePermission)
{
	if (logins.find (username) == logins.end ())
		return false;
	// crypt password using salt..
#ifdef RTS2_HAVE_CRYPT
	char *crp = crypt (pass.c_str (), logins[username].first.c_str ());
	return logins[username].first == std::string(crp);
#else
	return logins[username] == pass;
#endif
}

void UserLogins::setUserPassword (std::string username, std::string newpass)
{
#ifdef RTS2_HAVE_CRYPT
	char salt[100];
	strcpy (salt, "$6$");
	random_salt (salt + 3, 8);
	strcpy (salt + 11, "$");
	logins[username].first = std::string (crypt (newpass.c_str (), salt));
#else
	logins[username] = newpass;
#endif
}

void UserLogins::deleteUser (std::string username)
{
	std::map <std::string, std::pair <std::string, std::vector <std::string> > >::iterator iter = logins.find (username);
	if (iter == logins.end ())
		throw rts2core::Error ("cannot find user with name " + username);
	logins.erase (iter);
}
