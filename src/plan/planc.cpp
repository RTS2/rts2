/*!
 * @file Plan test client
 * $Id$
 *
 * Planner client.
 *
 * @author petr
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /*! _GNU_SOURCE */

#include <errno.h>
#include <ctype.h>
#include <libnova/libnova.h>

#include "../utils/config.h"
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../writers/fits.h"
#include "status.h"
#include "target.h"
#include "plan.h"
#include "camera_info.h"
#include "phot_info.h"
#include "../db/db.h"
#include "../writers/process_image.h"
#include "centering.h"

char *phot_filters[] = {
  "A", "B", "C", "D", "E", "F"
};

struct device *telescope;

static int phot_obs_id = 0;

struct ln_lnlat_posn observer;

pthread_t image_que_thread;

int watch_status = 1;		// watch central server status

int ignore_astro = 0;

int
phot_handler (struct param_status *params, struct phot_info *info)
{
  time_t t;
  time (&t);
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
      printf ("%05i %s %i %i \n", phot_obs_id, tc, info->filter, info->count);
      fflush (stdout);
      if (telescope)
	{
	  db_add_count (phot_obs_id, &t, info->count, info->integration,
			phot_filters[info->filter],
			telescope->info.telescope.ra,
			telescope->info.telescope.dec, "PHOT0");
	}
    }
  return ret;
}

// returns 1, when we are in state allowing observation
int
ready_to_observe (int status)
{
  char *autoflats;
  autoflats = get_string_default ("autoflats", "Y");
  if (autoflats[0] == 'Y')
    return status == SERVERD_NIGHT
      || status == SERVERD_DUSK || status == SERVERD_DAWN;
  return status == SERVERD_NIGHT;
}

int
main (int argc, char **argv)
{
  Rts2Plan *plan = new Rts2Plan (argc, argv);
  plan->init ();
  plan->run ();
  delete plan;
  return 0;
}
