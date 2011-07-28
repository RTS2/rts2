/* 
 * Infrastructure for option parser.
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

#include "option.h"

#include <iostream>
#include <iomanip>

using namespace rts2core;

void Option::getOptionChar (char **end_opt)
{
	if (!isalnum (short_option))
		return;
	**end_opt = short_option;
	(*end_opt)++;
	if (has_arg)
	{
		**end_opt = ':';
		(*end_opt)++;
		// optional text after option..
		if (has_arg == 2)
		{
			**end_opt = ':';
			(*end_opt)++;
		}
	}
}

void Option::help ()
{
	if (short_option < 900)
	{
		if (long_option)
		{
			std::cout << "  -" << ((char) short_option) << "|--" << std::
				left << std::setw (15) << long_option;
		}
		else
		{
			std::cout << "  -" << ((char) short_option) << std::
				setw (18) << " ";
		}
	}
	else
	{
		std::cout << "  --" << std::left << std::setw (18) << long_option;
	}
	std::cout << " " << help_msg << std::endl;
}
