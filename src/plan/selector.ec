/*!
 * @file Selector - build plan from target database.
 *
 * @author petr
 */

#include "selector.h"
#include "image_info.h"
#include "../db/db.h"
#include "status.h"
#include "../utils/config.h"

#include <malloc.h>
#include <libnova.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define READOUT_TIME		75
#define PLAN_TOLERANCE		180
#define PLAN_DARK_TOLERANCE	1800

#define DEFAULT_DARK_FREQUENCY	50	// every n image will be dark

EXEC SQL include sqlca;

int
find_plan (struct target *plan, int id, time_t c_start)
{
  while (plan)
    {
      if (plan->id == id && plan->ctime > c_start)
	return 0;
      plan = plan->next;
    }

  return -1;
}

struct target *
add_target (struct target *plan, int type, int id, int obs_id, double ra,
	    double dec, time_t obs_time, int tolerance)
{
  struct target *new_plan;
  new_plan = (struct target *) malloc (sizeof (struct target));
  new_plan->next = NULL;
  new_plan->type = type;
  new_plan->id = id;
  new_plan->obs_id = obs_id;
  new_plan->ra = ra;
  new_plan->dec = dec;
  new_plan->ctime = obs_time;
  new_plan->tolerance = tolerance;
  new_plan->moved = 0;

  while (plan->next)
    plan = plan->next;
  plan->next = new_plan;
  return new_plan;
}

int
select_next_alt (time_t c_start, struct target *plan, float lon, float lat)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
  double st;
  int tar_id;
  double dec;
  double ra;
  double alt;
  float db_lon = lon;
  float db_lat = lat;
  long int obs_start = c_start - 8640;
  EXEC SQL END DECLARE SECTION;

  printf ("C_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\n", st);

  db_lock ();
  EXEC SQL DECLARE obs_cursor_alt CURSOR FOR
    SELECT tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:st,:db_lon,:db_lat) AS alt FROM targets
    WHERE (tar_lastobs is NULL) or tar_lastobs < abstime (:obs_start)
    ORDER BY alt DESC;
  EXEC SQL OPEN obs_cursor_alt;
  test_sql;
  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_alt INTO:tar_id,:ra,:dec,:alt;
      if (sqlca.sqlcode)
	break;
      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\n", tar_id, ra, dec, alt);
      if (find_plan (plan, tar_id, obs_start))
	{
	  printf ("find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);
	  break;
	}
    }

  EXEC SQL CLOSE obs_cursor_alt;
  db_unlock ();
#undef test_sql
  return 0;
err:
  db_unlock ();
#ifdef DEBUG
  printf ("err select_next_alt: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_gps (time_t c_start, struct target *plan, float lon, float lat)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
  double st;
  int tar_id;
  double dec;
  double ra;
  double alt;
  float db_lon = lon;
  float db_lat = lat;
  long int obs_start = c_start;
  EXEC SQL END DECLARE SECTION;
  obs_start -= get_device_double_default ("gps", "interval", 302400);

  printf ("C_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\n", st);

  db_lock ();
  EXEC SQL DECLARE obs_cursor_gps CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:st,:db_lon,:db_lat) AS alt
    FROM targets
    WHERE targets.type_id =
    'P' AND NOT EXISTS (SELECT * FROM observations WHERE observations.tar_id =
			targets.tar_id AND observations.
			obs_start >:obs_start) AND obj_alt (tar_ra,
							    tar_dec,:st,:db_lon,:db_lat)
    > 10 ORDER BY tar_dec ASC, alt DESC;
  EXEC SQL OPEN obs_cursor_gps;
  test_sql;
  printf ("\ttar_id\tra\tdec\talt\n");
  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_gps INTO:tar_id,:ra,:dec,:alt;
      if (sqlca.sqlcode)
	break;
      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\n", tar_id, ra, dec, alt);
      if (find_plan (plan, tar_id, obs_start))
	{
	  printf ("find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);
	  EXEC SQL CLOSE obs_cursor_gps;
	  db_unlock ();
	  return 0;
	}
    }
  EXEC SQL CLOSE obs_cursor_gps;
  db_unlock ();
#undef test_sql
  return -1;
err:
  db_unlock ();
