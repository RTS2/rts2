/* 
 * Extends libnova by calculating next riseset event, not the one occuring today.
 * Copyright (C) 2003 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS_RISESET__
#define __RTS_RISESET__

#include <libnova/ln_types.h>
#include <time.h>

#include "status.h"

#ifdef __cplusplus
extern "C"
{
	#endif
	/**
	 * Calculate rise and set times. Used for calculating of next events (dusk, night, dawn, day).
	 *
	 * @author Petr Kubanek <petr@kubanek.net>
	 */
	int next_event (struct ln_lnlat_posn *observer, time_t * start_time, rts2_status_t *curr_type, rts2_status_t *type, time_t * ev_time, double night_horizon, double day_horizon, int in_eve_time, int in_mor_time, bool verbose = false);

	#ifdef __cplusplus
};
#endif
#endif							 /* __RTS_RISESET__ */
