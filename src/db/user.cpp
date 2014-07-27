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

#include "rts2db/appdb.h"
#include "rts2db/userset.h"

#include "askchoice.h"
#include "rts2target.h"

#define OPT_PASSWORD   OPT_LOCAL + 321

using namespace rts2db;

/**
 * Application for user management.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2UserApp:public AppDb
{
	public:
		Rts2UserApp (int argc, char **argv);
		virtual ~Rts2UserApp (void);
	protected:
		virtual void usage ();
		virtual int processOption (int in_opt);
		virtual int doProcessing ();
	private:
		enum {NOT_SET, LIST_USER, NEW_USER, DELETE_USER, USER_PASSWORD, USER_EMAIL, TYPES_EMAIL}
		op;
		const char *user;
		const char *password;
		User r2user;

		int listUser ();
		int newUser ();
		int deleteUser ();
		int userPassword ();
		int userEmail ();
		int typesEmail ();

		// menu for flags informations
		rts2core::AskChoice *flagsChoice;

		// type actions
		int addNewType ();
		int removeType ();
		int editType ();
};

Rts2UserApp::Rts2UserApp (int in_argc, char **in_argv): AppDb (in_argc, in_argv)
{
	op = NOT_SET;
	user = NULL;
	password = NULL;

	r2user = User ();

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
				case 'e':
					user = optarg;
					op = USER_EMAIL;
					break;
				case 'm':
					user = optarg;
					op = TYPES_EMAIL;
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
	UserSet userSet = UserSet ();
	std::cout << userSet;
	return 0;
}

int Rts2UserApp::newUser ()
{
	std::string passwd;
	std::string email;
	std::string allowed_devices;

	if (password)
	{
		passwd = std::string (password);
		email = std::string ("");
	}
	else
	{
		int ret = askForPassword ("User password", passwd);
		if (ret)
			return ret;
		ret = askForString ("User email (can be left empty)", email);
		if (ret)
			return ret;
		ret = askForString ("Allowed Devices (can be left empty)", allowed_devices);
		if (ret)
			return ret;
	}
	return createUser (std::string (user), passwd, email, allowed_devices);
}

int Rts2UserApp::deleteUser ()
{
	int ret;
	bool ch = false;
	std::ostringstream os;
	os << "Delete user " << user;
	ret = askForBoolean (os.str ().c_str (), ch);
	if (ret == true)
	{
		ret = removeUser (std::string (user));
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

	ret = r2user.load (user);
	if (ret)
		return ret;
	
	ret = askForPassword ("New user password", passwd);
	if (ret)
		return ret;
	return r2user.setPassword (passwd);
}

int Rts2UserApp::userEmail ()
{
	std::string email;

	int ret;

	ret = r2user.load (user);
	if (ret)
		return ret;

	ret = askForString ("New user email", email);
	if (ret)
		return ret;
	
	return r2user.setEmail (email);
}

int Rts2UserApp::typesEmail ()
{
	rts2core::AskChoice typeChoice = rts2core::AskChoice (this);
	typeChoice.addChoice ('a', "Add triggers for new type");
	typeChoice.addChoice ('r', "Remove triggers for a target type");
	typeChoice.addChoice ('e', "Edit triggers for a target type");
	typeChoice.addChoice ('q', "Quit");

	int ret;

	ret = r2user.load (user);
	if (ret)
		return ret;

	while (!getEndLoop ())
	{
		// display user..
		std::cout << r2user;

		char rkey = typeChoice.query (std::cout);
		switch (rkey)
		{
			case 'a':
				addNewType ();
				break;
			case 'r':
				removeType ();
				break;
			case 'e':
				editType ();
				break;
			case 'q':
				return 0;
		}
	}
	return -1;
}

int Rts2UserApp::addNewType ()
{
	char type;
	if (askForChr ("Please enter a type", type))
		return -1;
	
	// selected flags
	int flags = 0x05;

	while (!getEndLoop ())
	{
	  	std::cout << "Current mask: ";
		printEventMask (flags, std::cout);
		std::cout << std::endl;

		switch (flagsChoice->query (std::cout))
		{
			case '1':
				flags ^= 0x01;
				break;
			case '2':
				flags ^= 0x02;
				break;
			case '3':
				flags ^= 0x04;
				break;
			case '4':
				flags ^= 0x08;
				break;
			case '5':
				flags ^= 0x10;
				break;
			case 'q':
				return 0;
			case 's':
				return r2user.addNewTypeFlags (type, flags);
		}
	}
	return -1;
}

int Rts2UserApp::removeType ()
{
	int ret;

	ret = r2user.load (user);
	if (ret)
		return ret;

	std::cout << r2user;

	// let's user decide which type he would likt to remove
	char type;
	if (askForChr ("Enter type which you would like to remove", type))
		return -1;
	
	return r2user.removeType (type);
}

int Rts2UserApp::editType ()
{
	int ret;

	ret = r2user.load (user);
	if (ret)
		return ret;

	std::cout << r2user;

	// let's user decide which type he would likt to remove
	char type;
	if (askForChr ("Enter type which you would like to edit", type))
		return -1;

	TypeUser *typeUser;
	
	// selected flags
	typeUser = r2user.getFlags (type);

	if (typeUser == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot find entry for type " << type << sendLog;
		return -1;
	}

	int eventMask = typeUser->getEventMask ();

	while (!getEndLoop ())
	{
	  	std::cout << "Current mask: ";
		printEventMask (eventMask, std::cout);
		std::cout << std::endl;

		switch (flagsChoice->query (std::cout))
		{
			case '1':
				eventMask ^= 0x01;
				break;
			case '2':
				eventMask ^= 0x02;
				break;
			case '3':
				eventMask ^= 0x04;
				break;
			case '4':
				eventMask ^= 0x08;
				break;
			case '5':
				eventMask ^= 0x10;
				break;
			case 'q':
				return 0;
			case 's':
				return typeUser->updateFlags (r2user.getUserId (), eventMask);
		}
	}

	return -1;
}

int Rts2UserApp::doProcessing ()
{
	srandom (time (NULL));
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
		case USER_EMAIL:
			return userEmail ();
		case TYPES_EMAIL:
			return typesEmail ();
	}
	return -1;
}

int main (int argc, char **argv)
{
	Rts2UserApp app = Rts2UserApp (argc, argv);
	return app.run ();
}
