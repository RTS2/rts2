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

#define READOUT_TIME	75


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


int
select_next_alt (time_t c_start, struct target *plan, struct target *last)
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

  EXEC SQL BEGIN;

  test_sql;

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
	  last->type = TARGET_LIGHT;
	  last->id = tar_id;
	  last->ra = ra;
	  last->dec = dec;
	  last->ctime = c_start;
	  last->next = NULL;
	  break;
	}
    }

  EXEC SQL CLOSE obs_cursor_alt;

  test_sql;

  EXEC SQL END;

#undef test_sql

  return 0;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_airmass (time_t c_start, struct target *plan, struct
		     target *last, double target_airmass, double az_end,
		     double az_start)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;

  double st;

  int tar_id;

  double dec;
  double ra;

  double az;
  double airmass;

  long int obs_start = c_start - 8640;

  double d_az_start = az_start;
  double d_az_end = az_end;
  double t_airmass = target_airmass;

  EXEC SQL END DECLARE SECTION;

  printf ("c_start: %s", ctime (&c_start));
  st = get_mean_sidereal_time (get_julian_from_timet (&c_start));
  printf ("st: %f\nairmass: %f", st, t_airmass);


  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE obs_cursor_airmass CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_az (tar_ra, tar_dec,:st, -14.95, 50) AS az,
    obj_airmass (tar_ra, tar_dec,:st, -14.95, 50) AS
    airmass
    FROM targets, targets_images WHERE targets.tar_id =
    targets_images.
    tar_id
    AND (abs (obj_airmass (tar_ra, tar_dec,:st, -14.95, 50) -:t_airmass)) <
    0.1 AND (obj_az (tar_ra, tar_dec,:st, -14.95, 50) <:d_az_end OR
	     obj_az (tar_ra, tar_dec,:st, -14.95,
		     50) >:d_az_start) ORDER BY img_count ASC;
  EXEC SQL OPEN obs_cursor_airmass;

  test_sql;

  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_airmass
	INTO:tar_id,:ra,:dec,:az,:airmass;

      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\t%+03.3f\n", tar_id, ra, dec,
	      az, airmass);

      if (find_plan (plan, tar_id, c_start - 1800))
	{
	  printf ("airmass find id: %i\n", tar_id);
	  last->type = TARGET_LIGHT;
	  last->id = tar_id;
	  last->ra = ra;
	  last->dec = dec;
	  last->ctime = c_start;
	  last->next = NULL;
	  break;
	}
    }

  EXEC SQL CLOSE obs_cursor_airmass;

  test_sql;

  EXEC SQL END;

#undef test_sql

  return 0;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_grb (time_t c_start, struct target *plan, struct target *last)
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

  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE obs_cursor_grb CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:st, -14.95, 50) AS alt FROM targets, grb
    WHERE (type_id =
	   'G' AND (tar_lastobs is NULL) or tar_lastobs <
	   abstime (:obs_start)) and grb_id > 100 and obj_alt (tar_ra,
							       tar_dec,:st,
							       -14.95,
							       50) >
    20 and targets.tar_id = grb.tar_id ORDER BY alt DESC;

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
	  last->type = TARGET_LIGHT;
	  last->id = tar_id;
	  last->ra = ra;
	  last->dec = dec;
	  last->ctime = c_start;
	  last->next = NULL;

	  EXEC SQL CLOSE obs_cursor_grb;

	  test_sql;

	  EXEC SQL END;
	  return 0;
	}
    }

  EXEC SQL CLOSE obs_cursor_grb;

  test_sql;

  EXEC SQL END;

#undef test_sql

  return -1;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
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
 */
int
get_next_plan (struct target *plan, int selector_type,
	       struct target *curr_plan, time_t c_start, int number,
	       float exposure)
{
  double az;
  double airmass;

  double jd;

  struct ln_equ_posn moon;
  struct ln_hrz_posn moon_hrz;
  struct ln_lnlat_posn observer;

  time_t obs_start = c_start + number * (READOUT_TIME + exposure);

  if (!select_next_grb (obs_start, curr_plan, plan))
    {
      return 0;
    }

  switch (selector_type)
    {

    case SELECTOR_ALTITUDE:
      return select_next_alt (obs_start, curr_plan, plan);

    case SELECTOR_AIRMASS:
      // check for GRB..
      // every 50th image will be dark..
      if (number % 50 == 5)
	{
	  plan->type = TARGET_DARK;
	  plan->id = -1;
	  plan->ctime = c_start + number * (READOUT_TIME + exposure);
	  plan->next = NULL;
	  return 0;
	}

      observer.lng = -15;
      observer.lat = 50;

      c_start += number * (READOUT_TIME + exposure);

      jd = get_julian_from_timet (&c_start);

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

      return select_next_airmass (obs_start, curr_plan, plan, airmass, 150,
				  210);
      break;

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
      last->next = (struct target *) malloc (sizeof (struct target));
      last = last->next;
      last->next = NULL;
      get_next_plan (last, SELECTOR_AIRMASS, *plan, c_start, i, exposure);
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
