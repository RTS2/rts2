#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../utils/config.h"
#include "camera_info.h"
#include "phot_info.h"
#include "centering.h"
#include "median.h"

#define MAX_COUNT		20

#define DARK_FILTER		0
#define LIGHT_FILTER		2
#define COUNTS			3

struct count_info
{
  int filter;
  int count;
} counts[MAX_COUNT];

int last_count = 0;

double step_ra;
double step_dec;

int err_ra;
int err_dec;

int total_step;

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
      time (&t);
      ctime_r (&t, tc);
      tc[strlen (tc) - 1] = 0;
      ret = param_next_integer (params, &info->count);
      printf ("centering step %4i err_ra %+3i err_dec %+3i %s %i %i \n",
	      total_step, err_ra, err_dec, tc, info->filter, info->count);
      fflush (stdout);
      counts[last_count].count = info->count;
      counts[last_count].filter = info->filter;
      last_count++;
      last_count %= MAX_COUNT;
    }
  return ret;
}

int
next_move (int right, int up, struct device *telescope)
{
  if (devcli_command
      (telescope, NULL, "change %f %f", right * step_ra, up * step_dec))
    {
      printf ("telescope error - change\n\n--------------\n");
      return -1;
    }
  err_ra += right;
  err_dec += up;
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
  devcli_command (phot, NULL, "integrate 1 %i", count);
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
 * @param 
 *
 * @return 0 if proceed without error, non-zero otherwise
 */
int
find_center_real (int *step, int par_x, int par_y, struct device *phot, struct device *telescope, double phot_dark, int dir)
{
#define MAX_MOVE 	20
  int ret, ret2;
  double phot_actual, last_phot;
  last_phot = phot_integrate (phot, LIGHT_FILTER, COUNTS);
  for ( *step = 0; *step < MAX_MOVE;)
  {
    ret = next_move (dir * par_x, dir * par_y, telescope);
    if (ret)
      break;
    *step = *step + 1;
    phot_actual = phot_integrate (phot, LIGHT_FILTER, COUNTS);
    if (phot_actual / last_phot < 0.75 || phot_actual - phot_dark < get_device_double_default (phot->name, "minsn", 250))
    {
      break;
    }
  }
  dir *= -1;
  // return back..
  ret2 = next_move (dir * par_x * *step, dir * par_y * *step, telescope);
  return ret || ret2;
}

int
find_center (int par_x, int par_y, struct device *phot, struct device *telescope, double phot_dark)
{
  int i;
  int step_1, step_2;
  int ret_1, ret_2;
  // move to one end;
  ret_1 = find_center_real (&step_1, par_x, par_y, phot, telescope, phot_dark, 1);
  ret_2 = find_center_real (&step_2, par_x, par_y, phot, telescope, phot_dark, -1);
  if (ret_1 || ret_2)
    return -1;
  ret_1 = (step_1 - step_2) / 2;
  // move to center
  ret_2 = next_move (par_x * ret_1, par_y * ret_1, telescope);
  printf ("center centering err_ra: %i (%f) err_dec: %i (%f)\n", err_ra,
    err_ra * step_ra, err_dec, err_dec * step_dec);
  return ret_2;
}

/**
 *
 * @return 0 on sucess (centered), !0 otherwise
 */
int
rts2_centering (struct device *camera, struct device *telescope)
{
  struct device *phot = NULL;
  char *phot_name = get_string_default ("centering_phot", NULL);
  devcli_handle_response_t old_handler = NULL;
  int ret, step;
  int step_size_x, step_size_y;
  int step_size, up_d;
  int right, up;
  int right_move, up_move;
  int step_background;
  double phot_dark, phot_light, phot_background;
  double *background_array = NULL;
  double back_max = -1;
  int back_max_ra, back_max_dec;

  err_ra = 0;
  err_dec = 0;

  for (phot = devcli_devices (); phot; phot = phot->next)
    if (phot->type == DEVICE_TYPE_PHOT && !strcmp (phot->name, phot_name))
      {
	printf ("centering changing handler: %s\n", phot->name);
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
  step_ra = get_double_default ("centering_step_ra", 0.4);
  step_dec = get_double_default ("centering_step_dec", 0.4);
  step_ra /= 60.0; // convert to degrees
  step_dec /= 60.0; // convertm to degrees
  step_background = (int) get_device_double_default (phot->name, "step_background", 10);
  background_array = (double*) malloc (sizeof (double) * step_background);
  up_d = 1;
  phot_dark = phot_integrate (phot, DARK_FILTER, COUNTS);
  if (isnan (phot_dark))
    {
      ret = 1;
      goto end;
    }
  for (total_step = 0; total_step < 500; total_step++)
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
      right_move = up_d * right;
      up_move = up_d * up;
      ret = next_move (right_move, up_move, telescope);
      if (ret)
	goto end;
      // now do the work
      phot_light = phot_integrate (phot, LIGHT_FILTER, COUNTS);
      // bellow backgroudn step, save for futher use..
      if (total_step < step_background)
      {
	background_array[total_step] = phot_light;
	if  (phot_light > back_max)
	{
	  back_max = phot_light;
	  back_max_ra = err_ra;
	  back_max_dec = err_dec;
	  printf ("max_back: %f err_ra: %i err_dec: %i\n", back_max, back_max_ra, back_max_dec);
	}
      }
      // calculate background, get maximum if found, proceed with maximum
      if (total_step == step_background)
      {
	// calculate background
	// median..
	phot_background = get_median (background_array, step_background);
	printf ("background: %f\n", phot_background);
	if (back_max - phot_background > get_device_double_default (phot->name, "minsn", 250))
	{
	  // move the center
	  phot_light = back_max;
	  ret = next_move (back_max_ra - err_ra, back_max_dec - err_dec, telescope);
	  if (ret)
	    goto end;
	}
      }
      if (total_step >= step_background 
        && phot_light - phot_background > get_device_double_default (phot->name, "minsn", 250))
	{
	  // something found, make step aside, and recenter
	  printf ("err_ra: %i (%f) err_dec: %i (%f)\n", err_ra,
		  err_ra * step_ra, err_dec, err_dec * step_dec);
	  if (!right_move)
	    {
	      right_move = up_d;
	    }
	  if (!up_move)
	    {
	      up_move = up_d;
	    }
	  step_ra /= 2.0;
	  step_dec /= 2.0;
/* ret = next_move (right_move, up_move, telescope);
	  if (ret)
	    goto end;
	  // find center
	  phot_light = phot_integrate (phot, LIGHT_FILTER, COUNTS);
	  if (phot_light - phot_dark < get_device_double_default (phot->name, "minsn", 250))
	    {
              // bad idea, move back
	      next_move (-right_move, -up_move, telescope);
	      printf ("bad idea, moved back\n");
	    } */
	  step_ra /= 2.0;
	  step_dec /= 2.0;
	  err_ra = 0;
	  err_dec = 0;
	  // find one end
	  ret = find_center (1, 0, phot, telescope, phot_background);
	  if (ret)
	    goto end;
	  // find other end
	  ret = find_center (0, 1, phot, telescope, phot_background);
	  if (ret)
	    goto end;
	  phot_integrate (phot, LIGHT_FILTER, COUNTS); 
	  ret = 0;
	  goto end;
	}
      step++;
      if (step == step_size)
	{
	  up_d *= -1;
	  if (up_d == 1)
	    {
	      step_size_x++;
	      if (step_size_x > 10)
		{
		  ret = 3;
		  goto end;
		}
	    }
	  step_size_y++;
	  // calculate new step size
	  step_size = step_size_x * 2 + step_size_y;
	  step = 0;
	}
    }
  ret = 2;
end:
  if (old_handler)
    devcli_set_command_handler (phot, old_handler);
  free (background_array);
  return ret;
}
