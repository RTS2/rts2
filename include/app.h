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

#ifndef __RTS2_APP__
#define __RTS2_APP__

#include <vector>
#include <time.h>
#include <stdlib.h>

#include "object.h"
#include "option.h"
#include "message.h"
#include "logstream.h"

namespace rts2core
{

/**
 * Abstract class which provides functions for an application.
 * 
 * This class encapsulates method to process options, main arguments and
 * contains run method.
 *
 * Ussual implementation of a main routine of an application using App
 * descendat looks like:
 *
 @code
 int
 main (int argc, char **argv)
 {
	MyAppClass app = MyAppClass (argc, argv);
	return app.run ();
 }
 @endcode
 *
 * This class is used as base class for all RTS2 components which are executables.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class App:public Object
{
	public:
		/**
		 * Constructor for App.
		 *
		 * @param argc Number of arguments passed in argv.
		 * @param argv Application options and arguments.
		 */
		App (int argc, char **argv);
		virtual ~ App ();

		/**
		 * Run method of the application.
		 *
		 * Application is responsible for calling init () method to read
		 * variables etc..
		 *
		 * Both Daemon and Rts2CliApp define run method, so this method is
		 * pure virtual. It makes sense for all descendats of App
		 * to provide this method.
		 */
		virtual int run () = 0;

		/**
		 * If running in loop, caused end of loop.
		 */
		virtual void endRunLoop ()
		{
			exit (0);
		}

		/**
		 * Return end loop flag.
		 *
		 * @return End loop flag. When true, application should silently exit.
		 */
		bool getEndLoop ()
		{
			return end_loop;
		}

		/**
		 * Set end loop flag.
		 *
		 * @param in_end_loop True if run loop should be stopped.
		 */
		void setEndLoop (bool in_end_loop = true)
		{
			end_loop = in_end_loop;
		}

		/**
		 * Ask user for char, used to ask for chair in choice question.
		 *
		 * @param desc Description which will be displayed before user is queried for string.
		 * @param out  Char typed by user.
		 * @return 0 on sucess, -1 on error.
		 */
		int askForChr (const char *desc, char &out);

		/**
		 * Ask user for integer.
		 *
		 * @param desc Description.
		 * @param val  Value typed by user. Also it is default value, used when user only hits enter.
		 * @return -1 on error, 0 on success.
		 */
		int askForInt (const char *desc, int &val);

		/**
		 * Ask user for double number.
		 *
		 * @param desc Description.
		 * @param val  Value typed by user. Also it is default value, used when user only hits enter.
		 * @return -1 on error, 0 on success.
		 */
		int askForDouble (const char *desc, double &val);

		/**
		 * Ask user for string.
		 *
		 * @param desc Description.
		 * @param val  Value typed by user. Also it is default value, used when user only hits enter.
		 * @return -1 on error, 0 on success.
		 */
		int askForString (const char *desc, std::string & val);

		/**
		 * Ask user for password string.
		 * Allow user to type password on console, without displaying character
		 * he/she enters.
		 *
		 * @param desc Description.
		 * @param val  Password value.
		 * @return -1 on error, 0 on success.
		 */
		int askForPassword (const char *desc, std::string & val);

		/**
		 * Ask user for boolean value. Various entries (YyNn01) are allowed.
		 *
		 * @param desc     Description.
		 * @param val      Default value.
		 * @return value selected by user
		 */
		bool askForBoolean (const char *desc, bool val);

		/**
		 * Function used in messaging API to pass message to
		 * application. Client applications then print the message to
		 * stderr, daemons ussually pass it through TCP/IP connection
		 * to master daemon for logging.
		 *
		 * @param in_messageType   Message type.
		 * @param in_messageString Message.
		 */
		virtual void sendMessage (messageType_t in_messageType, const char *in_messageString);


		/**
		 * Sends message without sending ending endl.
		 *
		 * @see sendMessage()
		 */
		virtual void sendMessageNoEndl (messageType_t in_messageType, const char *in_messageString);

		/**
		 * Sends message.
		 *
		 * @param in_messageType   Message type.
		 * @param _os              std::ostringstream holding the message.
		 */
		inline void sendMessage (messageType_t in_messageType, std::ostringstream & _os);

		virtual LogStream logStream (messageType_t in_messageType);

		/**
		 * Called on SIGHUP signal.
		 * This method is called from static signal routine.
		 *
		 * @warning { Do not call any filedescriptor operations from this routine.
		 * The recomended way how to handle this routine is to set flag that the application
		 * received SIGHUP, and call routines which shall respond to SIGHUP call from the idle() 
		 * method.
		 */
		virtual void sigHUP (int sig);

		bool usesLocalTime () { return useLocalTime; }

		/**
		 * Return set debug level.
		 *
		 * @return debug level
		 */
		int getDebug () { return debug; }

	protected:
		/**
		 * Called to process options of the programme.
		 *
		 * @param in_opt Option as extracted by getopt call, first parameter of the addOption call.
		 * @return -1 on failure to parse option, 0 on success.
		 */
		virtual int processOption (int in_opt);
		
		/**
		 * Process arguments passed to the programme.
		 * This routine is called for each argument passed to the application which is
		 * not an option.
		 *
		 * @param arg  Argument passed to the programme.
		 * @return -1 on failure, 0 on success.
		 */
		virtual int processArgs (const char *arg);

		/**
		 * Add an option to the processed ones.
		 *
		 * @param in_short_option  Short option. This is passed to processOption.
		 * @param in_long_option   Long option. Can be string.
		 * @param in_has_arg       0 if this option do not has an optarg, 1 if it has, 2 if it can have.
		 * @param in_help_msg      Help message printed to the user.
		 */
		void addOption (int in_short_option, const char *in_long_option, int in_has_arg, const char *in_help_msg);

		/**
		 * Return application name, taken from the command line.
		 *
		 * @return Application name (= argv[0]).
		 */
		const char *getAppName ()
		{
			return (app_argv != NULL && app_argv[0] != NULL) ? app_argv[0] : "--child--";
		}

		/**
		 * Print ussage line. Called from help() routine to print recommended usage.
		 */
		virtual void usage ();
		
		/**
		 * Prints help message.
		 */
		virtual void help ();

		/**
		 * Called from run() routine to initialize application.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int init ();

		void setDebug (int d) { debug = d; }

	private:
		/**
		 * Holds options which might be passed to the program.
		 */
		std::vector < Option > options;

		/**
		 * Call to process options and arguments.
		 */
		int initOptions ();

		/**
		 * Setup signal handling.
		 */
		void initSignals ();

		/**
		 * Prints help message, describing all options
		 */
		void helpOptions ();
		
		/**
		 * Copy of argc.
		 */
		int app_argc;

		/**
		 * Copy of argv.
		 */
		char **app_argv;

		/**
		 * Flag for loopEnd. True if application shall finish.
		 */
		bool end_loop;

		// debug level
		int debug;

		// use local time
		bool useLocalTime;
};

}

/**
 * Returns master (singleton) application.
 * If there are more then one App object in the executable, the one which constructor was
 * called latter will be used.
 *
 * @return App object.
 */
rts2core::App *getMasterApp ();

/**
 * Create logging stream.
 * 
 * Ussual call from anywhere in the code is:
 @code
 logStream (MESSAGE_XXX) << "send message" << send_param << sendLog;
 @endcode
 *
 * @param in_messageType Message type.
 * @return Message stream.
 */
rts2core::LogStream logStream (messageType_t in_messageType);
#endif							 /* !__RTS2_APP__ */