#ifdef DEBUG
  printf ("err select_next_gps: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_airmass (time_t c_start, struct target *plan,
		     float target_airmass, float az_end, float az_start,
		     float lon, float lat)
{
#define test_sql if (sqlca.sqlcode != 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
  double st;
  int tar_id;
  double dec;
  double ra;
  float az;
  float airmass;
  float d_az_start = az_start;
  float d_az_end = az_end;
  float t_airmass = target_airmass;
  float db_lon = lon;
  float db_lat = lat;
  int img_count;
  EXEC SQL END DECLARE SECTION;

  db_lock ();
  printf ("c_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\nairmass: %f\n", st, t_airmass);
  EXEC SQL DECLARE obs_cursor_airmass CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_az (tar_ra, tar_dec,:st,:db_lon,:db_lat) AS az,
    obj_airmass (tar_ra, tar_dec,:st,:db_lon,:db_lat) AS
    airmass, img_count
    FROM targets, targets_images WHERE targets.tar_id =
    targets_images.tar_id AND targets.type_id = 'S'
    AND (abs (obj_airmass (tar_ra, tar_dec,:st,:db_lon,:db_lat) -:t_airmass))
    <
    0.2 AND (obj_az (tar_ra, tar_dec,:st,:db_lon,:db_lat) <:d_az_end OR
	     obj_az (tar_ra,
		     tar_dec,:st,:db_lon,:db_lat) >:d_az_start) ORDER BY
    img_count ASC;
  EXEC SQL OPEN obs_cursor_airmass;

  test_sql;
  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_airmass
	INTO:tar_id,:ra,:dec,:az,:airmass,:img_count;
      test_sql;
      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\t%+03.3f\t%5i\n", tar_id, ra,
	      dec, az, airmass, img_count);

      if (find_plan (plan, tar_id, c_start - 1800))
	{
	  printf ("airmass find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);
	  break;
	}
    }

  EXEC SQL CLOSE obs_cursor_airmass;
  test_sql;
  db_unlock ();
#undef test_sql
  return 0;
err:
#ifdef DEBUG
  printf ("err select_next_airmass: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
  return -1;
}

int
select_next_grb (time_t c_start, struct target *plan, float lon, float lat)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
  double st;
  int tar_id;
  double dec;
  double ra;
  float db_lon = lon;
  float db_lat = lat;
  float alt;
  long int obs_start = c_start - 200;
  EXEC SQL END DECLARE SECTION;

  db_lock ();
  printf ("C_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\n", st);
  EXEC SQL DECLARE obs_cursor_grb CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:st,:db_lon,:db_lat) AS alt FROM targets, grb
    WHERE type_id =
    'G' AND grb_id > 100 AND obj_alt (tar_ra, tar_dec,:st,:db_lon,:db_lat)
    > 0 and targets.tar_id =
    grb.tar_id and grb_last_update >
    abstime (:obs_start - 200000) ORDER BY alt DESC;
  EXEC SQL OPEN obs_cursor_grb;
  test_sql;
  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_grb INTO:tar_id,:ra,:dec,:alt;
      if (sqlca.sqlcode)
	break;
      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\n", tar_id, ra, dec, alt);
      if (find_plan (plan, tar_id, obs_start))
	{
	  printf ("grb find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);
	  EXEC SQL CLOSE obs_cursor_grb;
	  test_sql;
	  db_unlock ();
	  return 0;
	}
    }
  EXEC SQL CLOSE obs_cursor_grb;
  test_sql;
  printf ("no grb found");
  db_unlock ();
#undef test_sql
  return -1;
err:
  db_unlock ();
