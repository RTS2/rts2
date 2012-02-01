/* 
 * Interface to ask users among diferent options.
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

#ifndef __RTS2_ASKCHOICE__
#define __RTS2_ASKCHOICE__

#include "app.h"

#include <list>
#include <ostream>

namespace rts2core
{

/**
 * Class representing one user choice. Has key associated with the
 * choice, and description of the choice.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Choice
{
	private:
		char key;
		const char *desc;
	public:
		/**
		 * Construct choice with assigned key and description.
		 *
		 * @param in_key  Key assigned to choice.
		 * @param in_desc Description assigned to choice.
		 */
		Choice (char in_key, const char *in_desc)
		{
			key = in_key;
			desc = in_desc;
		}
		
		/**
		 * Prints choice to a stream.
		 *
		 * @param _os     Stream to print on.
		 * @param choice  Choice to print.
		 * @return Stream to which choice was printed.
		 */
		friend std::ostream & operator << (std::ostream & _os, Choice & choice)
		{
			_os << "  " << choice.key << " .. " << choice.desc << std::endl;
			return _os;
		}
};

/**
 * Present set of choices to user.
 * Holds list of choices, and allow user to make selection from them.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class AskChoice: public std::list <Choice>
{
	private:
		App * app;
	public:
		/**
		 * Construct choice set.
		 *
		 * @param in_app Application associted with the set. Its askForChar
		 * method will be used in askChoice call.
		 */
		AskChoice (App * in_app);

		/**
		 * Destructor of the choice set.
		 */
		virtual ~ AskChoice (void);

		/**
		 * Add choice to set of choices.
		 *
		 * @param key   Key associated with choice.
		 * @param desc  Description of the choice.
		 */
		void addChoice (char key, const char *desc);

		/**
		 * Ask user for selection among choices.
		 *
		 * @param _os  Stream to which question will be printed.
		 * @return  Key pressed by user.
		 */
		char query (std::ostream & _os);
};

}
#endif							 /* !__RTS2_ASKCHOICE__ */
