/* 
 * Helper method to get night from incomplete day.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "configuration.h"

void getNightDuration (int year, int month, int day, time_t &from, int64_t &duration)
{
	if (year <= 0)
	{
		year = 2000;
		month = day = 1;
		duration = 1000LL * 365 * 86400;
	}
	else if (month <= 0)
	{
	 	month = day = 1;
		duration = 365 * 86400;
	}
	else if (day <= 0)
	{
		day = 1;
		duration = 31 * 86400;
	}
	else
	{
		duration = 86400;
	}
	from = rts2core::Configuration::instance ()->getNight (year, month, day);
}