#ifdef DEBUG
  printf ("err select_next_grb: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}


/*!
 * Select next observation from list of opportunity fields.
 */
int
select_next_to (time_t c_start, struct target *plan, float az_end,
		float az_start, float lon, float lat)
{
  EXEC SQL BEGIN DECLARE SECTION;
  double st;
  int tar_id;
  double ra;
  double dec;
  float alt;
  int ot_imgcount;
  int ot_minpause;
  int ot_isnull;
  float d_az_end = az_end;
  float d_az_start = az_start;
  float db_lon = lon;
  float db_lat = lat;
  EXEC SQL END DECLARE SECTION;

  db_lock ();
#define test_sql if (sqlca.sqlcode < 0) goto err
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("to st: %f\n", st);
  EXEC SQL DECLARE obs_cursor_to CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec, obj_alt (tar_ra,
						     tar_dec,:st,:db_lon,:db_lat)
    AS alt, ot_imgcount, EXTRACT (EPOCH FROM ot_minpause) FROM targets,
    targets_images, ot WHERE ot.tar_id = targets.tar_id AND targets.tar_id =
    targets_images.tar_id AND type_id = 'O' AND obj_alt (tar_ra, tar_dec,:st,
							 :db_lon,:db_lat) >
    10 AND (obj_az (tar_ra, tar_dec,:st,:db_lon,:db_lat) <:d_az_end OR
	    obj_az (tar_ra, tar_dec,:st,:db_lon,
		    :db_lat) >:d_az_start) ORDER BY ot_priority DESC,
    img_count ASC, alt DESC, ot_imgcount DESC;

  EXEC SQL OPEN obs_cursor_to;
  test_sql;
  while (1)
    {
      int last_o;
      EXEC SQL FETCH next FROM obs_cursor_to
	INTO:tar_id,:ra,:dec,:alt,:ot_imgcount,:ot_minpause:ot_isnull;
      if (sqlca.sqlcode)
	goto err;
      if (ot_isnull)
	ot_minpause = 0;
      last_o = db_last_observation (tar_id);
      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\t%i\t%i\n", tar_id, ra, dec,
	      alt, ot_imgcount, ot_minpause);
      if (db_last_night_images_count (tar_id) < ot_imgcount
	  && (last_o == -1 || last_o >= ot_minpause))
	{
	  printf ("to find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);
	  EXEC SQL CLOSE obs_cursor_to;
	  db_unlock ();
	  return 0;
	}
    }
  EXEC SQL CLOSE obs_cursor_to;
  test_sql;
  db_unlock ();
#undef test_sql
  return -1;
err:
  db_unlock ();
#ifdef DEBUG
  printf ("err select_next_to: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

/*!
 * If needed, select next observation from list of photometry fields
 */
int
select_next_photometry (time_t c_start, struct target *plan, float lon,
			float lat)
{
  EXEC SQL BEGIN DECLARE SECTION;
  EXEC SQL END DECLARE SECTION;
  return 0;
#define test_sql if (sqlca.sqlcode < 0) goto err
err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

/*! 
 * HETE field generation
 */
int
hete_mosaic (struct target *plan, double jd, time_t * obs_start, int number)
{
  struct ln_equ_posn sun;
  int frequency;
  frequency = (int) get_device_double_default ("hete", "frequency", 1);
  if (frequency <= 1)
    // check for darks
    if ((number %
	 (int) get_device_double_default ("hete", "dark_frequency",
					  DEFAULT_DARK_FREQUENCY)) == 1)
      {
	add_target (plan, TARGET_DARK, -1, -1, 0, 0, *obs_start,
		    PLAN_DARK_TOLERANCE);
	return 0;
      }
  if ((number % frequency) == 0)
    {
      int step = (number / frequency) % 4;
      get_equ_solar_coords (jd, &sun);
      sun.ra = range_degrees (sun.ra - 180 - 12.5 + 25.0 * (step > 1));
      sun.dec = (-sun.dec) - (10.0 / 2) + 10 * (step % 2);
      add_target (plan, TARGET_LIGHT, 50 + step, -1, sun.ra, sun.dec,
		  *obs_start, PLAN_TOLERANCE);
      return 0;
    }				//continue to airmass, if not anti-solar hete
  return -1;
}

/*!
 * Select flat field position
 */
int
flat_field (struct target *plan, time_t * obs_start, int number, float lon,
	    float lat)
{
  double jd;

  struct ln_equ_posn sun;
  struct ln_lnlat_posn observer;
  struct ln_hrz_posn sun_az;
  double sun_alt;

  observer.lng = lon;
  observer.lat = lat;

  jd = get_julian_from_timet (obs_start);

  get_equ_solar_coords (jd, &sun);
  sun.ra = range_degrees (sun.ra);
  get_hrz_from_equ (&sun, &observer, jd, &sun_az);
  sun_az.az = range_degrees (sun_az.az + 180.0 - 3 + 2 * (number % 4));
  sun_alt = sun_az.alt;
  sun_az.alt = 45 - 3 + 2 * ((number + 2) % 4);
  // ra + dec of antisun..
  get_equ_from_hrz (&sun_az, &observer, jd, &sun);
  add_target (plan,
	      (sun_alt >
	       get_double_default ("dark_horizont",
				   -2)) ? TARGET_FLAT_DARK : TARGET_FLAT, 10,
	      -1, sun.ra, sun.dec, *obs_start, PLAN_TOLERANCE);
  return 0;
}

/*! 
 * Makes one observation plan entry.
 *
 * Please note, that you are responsible to add plan on top of curr_plan, 
 * if you like it.
 * 
 * @param plan		target plan
 * @param sel_type	airmass or height?
 * @param curr_plan	plan up to date; when NULL, then it's not used
 * @param c_start	starting time
 * @param number	plan number
 * @param state		serverd status
 */
int
get_next_plan (struct target *plan, int selector_type,
	       time_t * obs_start, int number, float exposure, int state,
	       float lon, float lat)
{
  float az;
  float airmass;
  double jd;
  struct ln_equ_posn moon;
  struct ln_hrz_posn moon_hrz;
  struct ln_lnlat_posn observer;
  int dark_frequency;

  int last_good_img;

  printf ("lon: %f lat: %f\n", lon, lat);

  jd = get_julian_from_timet (obs_start);
  dark_frequency =
    get_double_default ("dark_frequency", DEFAULT_DARK_FREQUENCY);

  // check for GRB..
  if (!select_next_grb (*obs_start, plan, lon, lat))
    return 0;

  if (state != SERVERD_NIGHT)
    {
      if (!flat_field (plan, obs_start, number, lon, lat))
	return 0;
    }

  // check for OT
  last_good_img = db_last_good_image ("C0");
  printf ("last good image on C0: %i\n", last_good_img);
  if (last_good_img >= 0 && last_good_img < 3600)
    {
      printf ("Trying OT\n");
      if (!select_next_to (*obs_start, plan, 120, 230, lon, lat))
	return 0;
    }

  switch (selector_type)
    {

    case SELECTOR_ALTITUDE:
      return select_next_alt (*obs_start, plan, lon, lat);

    case SELECTOR_GPS:
      // every 50 image will be dark..
      if ((number % (int) get_device_double_default ("gps", "dark_frequency", dark_frequency)) == 1)	// because of HETE 
	{
	  add_target (plan, TARGET_DARK, -1, -1, 0, 0, *obs_start,
		      PLAN_DARK_TOLERANCE);
	  return 0;
	}

      if (number % (int) get_device_double_default ("gps", "frequency", 2)
	  && !select_next_gps (*obs_start, plan, lon, lat))
	return 0;

    case SELECTOR_HETE:
      if (!hete_mosaic (plan, jd, obs_start, number))
	return 0;

    case SELECTOR_AIRMASS:
      // every 50 image will be dark..
      if (number % dark_frequency == 1)	// because of HETE 
	{
	  add_target (plan, TARGET_DARK, -1, -1, 0, 0, *obs_start,
		      PLAN_DARK_TOLERANCE);
	  return 0;
	}

      observer.lng = lon;
      observer.lat = lat;

      get_lunar_equ_coords (jd, &moon, 0.01);
      get_hrz_from_equ (&moon, &observer, jd, &moon_hrz);

      switch (number & 1)
	{
	case 0:
	  while (1)
	    {
	      az = 180.0 * rand () / (RAND_MAX + 1.0);
	      airmass = 1.2 + (1.2 * rand () / (RAND_MAX + 1.0));
	      if (az > 90)
		az += 180;
	      if (moon_hrz.alt < -10)
		break;
	      else if (abs ((int) floor (moon_hrz.az - az) % 360) < 40)
		printf
		  (" skipping az: %f airmass: %f moon_az: %f moon_alt: %f\n",
		   az, airmass, moon_hrz.az, moon_hrz.alt);
	      else
		break;
	    }
	  break;

	case 1:
	  while (1)
	    {
	      az = 360.0 * rand () / (RAND_MAX + 1.0);
	      airmass = 1.0 + (0.2 * rand () / (RAND_MAX + 1.0));

	      if (moon_hrz.alt < -10)
		break;
	      else if (abs ((int) floor (moon_hrz.az - az) % 360) < 40)
		printf
		  (" skipping az: %f airmass: %f moon_az: %f moon_alt: %f\n",
		   az, airmass, moon_hrz.az, moon_hrz.alt);
	      else
		break;
	    }
	  break;
	}
      printf ("executing select_next_airmass\n");
      return select_next_airmass (*obs_start, plan, airmass, 90, 270, lon,
				  lat);

    default:
      return -1;
    }
}

int
make_plan (struct target **plan, float exposure, float lon, float lat)
{
  struct target *last;
  time_t c_start;
  int i;

  //ECPGdebug (1, stdout);

  c_start = time (NULL);

  *plan = (struct target *) malloc (sizeof (struct target));
  (*plan)->type = TARGET_LIGHT;
  (*plan)->id = -1;
  (*plan)->next = NULL;
  last = *plan;

  for (i = 0; i < 10; i++)
    {
      get_next_plan (*plan,
		     (int) get_double_default ("planc_selector",
					       SELECTOR_HETE), &c_start, i,
		     exposure, SERVERD_NIGHT, lon, lat);
      c_start += READOUT_TIME + exposure;
    }

  last = *plan;
  *plan = (*plan)->next;
  free (last);

  return 0;
}

void
free_plan (struct target *plan)
{
  struct target *last;

  last = plan->next;
  free (plan);

  for (; last; plan = last, last = last->next, free (plan));
}
