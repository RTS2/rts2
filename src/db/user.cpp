/* 
 * User managment application.
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

#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2userset.h"

/**
 * Application for user management.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2UserApp:public Rts2AppDb
{
	private:
		enum {NOT_SET, LIST_USER, NEW_USER, USER_PASSWORD, USER_EMAIL}
		op;
		const char *user;

		int listUser ();
		int newUser ();
	protected:
		virtual int processOption (int in_opt);
		virtual int doProcessing ();
	public:
		Rts2UserApp (int argc, char **argv);
		virtual ~Rts2UserApp (void);
};

int
Rts2UserApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'l':
		case 'a':
		case 'p':
		case 'e':
			if (op != NOT_SET)
			{
				std::cerr << "Cannot specify two operations together" << std::endl;
				return -1;
			}
			switch (in_opt)
			{
				case 'l':
					op = LIST_USER;
					break;
				case 'a':
					user = optarg;
					op = NEW_USER;
					break;
				case 'p':
					user = optarg;
					op = USER_PASSWORD;
					break;
				case 'e':
					user = optarg;
					op = USER_EMAIL;
					break;
			}
			return 0;
	}
	return Rts2AppDb::processOption (in_opt);
}


int
Rts2UserApp::listUser ()
{
	Rts2UserSet userSet = Rts2UserSet ();
	std::cout << userSet;
	return 0;
}


int
Rts2UserApp::newUser ()
{
	std::string passwd;
	std::string email;

	int ret;
	ret = askForString ("User password", passwd);
	if (ret)
		return ret;
	ret = askForString ("User email (can be left empty)", email);
	if (ret)
		return ret;
	return createUser (std::string (user), passwd, email);
}

int
Rts2UserApp::doProcessing ()
{
	switch (op)
	{
		case NOT_SET:
			help ();
			return -1;
		case LIST_USER:
			return listUser ();
		case NEW_USER:
			return newUser ();
		case USER_PASSWORD:
			break;
		case USER_EMAIL:
			break;
	}
	return -1;
}


Rts2UserApp::Rts2UserApp (int in_argc, char **in_argv): Rts2AppDb (in_argc, in_argv)
{
	op = NOT_SET;
	user = NULL;

	addOption ('l', NULL, 0, "list user stored in the database");
	addOption ('a', NULL, 1, "add new user");
	addOption ('p', NULL, 1, "set user password");
	addOption ('e', NULL, 1, "set user email");
}


Rts2UserApp::~Rts2UserApp (void)
{
}


int
main (int argc, char **argv)
{
	Rts2UserApp app = Rts2UserApp (argc, argv);
	return app.run ();
}
