/*!
 * @file Selector - build plan from target database.
 *
 * @author petr
 */

#include <libnova.h>
#include <stdio.h>
#include <malloc.h>
#include "selector.h"


int
find_plan (struct target *plan, int id, time_t c_start)
{
  for ( ; plan; plan = plan->next)
	if (plan->id == id && plan->ctime > c_start)
		return 0;
  
  return -1;
}


int
select_next (time_t c_start, struct target *plan, struct target *last)
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

  printf ("c_start: %s", ctime (&c_start));
  jd = get_julian_from_timet (&c_start);
  printf ("jd: %f\n", jd);

  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE obs_cursor CURSOR FOR 
  	SELECT tar_id, tar_ra, tar_dec,
		obj_alt (tar_ra, tar_dec, :jd, -14.95, 50) AS alt FROM targets
	    	WHERE NOT EXISTS (SELECT * FROM observations WHERE 
		observations.tar_id = targets.tar_id and 
		observations.obs_start > abstime (:obs_start))
	ORDER BY alt DESC;

  EXEC SQL OPEN obs_cursor;

  test_sql;

  while (!sqlca.sqlcode)
    {
      EXEC SQL FETCH next FROM obs_cursor INTO :tar_id, :ra, :dec, :alt;

      printf ("%8i\t%+03.3f\t%+03.3f\t%+03.3f\n", tar_id, ra, dec, alt);

      if (find_plan (plan, tar_id, c_start - 1800))
      {
      	printf ("find id: %i\n", tar_id);
	last->next = (struct target*) malloc (sizeof (struct target));
	last = last->next;
	last->id = tar_id;
	last->ra = ra;
	last->dec = dec;
	last->ctime = c_start;
	last->next = NULL;
	break;
      }
    }

  EXEC SQL CLOSE obs_cursor;

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
make_plan (struct target **plan)
{
  struct target *last;
  time_t c_start;
  int i;

  c_start = time(NULL) + 20;

  *plan = (struct target*) malloc (sizeof (struct target));
  (*plan)->id = -1;
  (*plan)->next = NULL;
  last = *plan;

  for (i = 0; i < 100; i++)
  {
	select_next (c_start + i * 180, *plan, last);
	last = last->next;
  }

  last = *plan;
  *plan = (*plan) -> next;
  free (last);

  return 0;  
}

void
free_plan (struct target *plan)
{
  struct target *last;

  last = plan->next;
  free (plan);

  for ( ; last; plan = last, last = last->next, free(plan));
}
