#define _GNU_SOURCE

#include <string.h>

#include <libnova/libnova.h>

#include "../telescope.h"
#include "maps.c"

#define TEL_STATUS  	Tstat->serial_number[63]
#define MOVE_TIMEOUT	200

extern int
telescope_init (const char *device_name, int telescope_id)
{
  use_maps ();
  return 0;
}

extern void
telescope_done ()
{
  // should be something like close_maps..
}

extern int
telescope_base_info (struct telescope_info *info)
{
  sprintf (info->type, Tstat->type);
  sprintf (info->serial_number, Tstat->serial_number);
  info->longtitude = Tstat->longtitude;
  info->latitude = Tstat->latitude;
  info->altitude = Tstat->altitude;
  info->park_dec = Tstat->park_dec;

  return 0;
}

double
get_loc_sid_time ()
{
  return 15 * ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) +
    Tstat->longtitude;
}

extern int
telescope_info (struct telescope_info *info)
{
  info->ra = Tstat->ra;
  info->dec = Tstat->dec;

  info->siderealtime = get_loc_sid_time ();
  info->localtime = Tstat->localtime;
  info->flip = Tstat->flip;

  info->axis0_counts = Tstat->axis0_counts;
  info->axis1_counts = Tstat->axis1_counts;

  return 0;
}

extern int
telescope_move_to (double ra, double dec)
{
  int timeout = 0;
  Tctrl->power = 1;
  Tctrl->ra = ra;
  Tctrl->dec = dec;
  sleep (2);

  while (TEL_STATUS != TEL_POINT && timeout < TEL_TIMEOUT)
    {
      timeout++;
      sleep (1);
    }
  return (timeout >= TEL_TIMEOUT);
}

extern int
telescope_set_to (double ra, double dec)
{
  return -1;
}

extern int
telescope_correct (double ra, double dec)
{
  return -1;
}

extern int
telescope_change (double ra, double dec)
{
  return -1;
}

extern int
telescope_start_move (char direction)
{
  return -1;
}

extern int
telescope_stop_move (char direction)
{
  return -1;
}

extern int
telescope_stop ()
{
  // should do the work
  return telescope_move_to (Tstat->ra, Tstat->dec);
}

extern int
telescope_park ()
{
  int ret;
  ret = telescope_move_to (get_loc_sid_time () - 30, Tstat->dec);
  Tctrl->power = 0;
  return ret;
}
extern int
telescope_home ()
{
  return 0;
}

extern int
telescope_off ()
{
  Tctrl->power = 0;
  sleep (2);
  return TEL_STATUS != TEL_TIMEOUT;
}
