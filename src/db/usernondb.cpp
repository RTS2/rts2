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

#include "rts2/app.h"

#define OPT_PASSWORD   OPT_LOCAL + 321

using namespace rts2db;

/**
 * Application for user management.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class User:public App
{
	public:
		Rts2UserApp (int argc, char **argv);
		virtual ~Rts2UserApp (void);
	protected:
		virtual void usage ();
		virtual int processOption (int in_opt);
		virtual int doProcessing ();
	private:
		enum {NOT_SET, LIST_USER, NEW_USER, DELETE_USER, USER_PASSWORD} op;
		const char *user;
		const char *password;
		//User r2user;

		int listUser ();
		int newUser ();
		int deleteUser ();
		int userPassword ();
		int userEmail ();
		int typesEmail ();

		// menu for flags informations
		rts2core::AskChoice *flagsChoice;
};

Rts2UserApp::Rts2UserApp (int in_argc, char **in_argv): AppDb (in_argc, in_argv)
{
	op = NOT_SET;
	user = NULL;
	password = NULL;

	//r2user = User ();

	// construct menu for flags solution
	flagsChoice = new rts2core::AskChoice (this);
	flagsChoice->addChoice ('1', "send at start of observation");
	flagsChoice->addChoice ('2', "send when first image receives astrometry");
	flagsChoice->addChoice ('3', "send when observation finishes");
	flagsChoice->addChoice ('4', "send when observation is processed");
	flagsChoice->addChoice ('5', "send at the end of night");

	flagsChoice->addChoice ('s', "save");
	flagsChoice->addChoice ('q', "quit");

	addOption ('l', NULL, 0, "list user stored in the database");
	addOption ('a', NULL, 1, "add new user");
	addOption ('d', NULL, 1, "delete user");
	addOption ('p', NULL, 1, "set user password");
	addOption ('e', NULL, 1, "set user email");
	addOption ('m', NULL, 1, "edit user mailing preferences");
	addOption (OPT_PASSWORD, "password", 1, "user password");
}

Rts2UserApp::~Rts2UserApp (void)
{
	delete flagsChoice;
}

void Rts2UserApp::usage ()
{
	std::cout << "\t" << getAppName () << " -l" << std::endl
		<< "\t" << getAppName () << " -a <user_name>" << std::endl
		<< "\t" << getAppName () << " -p <user_name>" << std::endl
		<< "\t" << getAppName () << " -e <user_name>" << std::endl
		<< "\t" << getAppName () << " -m <user_name>" << std::endl;
}

int Rts2UserApp::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'l':
		case 'a':
		case 'd':
		case 'p':
		case 'e':
		case 'm':
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
				case 'd':
					user = optarg;
					op = DELETE_USER;
					break;
				case 'p':
					user = optarg;
					op = USER_PASSWORD;
					break;
			}
			return 0;
		case OPT_PASSWORD:
			password = optarg;
			return 0;
	}
	return AppDb::processOption (in_opt);
}

int Rts2UserApp::listUser ()
{
//	UserSet userSet = UserSet ();
//	std::cout << userSet;
	return 0;
}

int Rts2UserApp::newUser ()
{
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
//	return createUser (std::string (user), passwd, email);
}

int Rts2UserApp::deleteUser ()
{
	int ret;
	bool ch = false;
	std::ostringstream os;
	os << "Delete user " << user;
	ret = askForBoolean (os.str ().c_str (), ch);
	if (ret < 0)
		return ret;
	if (ch == true)
	{
//		ret = removeUser (std::string (user));
		if (ret == 0)
			std::cout << "Deleted user " << user << "." << std::endl;
	}
	else
	{
		ret = 0;
	}
	return ret;
}

int Rts2UserApp::userPassword ()
{
	std::string passwd;

	int ret;

//	ret = r2user.load (user);
//	if (ret)
//		return ret;
	
	ret = askForPassword ("New user password", passwd);
	if (ret)
		return ret;
//	return r2user.setPassword (passwd);
}

int Rts2UserApp::doProcessing ()
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
		case DELETE_USER:
			return deleteUser ();
		case USER_PASSWORD:
			return userPassword ();
	}
	return -1;
}

int main (int argc, char **argv)
{
	Rts2UserApp app = Rts2UserApp (argc, argv);
	return app.run ();
}
