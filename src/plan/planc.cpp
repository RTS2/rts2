/*!
 * @file Plan test client
 * $Id$
 *
 * Planner client.
 *
 * @author petr
 */

#define MAX_READOUT_TIME		120

#include <errno.h>
#include <ctype.h>
#include <libnova/libnova.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <mcheck.h>
#include <math.h>
#include <getopt.h>

#include "../utils/config.h"
#include "../utils/devcli.h"
#include "../utils/devconn.h"
#include "../writers/fits.h"
#include "status.h"
#include "selector.h"
#include "target.h"
#include "phot_info.h"
#include "../db/db.h"
#include "../writers/process_image.h"

#define EXPOSURE_TIME		2*60

#define COMMAND_EXPOSURE	'E'
#define COMMAND_FILTER		'F'
#define COMMAND_PHOTOMETER      'P'
#define COMMAND_CHANGE		'C'
#define COMMAND_WAIT            'W'

struct thread_list
{
  pthread_t thread;
  struct thread_list *next;
};

struct ex_info
{
  struct device *camera;
  Target *last;
  int move_count;
};

pthread_mutex_t script_thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t script_thread_count_cond = PTHREAD_COND_INITIALIZER;
static int script_thread_count = 0;	// number of running scripts...

static int running_script_count = 0;	// number of running scripts (not in W)

pthread_mutex_t move_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t move_count_cond = PTHREAD_COND_INITIALIZER;
static int move_count = 0;	// number of running exposures 

pthread_mutex_t precission_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t precission_cond = PTHREAD_COND_INITIALIZER;
static int precission_count = 0;

struct device *telescope;

struct ln_lnlat_posn observer;

pthread_t image_que_thread;

time_t last_succes = 0;

int watch_status = 1;		// watch central server status

int
phot_handler (struct param_status *params, struct phot_info *info)
{
  time_t t;
  time (&t);
  int ret;
  if (!strcmp (params->param_argv, "filter"))
    return param_next_integer (params, &info->filter);	  
  if (!strcmp (params->param_argv, "count"))
  {
    FILE *phot_log;
    char tc[30];
    phot_log = fopen ("phot_log", "a");
    ctime_r (&t, tc);
    tc[strlen(tc) - 1] =0;
    ret = param_next_integer (params, &info->count);
    if (telescope)
      fprintf (phot_log, "%s %f %f %i %i %i\n", tc, telescope->info.telescope.ra, telescope->info.telescope.dec, info->filter, info->count, telescope->statutes[0].status);
      printf ("%s %f %f %i %i %i\n", tc, telescope->info.telescope.ra, telescope->info.telescope.dec, info->filter, info->count, telescope->statutes[0].status);
      fflush (stdout);
    fclose (phot_log);
  }
  return ret;
}

int
move (Target * last)
{
  if ((last->type == TARGET_LIGHT || last->type == TARGET_FLAT)
      && last->moved == 0)
    {
      struct ln_equ_posn object;
      struct ln_hrz_posn hrz;
      last->getPosition (&object);
      ln_get_hrz_from_equ (&object, &observer, ln_get_julian_from_sys (),
			   &hrz);
      printf ("Ra: %f Dec: %f\n", object.ra, object.dec);
      printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);

      if (devcli_command
	  (telescope, NULL, "move %f %f", object.ra, object.dec))
	{
	  printf ("telescope error\n\n--------------\n");
	  return -1;
	}
      last->moved = 1;
    }
  else if (last->type == TARGET_DARK && last->moved == 0)
    {
      last->moved = 1;
      if (devcli_command (telescope, NULL, "park"))
	{
	  printf ("telescope error\n\n--------------\n");
	  return -1;
	}
    }
  return 0;
}

int
get_info (Target * entry, struct device *tel, struct device *cam,
	  float exposure, hi_precision_t * hi_precision)
{
  struct image_info *info =
    (struct image_info *) malloc (sizeof (struct image_info));
  int ret;

  info->camera_name = cam->name;
  printf ("info camera_name = %s\n", cam->name);
  info->telescope_name = tel->name;
  info->exposure_time = time (NULL);
  info->exposure_length = exposure;
  info->target_id = entry->id;
  info->observation_id = entry->obs_id;
  info->target_type = entry->type;
  if ((ret = devcli_command (cam, NULL, "info")))
    {
      printf ("camera info error\n");
    }
  memcpy (&info->camera, &cam->info, sizeof (struct camera_info));
  memcpy (&info->telescope, &tel->info, sizeof (struct telescope_info));
  info->hi_precision = hi_precision;
  devcli_image_info (cam, info);
  free (info);
  return ret;
}

