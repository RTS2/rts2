#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define DEFAULT_TEL_CAMERA "C0"

#include "../utils/devcli.h"
#include "target.h"
#include "centering.h"
#include "image_info.h"
#include "../db/db.h"
#include "../utils/config.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int
Target::move ()
{
  if (type == TYPE_CALIBRATION)
    {
      printf ("homing..\n");
      if (devcli_command (telescope, NULL, "home"))
	{
	  printf ("telescope error\n\n--------------------\n");
	  return -1;
	}
      moved = 1;
    }
  else if ((type == TARGET_LIGHT || type == TARGET_FLAT) && moved == 0)
    {
      struct ln_equ_posn object;
      struct ln_hrz_posn hrz;
      double JD;
      getPosition (&object);
      JD = ln_get_julian_from_sys ();
      ln_get_hrz_from_equ (&object, observer, JD, &hrz);
      printf ("Ra: %f Dec: %f\n", object.ra, object.dec);
      printf ("Alt: %f Az: %f\n", hrz.alt, hrz.az);

      tel_target_state = TEL_OBSERVING;

      if (type == TYPE_TERESTIAL)
	{
	  if (devcli_command
	      (telescope, NULL, "fixed %f %f", object.ra, object.dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	      return -1;
	    }
	}
      else
	{
	  if (devcli_command
	      (telescope, NULL, "move %f %f", object.ra, object.dec))
	    {
	      printf ("telescope error\n\n--------------\n");
	      return -1;
	    }
	}
      moved = 1;
    }
  else if (type == TARGET_DARK && moved == 0)
    {
      tel_target_state = TEL_PARKED;

      if (devcli_command (telescope, NULL, "park"))
	{
	  printf ("telescope error\n\n--------------\n");
	  return -1;
	}
      moved = 1;
    }
  return 0;
}

int
Target::get_info (struct device *cam,
		  float exposure, hi_precision_t * img_hi_precision)
{
  struct image_info *info =
    (struct image_info *) malloc (sizeof (struct image_info));
  struct timezone tz;
  int ret;

  info->camera_name = cam->name;
  printf ("info camera_name = %s\n", cam->name);
  info->telescope_name = telescope->name;
  gettimeofday (&info->exposure_tv, &tz);
  info->exposure_length = exposure;
  info->target_id = id;
  info->observation_id = obs_id;
  info->target_type = type;
  if ((ret = devcli_command (cam, NULL, "info")))
    {
      printf ("camera info error\n");
    }
  memcpy (&info->camera, &cam->info, sizeof (struct camera_info));
  memcpy (&info->telescope, &telescope->info, sizeof (struct telescope_info));
  if (!img_hi_precision && hi_precision == 3
      && !strcmp (cam->name,
		  get_string_default ("telescope_camera", DEFAULT_TEL_CAMERA))
      && type == TARGET_LIGHT)
    {
      closed_loop_precission.hi_precision = 1;
      info->hi_precision = &closed_loop_precission;
    }
  else
    {
      info->hi_precision = img_hi_precision;
    }
  if (info->hi_precision)
    {
      info->hi_precision->processed = 0;
    }
  devcli_image_info (cam, info);
  free (info);
  return ret;
}


int
Target::dec_script_thread_count (void)
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

int
Target::join_script_thread ()
{
  struct thread_list *tl_top;
  struct device *camera;
  time_t t;
  for (tl_top = thread_l.next; thread_l.next; tl_top = thread_l.next)
    {
      pthread_join (tl_top->thread, NULL);
      thread_l.next = tl_top->next;
      free (tl_top);
    }

  if (obs_id >= 0)
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
}

int
Target::observe (Target * last_t)
{
  float exposure;
  int light;
  int start_move_count;
  struct thread_list *tl_top;
  struct device *camera;
  time_t t;

  time (&t);

  devcli_wait_for_status (telescope, "priority", DEVICE_MASK_PRIORITY,
			  DEVICE_PRIORITY, 0);
  // wait for readouts..
  if (last_t)
    {
      last_t->wait_for_readout_end ();
    }

  move ();

  if (abs (start_time - t) > tolerance)
    {
      printf ("start_time %li (%s)", start_time, ctime (&start_time));
      printf (" < t %li (%s)", t, ctime (&t));
      return 0;
    }

  while ((t = time (NULL)) < start_time - tolerance)
    {
      time_t sleep_time = start_time - tolerance;
      printf ("sleeping for %li sec, till %s\n", sleep_time - t,
	      ctime (&sleep_time));
      sleep (sleep_time - t);
    }

  struct ln_equ_posn object;

  getPosition (&object);

  devcli_server_command (NULL,
			 "status_txt P:_%i_obs:_%i_ra:%i_dec:%i",
			 id, obs_id, (int) object.ra, (int) object.dec);

  printf ("OK\n");
  time (&t);
  exposure = (type == TARGET_FLAT
	      || type == TARGET_FLAT_DARK) ? 1 : EXPOSURE_TIME;
  light = !(type == TARGET_DARK || type == TARGET_FLAT_DARK);

  if (last_t)
    {
      // wait for thread end..
      last_t->join_script_thread ();
    }

  if (type == TARGET_LIGHT)
    {
      time (&t);
      db_start_observation (id, &t, &obs_id);
    }

  // wait till end of telescope movement
  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			  tel_target_state, 120);

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

  acquire ();

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
	exinfo->last = this;
	exinfo->move_count = start_move_count;

	// execute per camera script
	printf ("ex: %p %p %p %p\n", exinfo, exinfo->last, this,
		this->telescope);
	if (!pthread_create (&tl_top->thread, NULL, runStart, exinfo))
	  // increase exposure count list
	  script_thread_count++;
      }
  running_script_count = script_thread_count;
  pthread_mutex_unlock (&script_thread_count_mutex);
}

