/* 
 * Infrastructure for option parser.
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

#ifndef __RTS2_OPTION__
#define __RTS2_OPTION__

#include "rts2-config.h"

#ifdef RTS2_HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt_own.h"
#endif
#include <malloc.h>
#include <stdio.h>

// custom configuration values

#define OPT_VERSION          999
#define OPT_DATABASE        1000
#define OPT_CONFIG          1001

#define OPT_LOCALHOST       1002
#define OPT_SERVER          1003
#define OPT_PORT            1004

#define OPT_LOCALPORT       1005

#define OPT_MODEFILE        1006
#define OPT_LOGFILE         1007
#define OPT_LOCKPREFIX      1008
#define OPT_OPENTPL_SERVER  1009 

#define OPT_NOTCHECKNULL    1010

#define OPT_RUNAS           1011
#define OPT_NOAUTH          1012

#define OPT_AUTOSAVE        1013

#define OPT_VALUEFILE       1014

/**
 * Start of local option number playground.
 */
#define OPT_LOCAL      10000

namespace rts2core
{

/**
 * A program option.
 *
 * This class represents program option.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Option
{
	public:
		Option (int in_short_option, const char *in_long_option, int in_has_arg, const char *in_help_msg)
		{
			short_option = in_short_option;
			long_option = in_long_option;
			has_arg = in_has_arg;
			help_msg = in_help_msg;
		}
		/**
		 * Print help message for the option.
		 */
		void help ();
		void getOptionChar (char **end_opt);
		bool haveLongOption () { return long_option != NULL; }
		void getOptionStruct (struct option *options)
		{
			options->name = long_option;
			options->has_arg = has_arg;
			options->flag = NULL;
			options->val = short_option;
		}

	private:
		int short_option;
		const char *long_option;
		int has_arg;
		const char *help_msg;
};

}
#endif							 /* !__RTS2_OPTION__ */