int
generate_next (int i, Target * plan)
{
  time_t start_time;
  time (&start_time);
  start_time += EXPOSURE_TIME;

  printf ("Making plan %s", ctime (&start_time));
  if (get_next_plan
      (plan, (int) get_double_default ("planc_selector", SELECTOR_HETE),
       &start_time, i, EXPOSURE_TIME,
       watch_status ? devcli_server ()->statutes[0].status : SERVERD_NIGHT,
       observer.lng, observer.lat))
    {
      printf ("Error making plan\n");
      fflush (stdout);
      exit (EXIT_FAILURE);
    }
  printf ("...plan made\n");
  i++;
  return i;
}

/*
 *
 * @return 0 if I can observe futher, -1 if observation was canceled
 */
int
process_precission (Target * tar, struct device *camera)
{
  float exposure = 10;
  struct timespec timeout;
  time_t now;
  exposure =
    get_device_double_default (camera->name, "precission_exposure", exposure);
  if (tar->hi_precision == 0)
    return 0;
  if (strcmp (camera->name, get_string_default ("telescope_camera", "C0")))
    {
      // not camera for astrometry
      time (&now);
      timeout.tv_sec = now + 3600;
      timeout.tv_nsec = 0;
      pthread_mutex_lock (&precission_mutex);
      while (tar->hi_precision == 1)
	{
	  printf ("waiting for hi_precision (camera %s)\n", camera->name);
	  fflush (stdout);
	  pthread_cond_timedwait (&precission_cond, &precission_mutex, &timeout);
	}
      pthread_mutex_unlock (&precission_mutex);
    }
  else
    {
      double ra_err = NAN, dec_err = NAN;	// no astrometry
      double ra_margin = 15, dec_margin = 15;	// in arc minutes
      int tries = 0;
      int max_tries = (int) get_double_default ("maxtries", 5);

      ra_margin = get_double_default ("ra_margin", ra_margin);
      dec_margin = get_double_default ("dec_margin", dec_margin);
      ra_margin = ra_margin / 60.0;
      dec_margin = dec_margin / 60.0;
      hi_precision_t hi_precision;
      pthread_mutex_t ret_mutex = PTHREAD_MUTEX_INITIALIZER;
      pthread_cond_t ret_cond = PTHREAD_COND_INITIALIZER;
      hi_precision.mutex = &ret_mutex;
      hi_precision.cond = &ret_cond;

      devcli_command (camera, NULL, "binning 0 2 2");

      while (tries < max_tries)
	{
	  int light = (precission_count % 10 == 0) ? 0 : 1;
	  struct ln_equ_posn object;
	  if (!light)
	    tar->type = TARGET_DARK;
	  else
	    tar->type = TARGET_LIGHT;
	  tar->getPosition (&object);
	  printf
	    ("triing to get to %f %f, %i try, %s image, %i total precission\n",
	     object.ra, object.dec, tries, (light == 1) ? "light" : "dark",
	     precission_count);

	  precission_count++;

	  fflush (stdout);
	  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
				  TEL_OBSERVING, 0);

	  devcli_command (camera, NULL, "expose 0 %i %f", light, exposure);
	  devcli_command (telescope, NULL, "base_info;info");

	  get_info (tar, telescope, camera, exposure, &hi_precision);

	  devcli_wait_for_status (camera, "img_chip",
				  CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
				  (long int) (1.1 * exposure + 10));
	  devcli_command (camera, NULL, "readout 0");
	  devcli_wait_for_status (camera, "img_chip",
				  CAM_MASK_READING, CAM_NOTREADING,
				  MAX_READOUT_TIME);
	  if (light)
	    {
	      // after finishing astrometry, I'll have telescope true location
	      // in location parameters, passd as reference to get_info.
	      // So I can compare that with
	      pthread_mutex_lock (hi_precision.mutex);
	      time (&now);
	      timeout.tv_sec = now + 350;
	      timeout.tv_nsec = 0;
	      pthread_cond_timedwait (hi_precision.cond, hi_precision.mutex,
				      &timeout);
	      printf ("hi_precision->image_pos->ra: %f dec: %f ",
		      hi_precision.image_pos.ra, hi_precision.image_pos.dec);
	      pthread_mutex_unlock (hi_precision.mutex);
	      fflush (stdout);
	      if (!isnan (hi_precision.image_pos.ra)
		  && !isnan (hi_precision.image_pos.dec))
		{
		  ra_err =
		    ln_range_degrees (object.ra - hi_precision.image_pos.ra);
		  dec_err =
		    ln_range_degrees (object.dec -
				      hi_precision.image_pos.dec);
		}
	      else
		{
		  // not astrometry, get us out of here
		  tries = max_tries;
		  break;
		}
	      printf ("ra_err: %f dec_err: %f\n", ra_err, dec_err);
	      fflush (stdout);
	      if ((ra_err < ra_margin || ra_err > 360.0 - ra_margin)
		  && (dec_err < dec_margin || dec_err > 360.0 - dec_margin))
		{
		  break;
		}
	      else
		{
		  tar->moved = 0;
		  // try to synchro the scope again
		  move (tar);
		}
	      tries++;
	    }
	}
      devcli_command (camera, NULL, "binning 0 1 1");
      pthread_mutex_lock (&precission_mutex);
      tar->hi_precision = (tries < max_tries) ? 0 : -1;
      pthread_cond_broadcast (&precission_cond);
      pthread_mutex_unlock (&precission_mutex);
    }
  return tar->hi_precision;
}