/**
 * Return script for camera exposure.
 *
 * @param last		observing target
 * @param camera	camera device for script
 * @param buf		buffer for script
 *
 * @return 0 on success, < 0 on error
 */
int
Target::get_script (struct device *camera, char *buf)
{
  char obs_type_str[2];
  char *s;
  int ret;
  obs_type_str[0] = obs_type;
  obs_type_str[1] = 0;

  ret = db_get_script (id, camera->name, buf);
  if (!ret)
    return 0;

  s = get_sub_device_string_default (camera->name, "script", obs_type_str,
				     "E D");
  strncpy (buf, s, MAX_COMMAND_LENGTH);
  return 0;
}

/**
 * Move to designated target, get astrometry, proof that target was acquired with given margin.
 *
 * @return 0 if I can observe futher, -1 if observation was canceled
 */
int
Target::acquire ()
{
  float exposure = 10;
  struct timespec timeout;
  struct device *camera;
  time_t now;
  int bin;
  double ra_err = NAN, dec_err = NAN;	// no astrometry
  double ra_margin = 15, dec_margin = 15;	// in arc minutes
  int max_tries = (int) get_double_default ("maxtries", 5);
  if (hi_precision == 0 || hi_precision == 3)
    return 0;

  camera =
    devcli_find (get_string_default ("telescope_camera", DEFAULT_TEL_CAMERA));

  if (!camera)
    return 0;

  exposure =
    get_device_double_default (camera->name, "precission_exposure", exposure);

  bin = (int) get_double_default ("precission_binning", 1);

  hi_precision_t img_hi_precision;

  img_hi_precision.ntries = 0;

  ra_margin = get_double_default ("ra_margin", ra_margin);
  dec_margin = get_double_default ("dec_margin", dec_margin);
  ra_margin = ra_margin / 60.0;	// margin is in minutes, we need degree!!
  dec_margin = dec_margin / 60.0;

  pthread_mutex_t ret_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t ret_cond = PTHREAD_COND_INITIALIZER;
  img_hi_precision.mutex = &ret_mutex;
  img_hi_precision.cond = &ret_cond;
  if (hi_precision == 3)
    {
      img_hi_precision.hi_precision = 1;
    }
  else
    {
      img_hi_precision.hi_precision = hi_precision;
    }

  if (bin > 1)			// change binning
    {
      devcli_command (camera, NULL, "binning 0 %i %i", bin, bin);
    }

  while (img_hi_precision.ntries < max_tries)
    {
      int light = 1;
      struct ln_equ_posn object;
      if (((struct camera_info *) (&(camera->info)))->can_df)
	{
	  light = (precission_count % 10 == 0) ? 0 : 1;
	}
      if (!light)
	type = TARGET_DARK;
      else
	type = TARGET_LIGHT;
      getPosition (&object);
      printf
	("triing to get to %f %f, %i try, %s image, %i total precission, can_df: %i\n",
	 object.ra, object.dec, img_hi_precision.ntries,
	 (light == 1) ? "light" : "dark", precission_count,
	 ((struct camera_info *) (&(camera->info)))->can_df);

      precission_count++;

      fflush (stdout);
      // move is at the end & we not move for dark images, so we shall
      // not get to parked state
      devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			      tel_target_state, 120);

      devcli_command (camera, NULL, "base_info");
      devcli_command (camera, NULL, "expose 0 %i %f", light, exposure);
      devcli_command (telescope, NULL, "base_info");
      devcli_command (telescope, NULL, "info");

      get_info (camera, exposure, &img_hi_precision);

      devcli_wait_for_status (camera, "img_chip",
			      CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
			      (long int) (1.1 * exposure + EXPOSURE_TIMEOUT));
      devcli_command (camera, NULL, "readout 0");
      devcli_wait_for_status (camera, "img_chip",
			      CAM_MASK_READING, CAM_NOTREADING,
			      MAX_READOUT_TIME);
      if (light)
	{
	  // after finishing astrometry, I'll have telescope true location
	  // in location parameters, passed as reference to get_info.
	  // So I can compare that with required ones
	  pthread_mutex_lock (img_hi_precision.mutex);
	  time (&now);
	  timeout.tv_sec = now + 350;
	  timeout.tv_nsec = 0;
	  pthread_cond_timedwait (img_hi_precision.cond,
				  img_hi_precision.mutex, &timeout);
	  printf
	    ("img_hi_precision->image_pos->ra: %f dec: %f img_hi_precision.hi_precision %i\n",
	     img_hi_precision.image_pos.ra, img_hi_precision.image_pos.dec,
	     img_hi_precision.hi_precision);
	  pthread_mutex_unlock (img_hi_precision.mutex);
	  fflush (stdout);
	  if (!isnan (img_hi_precision.image_pos.ra)
	      && !isnan (img_hi_precision.image_pos.dec))
	    {
	      switch (img_hi_precision.hi_precision)
		{
		case 1:
		  ra_err =
		    ln_range_degrees (object.ra -
				      img_hi_precision.image_pos.ra);
		  dec_err =
		    ln_range_degrees (object.dec -
				      img_hi_precision.image_pos.dec);
		  break;
		case 2:
		  ra_err = (fabs (img_hi_precision.image_pos.ra)) / 60.0;
		  dec_err = (fabs (img_hi_precision.image_pos.dec)) / 60.0;
		  break;
		}
	    }
	  else
	    {
	      // not astrometry, get us out of here
	      img_hi_precision.ntries = max_tries;
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
	      moved = 0;
	      // try to synchro the scope again..only if not dark
	      // image
	      if (img_hi_precision.hi_precision == 1 && type == TARGET_LIGHT)
		move ();
	    }
	  img_hi_precision.ntries++;
	}
    }
  if (bin > 1)
    {
      devcli_command (camera, NULL, "binning 0 1 1");
    }
  if (hi_precision == 2)
    {
      devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
			      tel_target_state, 300);
    }
  // try centering, if required
  if (img_hi_precision.ntries < max_tries
      && get_string_default ("centering_phot", NULL))
    {
      int ret;
      ret = rts2_centering (camera, telescope);
      if (ret)
	{
	  img_hi_precision.ntries = max_tries;
	}
    }
  hi_precision = (img_hi_precision.ntries < max_tries) ? 0 : -1;	// return something usefull - tar is pointer, so it will get there
  return hi_precision;
}

