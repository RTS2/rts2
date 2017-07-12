/* 
 * User managment application.
 * Copyright (C) 2008,2010 Petr Kubanek <petr@kubanek.net>
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

#include "app.h"
#include "configuration.h"
#include "userlogins.h"
#include "error.h"

#include <iostream>

#define OPT_PASSWORD   OPT_LOCAL + 321
#define OPT_USER_FILE  OPT_LOCAL + 322

/**
 * Application for user management.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class UserApp:public rts2core::App
{
	public:
		UserApp (int argc, char **argv);
		virtual ~UserApp (void);

		virtual int run ();
	protected:
		virtual void usage ();
		virtual int processOption (int in_opt);
	private:
		enum {NOT_SET, LIST_USER, NEW_USER, DELETE_USER, USER_PASSWORD, USER_VERIFY} op;
		const char *user;
		const char *password;

		char *userfile;

		rts2core::UserLogins logins;

		int listUser ();
		int newUser ();
		int deleteUser ();
		int userPassword ();
		int verifyPassword();
};

UserApp::UserApp (int in_argc, char **in_argv): rts2core::App (in_argc, in_argv)
{
	op = NOT_SET;
	user = NULL;
	password = NULL;

	userfile = NULL;

	addOption ('l', NULL, 0, "list user stored in user file");
	addOption ('a', NULL, 1, "add new user");
	addOption ('d', NULL, 1, "delete user");
	addOption ('p', NULL, 1, "set user password");
	addOption ('v', NULL, 1, "verify user password");
	addOption (OPT_PASSWORD, "password", 1, "user password");
	addOption (OPT_USER_FILE, "userfile", 1, "user file");
}

UserApp::~UserApp (void)
{
}

void UserApp::usage ()
{
	std::cout << "\t" << getAppName () << " -l" << std::endl
		<< "\t" << getAppName () << " -a <user_name>" << std::endl
		<< "\t" << getAppName () << " -p <user_name>" << std::endl;
}

int UserApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'l':
		case 'a':
		case 'd':
		case 'p':
		case 'v':
			if (op != NOT_SET)
			{
				std::cerr << "multiple operations are not allowed" << std::endl;
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
				case 'd':
					user = optarg;
					op = DELETE_USER;
					break;
				case 'p':
					user = optarg;
					op = USER_PASSWORD;
					break;
				case 'v':
					user = optarg;
					op = USER_VERIFY;
					break;
			}
			return 0;
		case OPT_PASSWORD:
			password = optarg;
			return 0;
		case OPT_USER_FILE:
			userfile = optarg;
			return 0;
	}
	return rts2core::App::processOption (in_opt);
}

int UserApp::listUser ()
{
	logins.load (userfile);
	logins.listUser (std::cout);
	return 0;
}

int UserApp::newUser ()
{
	try
	{
		logins.load (userfile);
	}
	catch (rts2core::Error &er)
	{
	}

	std::string passwd;

	if (password)
	{
		passwd = std::string (password);
	}
	else
	{
		int ret = askForPassword ("User password", passwd);
		if (ret)
			return ret;
	}
	
	logins.setUserPassword (std::string (user), passwd);
	logins.save (userfile);
	return 0;
}

int UserApp::deleteUser ()
{
	int ret;
	bool ch = false;
	std::ostringstream os;
	os << "Delete user " << user;
	ch = askForBoolean (os.str ().c_str (), ch);
	if (ch == true)
	{
		logins.load (userfile);
		logins.deleteUser (user);
		logins.save (userfile);
	}
	else
	{
		ret = 0;
	}
	return ret;
}

int UserApp::userPassword ()
{
	logins.load (userfile);

	std::string passwd;

	if (password)
	{
		passwd = std::string(password);
	}
	else
	{
		int ret = askForPassword ("New user password", passwd);
		if (ret)
			return ret;
	}

	logins.setUserPassword (std::string (user), passwd);
	logins.save (userfile);
	return 0;
}

int UserApp::verifyPassword()
{
	logins.load(userfile);

	std::string passwd;

	if (password)
	{
		passwd = std::string(password);
	}
	else
	{
		int ret = askForPassword("New user password", passwd);
		if (ret)
			return ret;
	}

	bool res = logins.verifyUser(user, passwd);
	if (res)
		std::cout << "password verified" << std::endl;
	else
		std::cout << "wrong username or password" << std::endl;
	return 0;
}

int UserApp::run ()
{
	int ret;
	ret = init ();
	if (ret)
		return ret;
	
	if (userfile == NULL)
	{
		std::string lf;
		rts2core::Configuration::instance ()->getString ("observatory", "logins", lf, RTS2_CONFDIR "/rts2/logins");
		userfile = new char[lf.length() + 1];
		strcpy (userfile, lf.c_str ());
	}

	switch (op)
	{
		case NOT_SET:
			help ();
			return -1;
		case LIST_USER:
			return listUser ();
		case NEW_USER:
			return newUser ();
		case DELETE_USER:
			return deleteUser ();
		case USER_PASSWORD:
			return userPassword ();
		case USER_VERIFY:
			return verifyPassword();
	}
	return -1;
}

int main (int argc, char **argv)
{
	UserApp app = UserApp (argc, argv);
	return app.run ();
}
