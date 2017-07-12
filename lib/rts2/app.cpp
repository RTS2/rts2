/* 
 * Application sceleton.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

#include "error.h"
#include "app.h"

#include "rts2-config.h"

#include <iostream>
#include <signal.h>
#include <sstream>
#include <errno.h>
#include <libnova/libnova.h>
#include <limits.h>

#include <termios.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

using namespace rts2core;

static App *masterApp = NULL;

#define OPT_VERSION      999
#define OPT_DEBUG        998
#define OPT_UTTIME       997

App *getMasterApp ()
{
	return masterApp;
}

LogStream logStream (messageType_t in_messageType)
{
	LogStream ls (masterApp, in_messageType);
	return ls;
}

App::App (int argc, char **argv):Object ()
{
	app_argc = argc;
	app_argv = argv;

	end_loop = false;

	debug = 0;

	useLocalTime = true;

	tzset ();

	addOption ('h', "help", 0, "write this help");
	addOption (OPT_VERSION, "version", 0, "show program version and license");
	addOption (OPT_DEBUG, "debug", 0, "print debug messages");
	addOption (OPT_UTTIME, "UT", 0, "use UT (not local) time for time displays");

	masterApp = this;
}

App::~App ()
{
}

int App::initOptions ()
{
	int c;
	int ret;

	std::vector < Option >::iterator opt_iter;

	struct option *long_option, *an_option;

	long_option = new struct option[options.size () + 1]; //(struct option *) malloc (sizeof (struct option) * (options.size () + 1));

	char *opt_char = new char[options.size () * 3 + 1];

	char *end_opt = opt_char;

	an_option = long_option;

	for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
	{
		if (opt_iter->haveLongOption ())
		{
			opt_iter->getOptionStruct (an_option);
			an_option++;
		}
		opt_iter->getOptionChar (&end_opt);
	}

	*end_opt = '\0';

	an_option->name = NULL;
	an_option->has_arg = 0;
	an_option->flag = NULL;
	an_option->val = 0;

	/* get attrs */
	while (1)
	{
		c = getopt_long (app_argc, app_argv, opt_char, long_option, NULL);

		if (c == -1)
			break;
		ret = processOption (c);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Error processing option " << c << " " << (char) c << sendLog;
			delete[] long_option;
			return ret;
		}
	}

	delete[] opt_char;

	while (optind < app_argc)
	{
		ret = processArgs (app_argv[optind]);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Error processing arg " << app_argv[optind] << sendLog;
			delete[] long_option;
			return ret;
		}
		optind++;
	}

	if (useLocalTime)
	{
		std::cout << localTime;
		std::cerr << localTime;
	}

	delete[] long_option;

	return 0;
}

void signalHUP (int sig)
{
	if (masterApp)
		masterApp->sigHUP (sig);
}

void killSignal (int sig)
{
	signal (SIGHUP, exit);
	signal (SIGINT, exit);
	signal (SIGTERM, exit);
	if (masterApp)
	{
		if (masterApp->getEndLoop ())
		{
			exit (0);
		}
		masterApp->endRunLoop ();
	}
}

void App::initSignals ()
{
	signal (SIGHUP, signalHUP);
	signal (SIGINT, killSignal);
	signal (SIGTERM, killSignal);
}

int App::init ()
{
	initSignals ();
	try
	{
		return initOptions ();
	}
	catch (rts2core::Error &er)
	{
		std::cerr << er << std::endl;
		std::cerr << std::endl << "Usage:" << std::endl;
		usage ();
		return -1;
	}
}

void App::helpOptions ()
{
	std::vector < Option >::reverse_iterator opt_iter;
	for (opt_iter = options.rbegin (); opt_iter != options.rend (); opt_iter++)
		opt_iter->help ();
}

void App::usage()
{
	std::cout << "\tUsage pattern not defined, please add feature request for missing usage pattern to http://www.rts2.org/featreq" << std::endl;
}

void App::help ()
{
	std::cout << "Usage:" << std::endl;
	usage ();
	std::cout << "Options:" << std::endl;
	helpOptions ();
}

int App::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'h':
		case 0:
			help ();
			exit (EXIT_SUCCESS);
		case OPT_DEBUG:
			debug++;
			break;
		case OPT_UTTIME:
			useLocalTime = false;
			break;
		case OPT_VERSION:
			std::cout << "Part of RTS2 version " << RTS2_VERSION << std::endl
				<< std::endl
				<< "(C) 2001-2017 Petr Kubanek and others, see AUTHORS file for complete list"
				<< std::endl
				<< std::endl
				<< "This program comes with ABSOLUTELY NO WARRANTY; for details see http://www.gnu.org. \
This is free software, and you are welcome to redistribute it under certain conditions; see http://www.gnu.org for details."
				<< std::endl
				<< "See http://rts2.org for news and more."
				<< std::endl
				<< std::endl
				<< "Compiled with" 
#ifdef RTS2_HAVE_PGSQL
				<< " pgsql"
#endif
#ifdef RTS2_HAVE_CRYPT
				<< " crypt"
#endif
#ifdef RTS2_LIBERFA
				<< " erfa/sofa"
#endif
#ifdef RTS2_LIBCHECK
				<< " libcheck"
#endif
#ifdef RTS2_HAVE_LIBJPEG
				<< " libjpeg"
#endif
#ifdef RTS2_HAVE_LIBARCHIVE
				<< " libarchive"
#endif
#ifdef RTS2_HAVE_CXX11
				<< " c++11"
