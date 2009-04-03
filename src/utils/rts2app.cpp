/* 
 * Application sceleton.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "rts2app.h"

#include "config.h"

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

static Rts2App *masterApp = NULL;

#define OPT_VERSION 999

Rts2App *
getMasterApp ()
{
	return masterApp;
}


Rts2LogStream
logStream (messageType_t in_messageType)
{
	Rts2LogStream ls (masterApp, in_messageType);
	return ls;
}


Rts2App::Rts2App (int argc, char **argv):
Rts2Object ()
{
	app_argc = argc;
	app_argv = argv;

	end_loop = false;

	tzset ();

	addOption ('h', "help", 0, "write this help");
	addOption (OPT_VERSION, "version", 0, "show program version and license");

	masterApp = this;
}


Rts2App::~Rts2App ()
{
	std::vector < Rts2Option * >::iterator opt_iter;

	for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
	{
		delete (*opt_iter);
	}
	options.clear ();
}


int
Rts2App::initOptions ()
{
	int c;
	int ret;

	std::vector < Rts2Option * >::iterator opt_iter;

	Rts2Option *opt;

	struct option *long_option, *an_option;

	long_option =
		(struct option *) malloc (sizeof (struct option) * (options.size () + 1));

	char *opt_char = new char[options.size () * 3 + 1];

	char *end_opt = opt_char;

	an_option = long_option;

	for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
	{
		opt = (*opt_iter);
		if (opt->haveLongOption ())
		{
			opt->getOptionStruct (an_option);
			an_option++;
		}
		opt->getOptionChar (&end_opt);
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
			logStream (MESSAGE_ERROR) << "Error processing option " << c << " "
				<< (char) c << sendLog;
			return ret;
		}
	}

	delete[]opt_char;

	while (optind < app_argc)
	{
		ret = processArgs (app_argv[optind]);
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Error processing arg " << app_argv[optind]
				<< sendLog;
			return ret;
		}
		optind++;
	}

	return 0;
}


void
signalHUP (int sig)
{
	masterApp->sigHUP (sig);
}


void
killSignal (int sig)
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


int
Rts2App::init ()
{
	signal (SIGHUP, signalHUP);
	signal (SIGINT, killSignal);
	signal (SIGTERM, killSignal);

	return initOptions ();
}


void
Rts2App::helpOptions ()
{
	std::vector < Rts2Option * >::iterator opt_iter;
	for (opt_iter = options.begin (); opt_iter != options.end (); opt_iter++)
	{
		(*opt_iter)->help ();
	}
}


void
Rts2App::usage()
{
	std::cout << "\tUsage pattern not defined, please add feature request for missing usage pattern to http://www.sf.net/projects/rts-2" << std::endl;
}


void
Rts2App::help ()
{
	std::cout << "Usage:" << std::endl;
	usage ();
	std::cout << "Options:" << std::endl;
	helpOptions ();
}


int
Rts2App::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'h':
		case 0:
			help ();
			exit (EXIT_SUCCESS);
		case OPT_VERSION:
			std::cout << "Part of RTS2 version: " << VERSION << std::endl
				<< std::endl
				<< "(C) 2001-2008 Petr Kubanek and others, see AUTHORS file for complete list"
				<< std::endl
				<< std::endl
				<< "This program comes with ABSOLUTELY NO WARRANTY; for details see http://www.gnu.org. \
This is free software, and you are welcome to redistribute it under certain conditions; see http://www.gnu.org for details."
				<< std::endl
				<< "See http://rts2.org for news and more."
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


int
Rts2App::processArgs (const char *arg)
{
	return -1;
}


int
Rts2App::addOption (int in_short_option, const char *in_long_option, int in_has_arg, const char *in_help_msg)
{
	Rts2Option *an_option = new Rts2Option (in_short_option, in_long_option, in_has_arg, in_help_msg);
	options.push_back (an_option);
	return 0;
}


int
Rts2App::askForInt (const char *desc, int &val)
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


int
Rts2App::askForDouble (const char *desc, double &val)
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


int
Rts2App::askForString (const char *desc, std::string & val)
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


int Rts2App::askForPassword (const char *desc, std::string & val)
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


bool Rts2App::askForBoolean (const char *desc, bool val)
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


int
Rts2App::askForChr (const char *desc, char &out)
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


void
Rts2App::sendMessage (messageType_t in_messageType,
const char *in_messageString)
{
	Rts2Message msg = Rts2Message ("app", in_messageType, in_messageString);
	std::cerr << msg.toString () << std::endl;
}


void
Rts2App::sendMessage (messageType_t in_messageType, std::ostringstream & _os)
{
	sendMessage (in_messageType, _os.str ().c_str ());
}


Rts2LogStream Rts2App::logStream (messageType_t in_messageType)
{
	Rts2LogStream ls (this, in_messageType);
	return ls;
}


int
Rts2App::sendMailTo (const char *subject, const char *text,
const char *in_mailAddress)
{
	int ret;
	char *cmd;
	FILE *mailFile;

	// fork so we will not inhibit calling process..
	ret = fork ();
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Rts2Block::sendMail fork: " <<
			strerror (errno) << sendLog;
		return -1;
	}
	if (ret != 0)
	{
		return 0;
	}
	forkedInstance ();
	std::string cmd_s = std::string ("/usr/bin/mail -s '")
		+ std::string (subject)
		+ std::string ("' '")
		+ std::string (in_mailAddress)
		+ std::string ("'");
	mailFile = popen (cmd_s.c_str (), "w");
	if (!mailFile)
	{
		logStream (MESSAGE_ERROR) << "Rts2Block::sendMail popen: " <<
			strerror (errno) << sendLog;
		exit (0);
	}
	fprintf (mailFile, "%s", text);
	pclose (mailFile);
	free (cmd);
	exit (0);
}


void
Rts2App::sigHUP (int sig)
{
	endRunLoop ();
}
