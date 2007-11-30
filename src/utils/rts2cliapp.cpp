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

#include "rts2cliapp.h"

#include <iostream>

Rts2CliApp::Rts2CliApp (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
}


void
Rts2CliApp::afterProcessing ()
{
}


int
Rts2CliApp::run ()
{
	int ret;
	ret = init ();
	if (ret)
		return ret;
	ret = doProcessing ();
	afterProcessing ();
	return ret;
}


int
Rts2CliApp::parseDate (const char *in_date, struct ln_date *out_time)
{
	int ret;
	int ret2;
	out_time->hours = out_time->minutes = 0;
	out_time->seconds = 0;
	ret =
		sscanf (in_date, "%i-%i-%i%n", &out_time->years, &out_time->months,
		&out_time->days, &ret2);
	if (ret == 3)
	{
		in_date += ret2;
		// we end with is T, let's check if it contains time..
		if (*in_date == 'T' || isspace (*in_date))
		{
			in_date++;
			ret2 =
				sscanf (in_date, "%u:%u:%lf", &out_time->hours,
				&out_time->minutes, &out_time->seconds);
			if (ret2 == 3)
				return 0;
			ret2 =
				sscanf (in_date, "%u:%u", &out_time->hours, &out_time->minutes);
			if (ret2 == 2)
				return 0;
			ret2 = sscanf (in_date, "%i", &out_time->hours);
			if (ret2 == 1)
				return 0;
			std::cerr << "Cannot parse time of the date: " << in_date << std::
				endl;
			return -1;
		}
		// only year..
		return 0;
	}
	std::cerr << "Cannot parse date: " << in_date << std::endl;
	return -1;
}
