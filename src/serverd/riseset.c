/*!
 * @file Calculate rise and set times.
 *
 * Used for calculating of next events (dusk, night, dawn, day).
 *
 * @author petr
 */

#define _GNU_SOURCE

#include <libnova/julian_day.h>
#include <libnova/solar.h>
#include <math.h>
#include <stdio.h>

#include "riseset.h"
#include "status.h"
#include "../utils/config.h"

int
next_naut (double jd, struct ln_lnlat_posn *observer, struct ln_rst_time *rst,
	   struct ln_rst_time *rst_naut, int *sun_rs)
{
  double t_jd = jd - 1;
  int sun_naut;
  rst_naut->rise = rst_naut->transit = rst_naut->set = 0;
  rst->rise = rst->transit = rst->set = 0;
  *sun_rs = 0;
  // find first next day, on which nautic sunset is occuring
  do
    {
      struct ln_rst_time t_rst;
      sun_naut =
	ln_get_solar_rst_horizont (t_jd, observer,
				   get_double_default ("night_horizont", -8),
				   &t_rst);
      if (!rst_naut->rise && jd < t_rst.rise)
	rst_naut->rise = t_rst.rise;
      if (!rst_naut->transit && jd < t_rst.transit)
	rst_naut->transit = t_rst.transit;
      if (!rst_naut->set && jd < t_rst.set)
	rst_naut->set = t_rst.set;
      if (!ln_get_solar_rst_horizont
	  (t_jd, observer,
	   get_double_default ("day_horizont", LN_SOLAR_STANDART_HORIZONT),
	   &t_rst))
	{
	  *sun_rs = 1;
	  if (!rst->set && jd < t_rst.set)
	    rst->set = t_rst.set;
	  if (!rst->transit && jd < t_rst.transit)
	    rst->transit = t_rst.transit;
	  if (!rst->rise && jd < t_rst.rise)
	    rst->rise = t_rst.rise;
	}
      t_jd++;
    }
  while (sun_naut != 0 || !rst_naut->rise || !rst_naut->transit
	 || !rst_naut->set || ((!rst->rise || !rst->transit || !rst->set)
			       && *sun_rs == 1));

  return 0;
}

int
next_event (struct ln_lnlat_posn *observer, time_t * start_time,
	    int *curr_type, int *type, time_t * ev_time)
{
  double jd_time = ln_get_julian_from_timet (start_time);
  struct ln_rst_time rst, rst_naut;

  int sun_rs;
  double eve_time = 7200;	// seconds
  double mor_time = 1800;	// seconds

  next_naut (jd_time, observer, &rst, &rst_naut, &sun_rs);

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
	      eve_time = get_double_default ("evening_time", eve_time) / 86400.0;	// get from config, convert to defaults
	      mor_time =
		get_double_default ("morning_time", mor_time) / 86400.0;
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
		  ln_get_timet_from_julian (rst.rise + mor_time - 1.0,
					    ev_time);
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
