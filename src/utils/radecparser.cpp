/* 
 * Parser for RA DEC string.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include <sstream>

#include "radecparser.h"

int
parseRaDec (const char *radec, double &ra, double &dec)
{
	std::istringstream is (radec);
	// now try to parse it to ra dec..
	ra = 0;
	dec = 0;
	int dec_sign = 1;
	int step = 1;
	enum { NOT_GET, RA, DEC }
	state;
	bool ra_six = false;

	state = NOT_GET;
	while (1)
	{
		std::string next;
		is >> next;
		if (is.fail ())
			break;
		std::istringstream next_num (next);
		int next_c = next_num.peek ();
		if (next_c == '+' || next_c == '-')
		{
			if (state != RA)
				return -1;
			state = DEC;
			step = 1;
			if (next_c == '-')
			{
				dec_sign = -1;
				next_num.get ();
			}
		}
		double val;
		next_num >> val;
		if (next_num.fail ())
			break;
		switch (state)
		{
			case NOT_GET:
				state = RA;
			case RA:
				ra += (val / step);
				if (step > 1)
					ra_six = true;
				step *= 60;
				break;
			case DEC:
				dec += (val / step);
				step *= 60;
				break;
		}
	}

	if (state == DEC)
		return 0;
	return -1;
}
