/* 
 * Extends libnova by calculating next riseset event, not the one occuring today.
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


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/julian_day.h>
#include <libnova/solar.h>
#include <math.h>
#include <stdio.h>
#include <iostream>

#include "libnova_cpp.h"
#include "riseset.h"

int next_naut (double jd, struct ln_lnlat_posn *observer, struct ln_rst_time *rst, struct ln_rst_time *rst_naut, int *sun_rs, double night_horizon, double day_horizon)
{
	double t_jd = jd - 2;
	int sun_naut;
	struct ln_rst_time last_naut;
	struct ln_rst_time last_rst;

	rst_naut->rise = rst_naut->transit = rst_naut->set = 0;
	rst->rise = rst->transit = rst->set = 0;

	last_naut.rise = last_naut.transit = last_naut.set = 0;
	last_rst.rise = last_rst.transit = last_rst.set = 0;
	*sun_rs = 0;
	// find first next day, on which nautic sunset is occuring
	do
	{
		struct ln_rst_time t_rst;
		sun_naut = ln_get_solar_rst_horizon (t_jd, observer, night_horizon, &t_rst);
		if (!rst_naut->rise && jd < t_rst.rise)
			rst_naut->rise = t_rst.rise;
		if (last_naut.rise && (t_rst.rise - last_naut.rise) > 1.5)
		{
			last_naut.rise = (t_rst.rise + last_naut.rise) / 2.0;
			if (jd < last_naut.rise)
				rst_naut->rise = last_naut.rise;
		}
		if (!rst_naut->transit && jd < t_rst.transit)
			rst_naut->transit = t_rst.transit;
		if (last_naut.transit && (t_rst.transit - last_naut.transit) > 1.5)
		{
			last_naut.transit = (t_rst.transit + last_naut.transit) / 2.0;
			if (jd < last_naut.transit)
				rst_naut->transit = last_naut.transit;
		}
		if (!rst_naut->set && jd < t_rst.set)
			rst_naut->set = t_rst.set;
		if (last_naut.set && (t_rst.set - last_naut.set) > 1.5)
		{
			last_naut.set = (t_rst.set + last_naut.set) / 2.0;
			if (jd < last_naut.set)
				rst_naut->set = last_naut.set;
		}
		last_naut = t_rst;
		if (!ln_get_solar_rst_horizon (t_jd, observer, day_horizon, &t_rst))
		{
			*sun_rs = 1;
			if (!rst->set && jd < t_rst.set)
				rst->set = t_rst.set;
			if (last_rst.set && (t_rst.set - last_rst.set) > 1.5)
			{
				last_rst.set = (t_rst.set + last_rst.set) / 2.0;
				if (jd < last_rst.set)
					rst->set = last_rst.set;
			}
			if (!rst->transit && jd < t_rst.transit)
				rst->transit = t_rst.transit;
			if (last_rst.transit && (t_rst.transit - last_rst.transit) > 1.5)
			{
				last_rst.transit = (t_rst.transit + last_rst.transit) / 2.0;
				if (jd < last_rst.transit)
					rst->transit = last_rst.transit;
			}
			if (!rst->rise && jd < t_rst.rise)
				rst->rise = t_rst.rise;
			if (last_rst.rise && (t_rst.rise - last_rst.rise) > 1.5)
			{
				last_rst.rise = (t_rst.rise + last_rst.rise) / 2.0;
				if (jd < last_rst.rise)
					rst->rise = last_rst.rise;
			}
			last_rst = t_rst;
		}
		t_jd++;
	}
	while (sun_naut != 0 || !rst_naut->rise || !rst_naut->transit
		|| !rst_naut->set || ((!rst->rise || !rst->transit || !rst->set)
		&& *sun_rs == 1));

	return 0;
}

int next_event (struct ln_lnlat_posn *observer, time_t * start_time, rts2_status_t *curr_type, rts2_status_t *type, time_t * ev_time, double night_horizon, double day_horizon, int in_eve_time, int in_mor_time, bool verbose)
{
	double jd_time = ln_get_julian_from_timet (start_time);
	struct ln_rst_time rst, rst_naut;

	int sun_rs;

	next_naut (jd_time, observer, &rst, &rst_naut, &sun_rs, night_horizon, day_horizon);

	if (verbose)
	{
		std::cout << "rise " << LibnovaDate (rst.rise) << " transit " << LibnovaDate (rst.transit) << " set " << LibnovaDate (rst.set) << std::endl
			<< "nautical rise " << LibnovaDate (rst_naut.rise) << " transit " << LibnovaDate (rst_naut.transit) << " set " << LibnovaDate (rst_naut.set) << std::endl;

	}

	// jd_time < rst_naut.rise && jd_time < rst_naut.transit && jd_time < rst_naut.set
	if (rst_naut.rise <= rst_naut.set)
	{
		*curr_type = SERVERD_NIGHT;
		*type = SERVERD_DAWN;
		ln_get_timet_from_julian (rst_naut.rise, ev_time);
	}
	else
	{
		if (sun_rs)
		{
			if (rst.set < rst.rise)
			{
				double eve_time = ((double) in_eve_time) / 86400.0;
				double mor_time = ((double) in_mor_time) / 86400.0;
				if (jd_time > rst.set - eve_time)
				{
					*curr_type = SERVERD_EVENING;
					*type = SERVERD_DUSK;
					ln_get_timet_from_julian (rst.set, ev_time);
				}
				else if (jd_time < rst.rise + mor_time - 1.0)
				{
					*curr_type = SERVERD_MORNING;
					*type = SERVERD_DAY;
					ln_get_timet_from_julian (rst.rise + mor_time - 1.0, ev_time);
				}
				else
				{
					*curr_type = SERVERD_DAY;
					*type = SERVERD_EVENING;
					ln_get_timet_from_julian (rst.set - eve_time, ev_time);
				}
			}
			else
			{
				if (rst_naut.rise < rst.rise)
				{
					*curr_type = SERVERD_DUSK;
					*type = SERVERD_NIGHT;
					ln_get_timet_from_julian (rst_naut.set, ev_time);
				}
				else
				{
					*curr_type = SERVERD_DAWN;
					*type = SERVERD_MORNING;
					ln_get_timet_from_julian (rst.rise, ev_time);
				}
			}

		}
		else
		{
			// rst_naut.rise > rst_naut.set
			if (jd_time < rst_naut.transit)
			{
				*curr_type = SERVERD_DAWN;
				*type = SERVERD_DUSK;
				ln_get_timet_from_julian (rst_naut.transit, ev_time);
			}
			else
			{
				*curr_type = SERVERD_DUSK;
				*type = SERVERD_NIGHT;
				ln_get_timet_from_julian (rst_naut.set, ev_time);
			}
		}
	}
	return 0;
}