#endif
#ifdef RTS2_LIBUSB1
				<< " libusb1"
#endif
				<< std::endl;

			exit (EXIT_SUCCESS);

		case '?':
			std::cerr << "Invalid option encountered, exiting."
				<< std::endl
				<< "For correct ussage, please consult following:"
				<< std::endl;

			help ();
			exit (EXIT_FAILURE);

		default:
			std::cout << "Unknow option: " << in_opt << "(" << (char) in_opt << ")"
				<< std::endl;

			exit (EXIT_FAILURE);
	}
	return 0;
}

int App::processArgs (const char *arg)
{
	return -1;
}

void App::addOption (int in_short_option, const char *in_long_option, int in_has_arg, const char *in_help_msg)
{
	options.push_back (Option (in_short_option, in_long_option, in_has_arg, in_help_msg));
}

int App::askForInt (const char *desc, int &val)
{
	char temp[200];
	while (!getEndLoop ())
	{
		std::cout << desc << " [";
		if (val == INT_MIN)
			std::cout << " new id ";
		else
			std::cout << val;
		std::cout << "]: ";
		std::cin.getline (temp, 200);
		if (std::cin.eof ())
		{
			setEndLoop ();
			return -1;
		}
		std::string str_val (temp);
		if (str_val.empty ())
			break;
		std::istringstream is (str_val);
		is >> val;
		if (!is.fail ())
			break;
		std::cout << "Invalid number!" << std::endl;
		std::cin.clear ();
		std::cin.ignore (2000, '\n');
	}
	std::cout << desc << ": " << val << std::endl;
	return 0;
}

int App::askForDouble (const char *desc, double &val)
{
	char temp[200];
	while (!getEndLoop ())
	{
		std::cout << desc << " [" << val << "]: ";
		std::cin.getline (temp, 200);
		if (std::cin.eof ())
		{
			setEndLoop ();
			return -1;
		}
		std::string str_val (temp);
		if (str_val.empty ())
			break;
		std::istringstream is (str_val);
		is >> val;
		if (!is.fail ())
			break;
		std::cout << "Invalid number!" << std::endl;
		std::cin.clear ();
		std::cin.ignore (2000, '\n');
	}
	std::cout << desc << ": " << val << std::endl;
	return 0;
}

int App::askForString (const char *desc, std::string & val)
{
	while (!getEndLoop ())
	{
		std::string new_val;
		std::cout << desc << " [" << val << "]: ";
		std::getline (std::cin, new_val);
		if (std::cin.eof ())
		{
			setEndLoop ();
			return -1;
		}
		// use default value
		if (new_val.length () == 0)
			break;
		val = new_val;
		if (!std::cin.fail ())
			break;
		std::cout << "Invalid string!" << std::endl;
		std::cin.clear ();
		std::cin.ignore ();
	}
	std::cout << desc << ": " << val << std::endl;
	return 0;
}

int App::askForPassword (const char *desc, std::string & val)
{
	std::cout << desc << ":";
	struct termios oldt, newt;
	tcgetattr (STDIN_FILENO, &oldt);
	newt = oldt;

	// do not echo
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr (STDIN_FILENO, TCSANOW, &newt);
	std::cin >> val;

	if (std::cin.eof ())
	{
		setEndLoop ();
		return -1;
	}

	tcsetattr (STDIN_FILENO, TCSANOW, &oldt);

	std::cin.ignore ();
	std::cout << std::endl;

	return 0;
}

bool App::askForBoolean (const char *desc, bool val)
{
	char temp[20];
	while (!getEndLoop ())
	{
		std::cout << desc << " (y/n) [" << (val ? "y" : "n") << "]: ";
		std::cin.getline (temp, 20);
		if (std::cin.eof ())
		{
			setEndLoop ();
			return -1;
		}
		// use default value
		if (strlen (temp) == 0)
			break;
		switch (*temp)
		{
			case 'Y':
			case 'y':
				val = true;
				break;
			case 'N':
			case 'n':
				val = false;
				break;
			default:
				std::cout << "Invalid string!" << std::endl;
				std::cin.clear ();
				std::cin.ignore (2000, '\n');
				continue;
		}
		break;
	}
	std::cout << desc << ": " << (val ? "y" : "n") << std::endl;
	return val;
}

int App::askForChr (const char *desc, char &out)
{
	char temp[201];
	std::cout << desc << ":";
	std::cin.getline (temp, 200);
	if (std::cin.eof ())
	{
		setEndLoop ();
		return -1;
	}
	if (std::cin.fail ())
		return -1;
	out = *temp;
	return 0;
}

void App::sendMessage (messageType_t in_messageType, const char *in_messageString)
{
  	if (debug == 0 && in_messageType == MESSAGE_DEBUG)
	  	return;
	Message msg = Message (getAppName (), in_messageType, in_messageString);
	std::cerr << msg << std::endl;
}

void App::sendMessageNoEndl (messageType_t in_messageType, const char *in_messageString)
{
  	if (debug == 0 && in_messageType == MESSAGE_DEBUG)
	  	return;
	Message msg = Message (getAppName (), in_messageType, in_messageString);
	std::cerr << msg;
}

void App::sendMessage (messageType_t in_messageType, std::ostringstream & _os)
{
	sendMessage (in_messageType, _os.str ().c_str ());
}

LogStream App::logStream (messageType_t in_messageType)
{
	LogStream ls (this, in_messageType);
	return ls;
}

void App::sigHUP (int sig)
{
	endRunLoop ();
}
