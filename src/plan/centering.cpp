#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "camera_info.h"
#include "phot_info.h"
#include "centering.h"

#define MAX_COUNT		20

#define DARK_FILTER		1
#define LIGHT_FILTER		2
#define COUNTS			3

struct count_info
{
  int filter;
  int count;
} counts[MAX_COUNT];

int last_count = 0;

double step_ra = 0.25;
double step_dec = 0.25;

int
phot_handler_centering (struct param_status *params, struct phot_info *info)
{
  time_t t;
  int ret;
  if (!strcmp (params->param_argv, "filter"))
    return param_next_integer (params, &info->filter);
  if (!strcmp (params->param_argv, "integration"))
    return param_next_integer (params, &info->integration);
  if (!strcmp (params->param_argv, "count"))
    {
      char tc[30];
      ctime_r (&t, tc);
      tc[strlen (tc) - 1] = 0;
      ret = param_next_integer (params, &info->count);
      printf ("centering %s %i %i \n", tc, info->filter, info->count);
      fflush (stdout);
      counts[last_count].count = info->count;
      counts[last_count].filter = info->filter;
      last_count++;
      last_count %= MAX_COUNT;
    }
  return ret;
}

int
next_move (double right, double up, struct device *telescope)
{
  if (devcli_command (telescope, NULL, "change %f %f", right, up))
    {
      printf ("telescope error - change\n\n--------------\n");
      return -1;
    }
  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			  TEL_OBSERVING, 100);
  return 0;
}

double
phot_integrate (struct device *phot, int filter, int count)
{
  int i, j = 0;
  double total = 0;
  devcli_command (phot, NULL, "filter %i", filter);
  devcli_command (phot, NULL, "integrate 1 %", count);
  devcli_wait_for_status (phot, "phot", PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
			  100);
  for (i = 0; i < last_count; i++)
    {
      if (counts[i].filter == filter)
	{
	  total += counts[i].count;
	  j++;
	}
    }
  last_count = 0;
  if (j > 0)
    return total / j;
  return nan ("a");
}

/**
 *
 * @return 0 on sucess (centered), !0 otherwise
 */
int
rts2_centering (struct device *camera, struct device *telescope)
{
  struct device *phot = NULL;
  devcli_handle_response_t old_handler = NULL;
  int step, ret, i;
  int step_size_x, step_size_y;
  int step_size, up_d;
  int right, up;
  double phot_dark, phot_light;

  for (phot = devcli_devices (); phot; phot = phot->next)
    if (phot->type == DEVICE_TYPE_PHOT)
      {
	printf ("changing handler: %s\n", phot->name);
	old_handler = devcli_set_command_handler (phot,
						  (devcli_handle_response_t)
						  phot_handler_centering);
	break;
      }
  if (!phot)			// no photometer
    return 1;
  step = 0;
  step_size_x = 1;
  step_size_y = 1;
  step_size = step_size_x * 2 + step_size_y;
  up_d = 1;
  phot_dark = phot_integrate (phot, DARK_FILTER, COUNTS);
  if (isnan (phot_dark))
    {
      ret = 1;
      goto end;
    }
  for (i = 0; i < 100; i++)
    {
      // one centering step
      right = 0;
      up = 0;
      if (step < step_size_x)
	{
	  up = 1;
	}
      else if (step < step_size_x + step_size_y)
	{
	  right = 1;
	}
      else
	{
	  up = -1;
	}
      ret =
	next_move (up_d * right / step_ra, up_d * up / step_dec, telescope);
      if (ret)
	goto end;
      step++;
      if (step == step_size)
	{
	  up_d *= -1;
	  if (up == 1)
	    {
	      step_size_x++;
	    }
	  step_size_y++;
	  // calculate new step size
	  step_size = step_size_x * 2 + step_size_y;
	  step = 0;
	}
      // now do the work
      phot_light = phot_integrate (phot, LIGHT_FILTER, COUNTS);
      if (phot_light / phot_dark > 2.0)
	{
	  ret = 0;
	  goto end;
	}
    }
  ret = 2;
end:
  if (old_handler)
    devcli_set_command_handler (phot, old_handler);
  return ret;
}