int
dec_script_thread_count (void)
{
  pthread_mutex_lock (&script_thread_count_mutex);
  script_thread_count--;
  running_script_count--;
  printf ("new script thread count: %i\n", script_thread_count);
  fflush (stdout);
  pthread_cond_broadcast (&script_thread_count_cond);
  pthread_mutex_unlock (&script_thread_count_mutex);
  return 0;
}

void *
execute_camera_script (void *exinfo)
{
  struct device *camera = ((struct ex_info *) exinfo)->camera;
  Target *last = ((struct ex_info *) exinfo)->last;
  int light;
  char *command, *s;
  char *arg1, *arg2;
  float exposure;
  int filter, count;
  int ret;
  time_t t;
  char s_time[27];
  char obs_type_str[2];
  struct timespec timeout;
  time_t now;
  enum
  { NO_EXPOSURE, EXPOSURE_BEGIN, EXPOSURE_PROGRESS,
      INTEGRATION_PROGRESS } exp_state;

  switch (last->type)
    {
    case TARGET_LIGHT:
      obs_type_str[0] = last->obs_type;
      obs_type_str[1] = 0;
      command =
	get_sub_device_string_default (camera->name, "script", obs_type_str,
				       "E D");
      light = 1;
      ret = process_precission (last, camera);
      if (ret)
	{
	  dec_script_thread_count ();
	  return NULL;
	}
      break;
    case TARGET_DARK:
      command = "E D";
      light = 0;
      break;
    case TARGET_FLAT:
      command = "E 1";
      light = 1;
      break;
    case TARGET_FLAT_DARK:
      command = "E 1";
      light = 0;
      break;
    default:
      printf ("Unknow target type: %i\n", last->type);
      command = "E 1";
      light = 0;
    }

  exp_state = NO_EXPOSURE;

  while (*command)
    {
      if ((exp_state == EXPOSURE_BEGIN || exp_state == INTEGRATION_PROGRESS)
          && *command && !isspace (*command))
	{
	  // wait till exposure end..
	  devcli_command (telescope, NULL, "base_info;info");

	  get_info (last, telescope, camera, exposure, NULL);

	  if (exp_state == EXPOSURE_BEGIN)
	  {
	     devcli_wait_for_status (camera, "img_chip",
				  CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
				  (int) (1.1 * exposure + 10));
	     exp_state = EXPOSURE_PROGRESS;
	  }
	  if (exp_state == INTEGRATION_PROGRESS)
	  {
             devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
					  PHOT_MASK_INTEGRATE,
					  PHOT_NOINTEGRATE, 10000);
	     exp_state = NO_EXPOSURE;
	  }
	}
      switch (*command)
	{
	case COMMAND_EXPOSURE:
	  command++;
	  while (isspace (*command))
	    command++;
	  if (*command == 'D')
	    {
	      exposure =
		get_device_double_default (camera->name, "defexp",
					   EXPOSURE_TIME);
	      command++;
	    }
	  else
	    {
	      exposure = strtof (command, &s);
	      if (s == command || (!isspace (*s) && *s))
		{
		  fprintf (stderr,
			   "invalid arg, expecting float, get %s\n", s);
		  command = s;
		  continue;
		}
	      command = s;
	    }
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	    }
	  else if (exp_state == INTEGRATION_PROGRESS)
	    {
	      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
					  PHOT_MASK_INTEGRATE,
					  PHOT_NOINTEGRATE, 10000);
	    }
	  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
				  TEL_OBSERVING, 0);
	  devcli_command (camera, &ret, "expose 0 %i %f", light, exposure);
	  if (ret)
	    {
	      fprintf (stderr,
		       "error executing 'expose 0 %i %f', ret = %i\n",
		       light, exposure, ret);
	      continue;
	    }
	  time (&t);
	  t += (int) exposure;
	  ctime_r (&t, s_time);
	  printf ("exposure countdown (%s), readout at %s", camera->name,
		  s_time);
	  exp_state = EXPOSURE_BEGIN;
	  break;
	case COMMAND_FILTER:
	  command++;
	  filter = strtol (command, &s, 10);
	  if (s == command || (!isspace (*s) && *s))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;
	  devcli_command (camera, &ret, "filter %i", filter);
	  if (ret)
	    fprintf (stderr, "error executing 'filter %i'", filter);
	  break;
	case COMMAND_PHOTOMETER:
	  command++;
	  filter = strtol (command, &s, 10);
	  if (s == command || (!isspace (*s) && *s))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;
	  
	  exposure = strtof (command, &s);
	  if (s == command || (!isspace (*s) && *s))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;
	  
	  count = strtol (command, &s, 10);
	  if (s == command || (!isspace (*s) && *s))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;

	  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
				  TEL_OBSERVING, 0);
	  devcli_command_all (DEVICE_TYPE_PHOT, "filter %i", filter);
	  devcli_command_all (DEVICE_TYPE_PHOT, "integrate %f %i", exposure, count);
	  exp_state = INTEGRATION_PROGRESS;
	  break;
	case COMMAND_CHANGE:
	  // get two strings
	  command++;
	  s = command;
	  while (*command && !isspace (*command))
	  {
             command++;
	  }
	  arg1 = (char *) malloc (command - s + 1);
	  strncpy (arg1, s, command - s + 1);
	  s = command;
	  while (*command && !isspace (*command))
	  {
		  command ++;
	  }
	  arg2 = (char*) malloc (command - s + 1);
	  strncpy (arg2, s, command - s + 1);
	 
	  // wait till exposure count reaches 0 - no other thread do
	  // the exposure..
	  time (&now);
	  timeout.tv_sec = now + 1000;
	  timeout.tv_nsec = 0;
	  pthread_mutex_lock (&script_thread_count_mutex);
	  while (running_script_count > 1) // cause we will hold running_script_count lock in any case..
	    {
	      pthread_cond_timedwait (&script_thread_count_cond, &script_thread_count_mutex, &timeout);
	    }
	  pthread_mutex_unlock (&script_thread_count_mutex);

	  // we cannot imagine anything, 
	  // because there is forced readout before every command,
	  // just in case we integrate, let's finish
	  if (exp_state == INTEGRATION_PROGRESS)
	  {
	      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
				  PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
				  10000);
	  }
	  
	  // now there is no exposure running, do the move
	  devcli_command (telescope, NULL, "change %s %s", arg1, arg2);
	  free (arg2);
	  free (arg1);

	  // increase move count - unblock any COMMAND_WAIT
	  pthread_mutex_lock (&move_count_mutex);
	  move_count++;
	  pthread_cond_broadcast (&move_count_cond);
	  pthread_mutex_unlock (&move_count_mutex);
	  break;
	case COMMAND_WAIT:
	  command++;

	  // if we integrate, wait for integration end..
	  if (exp_state == INTEGRATION_PROGRESS)
	  {
	      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
				  PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
				  10000);
	  }

	  // inform, that we entered wait..
	  pthread_mutex_lock (&script_thread_count_mutex);
	  running_script_count--;
	  pthread_cond_broadcast (&script_thread_count_cond);
	  pthread_mutex_unlock (&script_thread_count_mutex);
	
	  time (&now);
	  timeout.tv_sec = now + 1000;
	  timeout.tv_nsec = 0;

	  // wait for change to complete (can be also 
	  // running_script_count == 1, but that one is be better
	  // approach - no one can quarantee, that move will start
	  // before we will start the exposure
	  pthread_mutex_lock (&move_count_mutex);
	  while (move_count == ((struct ex_info *) exinfo)->move_count)
	    {
	      pthread_cond_timedwait (&move_count_cond, &move_count_mutex, &timeout);
	    }
	  // we moved in that thread..
	  ((struct ex_info *) exinfo)->move_count = move_count;
	  pthread_mutex_unlock (&move_count_mutex);

	  // and get back hold of the running_script_count
	  pthread_mutex_lock (&script_thread_count_mutex);
	  running_script_count++;
	  pthread_mutex_unlock (&script_thread_count_mutex);

	  break;
	default:
	  if (!isspace (*command))
	    fprintf (stderr,
		     "Error in executing - unknow command, script is: '%s'\n",
		     command);
	  command++;
	}
    }
  switch (exp_state)
    {
    case EXPOSURE_BEGIN:
      devcli_command (telescope, NULL, "base_info;info");

      get_info (last, telescope, camera, exposure, NULL);

      devcli_wait_for_status (camera, "img_chip",
			      CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
			      (int) (1.1 * exposure + 10));
    case EXPOSURE_PROGRESS:
      dec_script_thread_count ();
      devcli_command (camera, NULL, "readout 0");
      devcli_wait_for_status (camera, "img_chip",
			      CAM_MASK_READING, CAM_NOTREADING,
			      MAX_READOUT_TIME);
      break;
    case INTEGRATION_PROGRESS:
      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
				  PHOT_MASK_INTEGRATE, PHOT_NOINTEGRATE,
				  10000);
      dec_script_thread_count ();
      break;
    default:
      dec_script_thread_count ();
    }
  free (exinfo);
  return NULL;
}

