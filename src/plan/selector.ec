/*!
 * @file Selector - build plan from target database.
 *
 * @author petr
 */

#include <libnova.h>
#include <stdio.h>
#include <malloc.h>
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

  double jd;

  int tar_id;

  double dec;
  double ra;

  double alt;

  long int obs_start = c_start - 8640;

  EXEC SQL END DECLARE SECTION;

  printf ("C_start: %s", ctime (&c_start));
  jd = get_julian_from_timet (&c_start);
  printf ("jd: %f\n", jd);

  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE obs_cursor_alt CURSOR FOR
    SELECT tar_id, tar_ra, tar_dec,
    obj_alt (tar_ra, tar_dec,:jd, -14.95, 50) AS alt FROM targets
    WHERE NOT EXISTS (SELECT * FROM observations WHERE
		      observations.tar_id = targets.tar_id and
		      observations.obs_start > abstime (:obs_start))
    ORDER BY alt DESC;

  EXEC SQL OPEN obs_cursor_alt;

  test_sql;

  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_alt INTO:tar_id,:ra,:dec,:alt;

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
  printf ("err: %i %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

int
select_next_airmass (time_t c_start, struct target *plan, struct
		     target *last, double target_airmass, double target_az)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;

  double jd;

  int tar_id;

  double dec;
  double ra;

  double az;
  double airmass;
  double rank;

  long int obs_start = c_start - 8640;

  double t_az = target_az;
  double t_airmass = target_airmass;

  EXEC SQL END DECLARE SECTION;

  printf ("c_start: %s", ctime (&c_start));
  jd = get_julian_from_timet (&c_start);
  printf ("jd: %f\n", jd);

  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE obs_cursor_airmass CURSOR FOR
    SELECT tar_id, tar_ra, tar_dec,
    obj_az (tar_ra, tar_dec,:jd, -14.95, 50) AS az,
    obj_airmass (tar_ra, tar_dec,:jd, -14.95, 50) AS
    airmass, (abs (obj_airmass (tar_ra, tar_dec,:jd,
				-14.95,
				50) -:t_airmass) * 180 + abs (obj_az (tar_ra,
								      tar_dec,:jd,
								      -14.95,
								      50)
							      -:t_az)) AS rank
    FROM targets WHERE NOT EXISTS (SELECT *
				   FROM observations WHERE observations.
				   tar_id =
				   targets.tar_id and observations.obs_start >
				   abstime (:obs_start)) ORDER BY rank ASC;

  EXEC SQL OPEN obs_cursor_airmass;

  test_sql;

  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor_airmass
	INTO:tar_id,:ra,:dec,:az,:airmass,:rank;

      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\t%+03.3f\n", tar_id, ra, dec,
	      az, airmass);

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

  EXEC SQL CLOSE obs_cursor_airmass;

  test_sql;

  EXEC SQL END;

#undef test_sql

  return 0;

err:
#ifdef DEBUG
  printf ("err: %i %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
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
  switch (selector_type)
    {
    case SELECTOR_ALTITUDE:
      return select_next_alt (c_start + number * (READOUT_TIME +
						  exposure), curr_plan, plan);

    case SELECTOR_AIRMASS:
      // every 50th image will be dark..
      if (number % 50 == 5)
	{
	  plan->type = TARGET_DARK;
	  plan->id = -1;
	  plan->ctime = c_start + number * (READOUT_TIME + exposure);
	  plan->next = NULL;
	  return 0;
	}

      switch (number & 1)
	{
	case 0:
	  az = 180.0 * rand () / (RAND_MAX + 1.0);
	  airmass = 1.2 + (1.2 * rand () / (RAND_MAX + 1.0));
	  if (az > 90)
	    az += 180;
	  break;
	case 1:
	  az = 360.0 * rand () / (RAND_MAX + 1.0);
	  airmass = 1.0 + (0.2 * rand () / (RAND_MAX + 1.0));
	  break;
	}

      return select_next_airmass (c_start +
				  number * (READOUT_TIME + exposure),
				  curr_plan, plan, airmass, az);
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

  for (i = 0; i < 100; i++)
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
