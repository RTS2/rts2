/* 
 * Application sceleton.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2object.h"
#include "rts2option.h"
#include "rts2message.h"
#include "rts2logstream.h"

class Rts2App:public Rts2Object
{
	private:
		std::vector < Rts2Option * >options;

		/**
		 * Prints help message, describing all options
		 */
		void helpOptions ();

		int argc;
		char **argv;

		bool end_loop;

	protected:
		virtual int processOption (int in_opt);
								 // for non-optional args
		virtual int processArgs (const char *arg);
		int addOption (int in_short_option, char *in_long_option, int in_has_arg,
			char *in_help_msg);

		int initOptions ();

		/**
		 * Ask user for integer.
		 */
		int askForInt (const char *desc, int &val);
		int askForDouble (const char *desc, double &val);
		int askForString (const char *desc, std::string & val);
		bool askForBoolean (const char *desc, bool val);

		virtual void help ();
		virtual int init ();

	public:
		Rts2App (int in_argc, char **in_argv);
		virtual ~ Rts2App ();

		/**
		 * Run method of the application.
		 *
		 * Application is responsible for calling init () method to read
		 * variables etc..
		 *
		 * Both Rts2Daemon and Rts2CliApp define run method, as this method is
		 * pure virtual.
		 */
		virtual int run () = 0;

		/**
		 * If running in loop, caused end of loop.
		 */
		virtual void endRunLoop ()
		{
			exit (0);
		}

		bool getEndLoop ()
		{
			return end_loop;
		}

		void setEndLoop (bool in_end_loop)
		{
			end_loop = in_end_loop;
		}

		/**
		 * Ask user for char, used to ask for chair in choice question.
		 */
		int askForChr (const char *desc, char &out);

		virtual void sendMessage (messageType_t in_messageType,
			const char *in_messageString);
		inline void sendMessage (messageType_t in_messageType,
			std::ostringstream & _os);

		virtual Rts2LogStream logStream (messageType_t in_messageType);

		// send mail to recepient; requires /usr/bin/mail binary; if master object is specified, that object
		// method forkedInstance will be called, which might close whenever descriptors are needed
		virtual int sendMailTo (const char *subject, const char *text,
			const char *in_mailAddress);

		virtual void sigHUP (int sig);
};

Rts2App *getMasterApp ();
Rts2LogStream logStream (messageType_t in_messageType);
#endif							 /* !__RTS2_APP__ */