int
observe (int watch_status)
{
  int i = 0;
  int tar_id = 0;
  int obs_id = -1;
  Target *last, *plan, *p;
  int exposure;
  int light;
  struct thread_list thread_l;
  struct thread_list *tl_top;

  int start_move_count;

  struct tm last_s;

  plan = new Target ();
  plan->next = NULL;
  plan->id = -1;
  i = generate_next (i, plan);
  last = plan->next;
  free (plan);
  plan = last;
  thread_l.next = NULL;
  thread_l.thread = 0;

  devcli_command (telescope, NULL, "info");

  for (; last; last = last->next, p = plan, plan = plan->next, free (p))
    {
      time_t t = time (NULL);
      struct device *camera;

      if (watch_status
	  && (devcli_server ()->statutes[0].status != SERVERD_NIGHT &&
	      devcli_server ()->statutes[0].status != SERVERD_DUSK &&
	      devcli_server ()->statutes[0].status != SERVERD_DAWN))
	break;
      devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			      DEVICE_PRIORITY, 0);
      if (abs (last->ctime - t) > last->tolerance)
	{
	  printf ("ctime %li (%s)", last->ctime, ctime (&last->ctime));
	  printf (" < t %li (%s)", t, ctime (&t));
	  generate_next (i, plan);
	  continue;
	}
      move (last);

      while ((t = time (NULL)) < last->ctime - last->tolerance)
	{
	  time_t sleep_time = last->ctime - last->tolerance;
	  printf ("sleeping for %li sec, till %s\n", sleep_time - t,
		  ctime (&sleep_time));
	  sleep (sleep_time - t);
	}

      gmtime_r (&last_succes, &last_s);

      struct ln_equ_posn object;

      last->getPosition (&object);

      devcli_server_command (NULL,
			     "status_txt P:_%i_obs:_%i_ra:%i_dec:%i_suc:%i:%i",
			     last->id, last->obs_id, (int) object.ra,
			     (int) object.dec, last_s.tm_hour, last_s.tm_min);

      printf ("OK\n");
      time (&t);
      exposure = (last->type == TARGET_FLAT
		  || last->type == TARGET_FLAT_DARK) ? 1 : EXPOSURE_TIME;
      light = !(last->type == TARGET_DARK || last->type == TARGET_FLAT_DARK);

      // wait for thread end..
      for (tl_top = thread_l.next; thread_l.next; tl_top = thread_l.next)
	{
	  pthread_join (tl_top->thread, NULL);
	  thread_l.next = tl_top->next;
	  free (tl_top);
	}

      if (obs_id >= 0 && last->id != tar_id)
	{
	  time (&t);
	  for (camera = devcli_devices (); camera; camera = camera->next)
	    if (camera->type == DEVICE_TYPE_CCD)
	      {
		t = camera->statutes[0].last_update;
		break;
	      }
	  db_end_observation (obs_id, &t);
	  obs_id = -1;
	}

      if (last->type == TARGET_LIGHT)
	{
	  if (last->id != tar_id)
	    {
	      time (&t);
	      tar_id = last->id;
	      db_start_observation (last->id, &t, &obs_id);
	    }
	  last->obs_id = obs_id;
	}

      // wait till end of telescope movement
      devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			      TEL_OBSERVING, 0);

      devcli_wait_for_status_all (DEVICE_TYPE_CCD, "priority",
				  DEVICE_MASK_PRIORITY, DEVICE_PRIORITY, 60);

      // execute exposition scripts
      tl_top = &thread_l;
      thread_l.thread = 0;
      thread_l.next = NULL;

      pthread_mutex_lock (&script_thread_count_mutex);

      running_script_count = 0;

      pthread_mutex_lock (&move_count_mutex);
      start_move_count = move_count;
      pthread_mutex_unlock (&move_count_mutex);

      for (camera = devcli_devices (); camera; camera = camera->next)
	if (camera->type == DEVICE_TYPE_CCD)
	  {
	    struct ex_info *exinfo =
	      (struct ex_info *) malloc (sizeof (struct ex_info));
	    tl_top->next =
	      (struct thread_list *) malloc (sizeof (struct thread_list));
	    tl_top = tl_top->next;
	    tl_top->next = NULL;

	    exinfo->camera = camera;
	    exinfo->last = last;
	    exinfo->move_count = start_move_count;

	    // execute per camera script
	    if (!pthread_create
		(&tl_top->thread, NULL, execute_camera_script,
		 (void *) exinfo))
	      // increase exposure count list
	      script_thread_count++;
	  }
      running_script_count = script_thread_count;
      pthread_mutex_unlock (&script_thread_count_mutex);

      while (!last->next)
	{
	  printf ("Generating next plan: %p %p %p\n", plan, plan->next, last);
	  i = generate_next (i, plan);
	}
      printf ("next plan #%i: id %i type %i\n", i, last->next->id,
	      last->next->type);
      // test, if there is none pending exposure
      pthread_mutex_lock (&script_thread_count_mutex);
      while (script_thread_count > 0)
	{
	  pthread_cond_wait (&script_thread_count_cond,
			     &script_thread_count_mutex);
	}
      pthread_mutex_unlock (&script_thread_count_mutex);
      move (last->next);
    }

  if (obs_id >= 0)
    {
      time_t t;
      time (&t);
      db_end_observation (obs_id, &t);
    }
  return 0;
}


