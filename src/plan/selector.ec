/*!
 * @file Selector - build plan from target database.
 *
 * @author petr
 */

#include <malloc.h>
#include <libnova.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "selector.h"
#include "image_info.h"

#define READOUT_TIME		75
#define PLAN_TOLERANCE		180
#define PLAN_DARK_TOLERANCE	1800

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

  while (plan->next)
    plan = plan->next;
  plan->next = new_plan;
  return new_plan;
}

int
select_next_alt (time_t c_start, struct target *plan)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;

  double st;

  int tar_id;

  double dec;
  double ra;

  double alt;

  long int obs_start = c_start - 8640;

  EXEC SQL END DECLARE SECTION;

  printf ("C_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\n", st);

  EXEC SQL DECLARE obs_cursor_alt CURSOR FOR
    SELECT tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:st, -14.95, 50) AS alt FROM targets
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

      if (find_plan (plan, tar_id, c_start - 1800))
	{
	  printf ("find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);
	  break;
	}
    }

  EXEC SQL CLOSE obs_cursor_alt;

  test_sql;

#undef test_sql

  return 0;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_airmass (time_t c_start, struct target *plan,
		     double target_airmass, double az_end, double az_start)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;

  double st;

  int tar_id;

  double dec;
  double ra;

  double az;
  double airmass;

  double d_az_start = az_start;
  double d_az_end = az_end;
  double t_airmass = target_airmass;

  int img_count;

  EXEC SQL END DECLARE SECTION;

  printf ("c_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\nairmass: %f", st, t_airmass);

  EXEC SQL DECLARE obs_cursor_airmass CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_az (tar_ra, tar_dec,:st, -14.95, 50) AS az,
    obj_airmass (tar_ra, tar_dec,:st, -14.95, 50) AS
    airmass, img_count
    FROM targets, targets_images WHERE targets.tar_id =
    targets_images.tar_id AND targets.type_id = 'S'
    AND (abs (obj_airmass (tar_ra, tar_dec,:st, -14.95, 50) -:t_airmass)) <
    0.2 AND (obj_az (tar_ra, tar_dec,:st, -14.95, 50) <:d_az_end OR
	     obj_az (tar_ra, tar_dec,:st, -14.95,
		     50) >:d_az_start) ORDER BY img_count ASC;
  EXEC SQL OPEN obs_cursor_airmass;

  test_sql;

  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_airmass
	INTO:tar_id,:ra,:dec,:az,:airmass,:img_count;

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

#undef test_sql

  return 0;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_grb (time_t c_start, struct target *plan)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;

  double st;

  int tar_id;

  double dec;
  double ra;

  double alt;

  long int obs_start = c_start - 8640;

  EXEC SQL END DECLARE SECTION;

  printf ("C_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\n", st);

  EXEC SQL DECLARE obs_cursor_grb CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:st, -14.95, 50) AS alt FROM targets, grb
    WHERE (type_id =
	   'G' AND (tar_lastobs is NULL) or tar_lastobs <
	   abstime (:obs_start)) and grb_id > 100 and obj_alt (tar_ra,
							       tar_dec,:st,
							       -14.95,
							       50) >
    20 and targets.tar_id =
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

      if (find_plan (plan, tar_id, c_start - 1800))
	{
	  printf ("grb find id: %i\n", tar_id);
	  add_target (plan, TARGET_LIGHT, tar_id, -1, ra, dec, c_start,
		      PLAN_TOLERANCE);

	  EXEC SQL CLOSE obs_cursor_grb;

	  test_sql;

	  return 0;
	}
    }

  EXEC SQL CLOSE obs_cursor_grb;

  test_sql;

#undef test_sql

  return -1;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

/*!
 * If needed, select next observation from list of photometry fields
 */
int
select_next_photometry (time_t c_start, struct target *plan)
{
EXEC SQL BEGIN DECLARE SECTION EXEC SQL END DECLARE SECTION}

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
 */
int
get_next_plan (struct target *plan, int selector_type,
	       time_t * obs_start, int number, float exposure)
{
  double az;
  double airmass;

  double jd;

  struct ln_equ_posn moon;
  struct ln_hrz_posn moon_hrz;
  struct ln_equ_posn sun;
  struct ln_lnlat_posn observer;

  // check for GRB..
  if (!select_next_grb (*obs_start, plan))
    {
      return 0;
    }

  switch (selector_type)
    {

    case SELECTOR_ALTITUDE:
      return select_next_alt (*obs_start, plan);

    case SELECTOR_AIRMASS:
      // every 50th image will be dark..
      if (number % 50 == 1)
	{
	  add_target (plan, TARGET_DARK, -1, -1, 0, 0, *obs_start,
		      PLAN_DARK_TOLERANCE);
	  return 0;
	}

      observer.lng = -15;
      observer.lat = 50;

      jd = get_julian_from_timet (obs_start);

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

      return select_next_airmass (*obs_start, plan, airmass, 90, 270);
      break;

    case SELECTOR_ANTISOLAR:
      // every 50th image will be dark..
      if (number % 50 == 1)
	{
	  add_target (plan, TARGET_DARK, -1, -1, 0, 0, *obs_start,
		      PLAN_DARK_TOLERANCE);
	  return 0;
	}

      observer.lng = -15;
      observer.lat = 50;

      jd = get_julian_from_timet (obs_start);

      get_equ_solar_coords (jd, &sun);
      sun.ra = range_degrees (sun.ra - 180);
      sun.dec = -sun.dec;

      add_target (plan, TARGET_LIGHT, 1000, -1, sun.ra, sun.dec, *obs_start,
		  PLAN_TOLERANCE);
      return 0;

    default:
      return -1;
    }
}

int
make_plan (struct target **plan, float exposure)
{
  struct target *last;
  time_t c_start;
  int i;

  c_start = time (NULL) + 180;

  *plan = (struct target *) malloc (sizeof (struct target));
  (*plan)->type = TARGET_LIGHT;
  (*plan)->id = -1;
  (*plan)->next = NULL;
  last = *plan;

  for (i = 0; i < 10; i++)
    {
      get_next_plan (*plan, SELECTOR_AIRMASS, &c_start, i, exposure);
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