int
Target::run ()
{

}

int
Target::postprocess ()
{

}

void *
Target::runStart (void *thisp)
{
  struct ex_info *exinfo = (struct ex_info *) thisp;
  printf ("ex: %p %p %p\n", exinfo, exinfo->last, exinfo->last->telescope);
  exinfo->last->runScript (exinfo);
}

int
Target::runScript (struct ex_info *exinfo)
{
  struct device *camera = exinfo->camera;
  int light;
  char com_buf[MAX_COMMAND_LENGTH], *command, *s;
  char *arg1, *arg2;
  float exposure;
  int filter, count;
  char mirror_pos;
  int ret;
  time_t t;
  time_t script_start;
  char s_time[27];
  struct timespec timeout;
  time_t now;
  enum
  { NO_EXPOSURE, EXPOSURE_BEGIN, EXPOSURE_PROGRESS,
    INTEGRATION_PROGRESS, CENTERING
  } exp_state;
  time (&script_start);
  switch (type)
    {
    case TARGET_LIGHT:
      get_script (camera, com_buf);
      command = com_buf;
      light = 1;
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
      printf ("Unknow target type: %i\n", type);
      command = "E 1";
      light = 0;
    }

  exp_state = NO_EXPOSURE;
  while (*command)
    {
      if ((exp_state == EXPOSURE_BEGIN
	   || exp_state == INTEGRATION_PROGRESS) && *command
	  && !isspace (*command))
	{
	  // wait till exposure end..
	  devcli_command (telescope, NULL, "base_info");
	  devcli_command (telescope, NULL, "info");
	  get_info (camera, exposure, NULL);
	  if (exp_state == EXPOSURE_BEGIN)
	    {
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_EXPOSE, CAM_NOEXPOSURE,
				      (int) (1.1 * exposure +
					     EXPOSURE_TIMEOUT));
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
	  devcli_wait_for_status (telescope, "telescope",
				  TEL_MASK_MOVING, tel_target_state, 300);
	  devcli_command (camera, &ret, "base_info");
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
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	    }
	  devcli_command (camera, &ret, "filter %i", filter);
	  if (ret)
	    fprintf (stderr, "error executing 'filter %i'", filter);
	  exp_state = NO_EXPOSURE;
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
	  printf ("integration exp_stat: %i\n", exp_state);
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	    }

	  devcli_wait_for_status (telescope, "telescope", TEL_MASK_MOVING,
				  tel_target_state, 300);
	  devcli_command_all (DEVICE_TYPE_PHOT, "filter %i", filter);
	  devcli_command_all (DEVICE_TYPE_PHOT, "integrate %f %i", exposure,
			      count);
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
	      command++;
	    }
	  arg2 = (char *) malloc (command - s + 1);
	  strncpy (arg2, s, command - s + 1);
	  // wait till exposure count reaches 0 - no other thread do
	  // the exposure..
	  time (&now);
	  timeout.tv_sec = now + 1000;
	  timeout.tv_nsec = 0;
	  pthread_mutex_lock (&script_thread_count_mutex);
	  while (running_script_count > 1)	// cause we will hold running_script_count lock in any case..
	    {
	      pthread_cond_timedwait (&script_thread_count_cond,
				      &script_thread_count_mutex, &timeout);
	    }
	  pthread_mutex_unlock (&script_thread_count_mutex);
	  // we cannot imagine anything, 
	  // because there is forced readout before every command,
	  // just in case we integrate, let's finish
	  if (exp_state == INTEGRATION_PROGRESS)
	    {
	      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
					  PHOT_MASK_INTEGRATE,
					  PHOT_NOINTEGRATE, 100);
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
					  PHOT_MASK_INTEGRATE,
					  PHOT_NOINTEGRATE, 10000);
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
	      pthread_cond_timedwait (&move_count_cond, &move_count_mutex,
				      &timeout);
	    }
	  // we moved in that thread..
	  ((struct ex_info *) exinfo)->move_count = move_count;
	  pthread_mutex_unlock (&move_count_mutex);
	  // and get back hold of the running_script_count
	  pthread_mutex_lock (&script_thread_count_mutex);
	  running_script_count++;
	  pthread_mutex_unlock (&script_thread_count_mutex);
	  break;
	case COMMAND_CENTERING:
	  command++;
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	      exp_state = NO_EXPOSURE;
	    }
	  if (exp_state == INTEGRATION_PROGRESS)
	    {
	      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
					  PHOT_MASK_INTEGRATE,
					  PHOT_NOINTEGRATE, 10000);
	    }
	  exp_state = CENTERING;
	  rts2_centering (camera, telescope);
	  break;
	case COMMAND_MIRROR:
	  command++;
	  mirror_pos = *command;
	  if (!(mirror_pos == 'A' || mirror_pos == 'B'))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	      exp_state = NO_EXPOSURE;
	    }
	  if (exp_state == INTEGRATION_PROGRESS)
	    {
	      devcli_wait_for_status_all (DEVICE_TYPE_PHOT, "phot",
					  PHOT_MASK_INTEGRATE,
					  PHOT_NOINTEGRATE, 100);
	    }
	  devcli_command_all (DEVICE_TYPE_MIRROR, "set %c", mirror_pos);
	  devcli_wait_for_status_all (DEVICE_TYPE_MIRROR, "mirror",
				      MIRROR_MASK_MOVE, MIRROR_NOTMOVE, 30);
	  break;
	case COMMAND_SLEEP:
	  command++;
	  filter = strtol (command, &s, 10);
	  if (s == command || (!isspace (*s) && *s))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	      exp_state = NO_EXPOSURE;
	    }

	  time (&t);
	  while (t - script_start < filter)
	    {
	      int sl;
	      sl = filter - (t - script_start);
	      printf ("sleeping for %i seconds\n", sl);
	      sleep (sl);
	      time (&t);
	    }

	  break;
	case COMMAND_UTC_SLEEP:
	  command++;
	  filter = strtol (command, &s, 10);
	  if (s == command || (!isspace (*s) && *s))
	    {
	      fprintf (stderr, "invalid arg, expecting int, get %s\n", s);
	      command = s;
	      continue;
	    }
	  command = s;
	  if (exp_state == EXPOSURE_PROGRESS)
	    {
	      devcli_command (camera, NULL, "readout 0");
	      devcli_wait_for_status (camera, "img_chip",
				      CAM_MASK_READING, CAM_NOTREADING,
				      MAX_READOUT_TIME);
	      exp_state = NO_EXPOSURE;
	    }

	  time (&t);
	  while (t - script_start < filter)
	    {
	      int sl;
	      sl = filter - (t - script_start);
	      printf ("sleeping for %i seconds\n", sl);
	      sleep (sl);
	      time (&t);
	    }

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
      devcli_command (telescope, NULL, "base_info");
      devcli_command (telescope, NULL, "info");
      get_info (camera, exposure, NULL);
      devcli_wait_for_status (camera, "img_chip", CAM_MASK_EXPOSE,
			      CAM_NOEXPOSURE,
			      (int) (1.1 * exposure + EXPOSURE_TIMEOUT));
    case EXPOSURE_PROGRESS:
      dec_script_thread_count ();
      devcli_command (camera, NULL, "readout 0");
      devcli_wait_for_status (camera, "img_chip", CAM_MASK_READING,
			      CAM_NOTREADING, MAX_READOUT_TIME);
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
  return 0;
}

int
TargetFocusing::get_script (struct device *camera, char *buf)
{
  buf[0] = COMMAND_FOCUSING;
  buf[1] = 0;
  return 0;
}