int
main (int argc, char **argv)
{
  uint16_t port = SERVERD_PORT;
  char *server;
  int c = 0;
  int priority = 20;
  struct device *devcli_ser = devcli_server ();
  struct device *phot;

#ifdef DEBUG
  mtrace ();
#endif

  /* get attrs */
  while (1)
    {
      static struct option long_option[] = {
	{
	 "ignore_status", 0, 0, 'i'}
	,
	{
	 "port", 1, 0, 'p'}
	,
	{
	 "priority", 1, 0, 'r'}
	,
	{
	 "help", 0, 0, 'h'}
	,
	{
	 0, 0, 0, 0}
      };
      c = getopt_long (argc, argv, "ip:r:h", long_option, NULL);

      if (c == -1)
	break;

      switch (c)
	{
	case 'i':
	  watch_status = 0;
	  break;
	case 'p':
	  port = atoi (optarg);
	  if (port < 1)
	    {
	      printf ("invalid server port option: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'r':
	  priority = atoi (optarg);
	  if (priority < 1 || priority > 10000)
	    {
	      printf ("invalid priority value: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'h':
	  printf
	    ("Options:\n\tport|p <port_num>\t\tport of the server\n"
	     "\tpriority|r <priority>\t\tpriority to run at\n"
	     "\tignore_status|i\t\t run even when you don't have priority\n");
	  exit (EXIT_SUCCESS);
	case '?':
	  break;
	default:
	  printf ("?? getopt returned unknow character %o ??\n", c);
	}
    }
  if (optind == argc - 1)
    {
      server = argv[optind++];
    }
  else if (optind == argc)
    {
      server = "localhost";
    }
  else
    {
      printf ("**** only legal parameter is server name\n");
      return EXIT_FAILURE;

    }

  printf ("Readign config file" CONFIG_FILE "...");
  if (read_config (CONFIG_FILE) == -1)
    printf ("failed, defaults will be used when apopriate.\n");
  else
    printf ("ok\n");
  observer.lng = get_double_default ("longtitude", 0);
  observer.lat = get_double_default ("latitude", 0);
  printf ("lng: %f lat: %f\n", observer.lng, observer.lat);
  printf ("Current selector is: %i\n",
	  (int) get_double_default ("planc_selector", SELECTOR_HETE));

  printf ("connecting to %s:%i\n", server, port);

  /* connect to the server */
  if (devcli_server_login (server, port, "petr", "petr") < 0)
    {
      perror ("devcli_server_login");
      return EXIT_FAILURE;
    }

  devcli_device_data_handler (DEVICE_TYPE_CCD, data_handler);

  telescope = devcli_find (get_string_default ("telescope_name", "T0"));
  if (!telescope)
    {
      printf
	("**** cannot find telescope!\n**** please check that it's connected and teld runs.\n");
      devcli_server_disconnect ();
      return 0;
    }

  /* connect to db */

  if (c == db_connect ())
    {
      fprintf (stderr, "cannot connect to db, SQL error code: %i\n", c);
      devcli_server_disconnect ();
      return 0;
    }

  if (devcli_command_all (DEVICE_TYPE_CCD, "ready") < 0
      || devcli_command_all (DEVICE_TYPE_CCD, "info") < 0)
    {
      perror ("devcli_write_read_camd camera");
      //return EXIT_FAILURE;
    }
  if (devcli_command (telescope, NULL, "ready;info") < 0)
    {
      perror ("devcli_write_read telescope");
      //return EXIT_FAILURE;
    }

//  devcli_set_general_notifier (camera, status_handler, NULL);

  srandom (time (NULL));

  umask (0x002);

  devcli_server_command (NULL, "priority %i", priority);

  printf ("waiting for priority on telescope");
  fflush (stdout);

  devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);

  printf ("...waiting end\n");

  pthread_create (&image_que_thread, NULL, process_images, NULL);

loop:
  devcli_server_command (NULL, "status_txt P:waiting");
  if (watch_status)
    {
      while (!(devcli_ser->statutes[0].status == SERVERD_NIGHT
	       || devcli_ser->statutes[0].status == SERVERD_DUSK
	       || devcli_ser->statutes[0].status == SERVERD_DAWN))
	{
	  time_t t;
	  time (&t);
	  printf ("waiting for night..%s", ctime (&t));
	  fflush (stdout);
	  if ((t = devcli_command (telescope, NULL, "ready;park")))
	    fprintf (stderr, "Error while moving telescope: %i\n", (int) t);
	  else
	    printf ("PARK ok\n");
//        devcli_wait_for_status (devcli_ser, SERVER_STATUS,
//                                SERVERD_STANDBY_MASK | SERVERD_STATUS_MASK,
//                                SERVERD_NIGHT, 10);
	  fflush (stdout);
	  sleep (100);
	}
    }

  for (phot = devcli_devices (); phot; phot = phot->next)
    if (phot->type == DEVICE_TYPE_PHOT)
    {
       printf ("setting handler: %s\n", phot->name);
       devcli_set_command_handler (phot, (devcli_handle_response_t) phot_handler);
    }

  observe (watch_status);
  printf ("done\n");

  devcli_command (telescope, NULL, "park");

  sleep (300);

  if (watch_status)
    goto loop;

  devcli_server_disconnect ();
  db_disconnect ();

  return 0;
}
