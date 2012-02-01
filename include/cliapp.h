/* 
 * Command Line Interface application sceleton.
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

#ifndef __RTS2_CLIAPP__
#define __RTS2_CLIAPP__

#include "app.h"

#include <libnova/libnova.h>

namespace rts2core
{

/**
 * Abstract base class for all client applications.
 *
 * Provides doProcessing() and afterProcessing() functions. The doProcessing()
 * must be defined in any descendant, afterProcessing() method is optional.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class CliApp:public App
{
	public:
		CliApp (int in_argc, char **in_argv);

		virtual int run ();
	protected:
		/**
		 * Called for application processing. This method shall be used instead of
		 * run() method.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int doProcessing () = 0;

		/**
		 * Called after processing is done. This method may be used to print
		 * summary of processed data etc.
		 */
		virtual void afterProcessing ();
};

}
#endif							 /* !__RTS2_CLIAPP__ */
