/*!
 * @file Observation database access.
 *
 * @author petr
 */

#include "db.h"
#include <libnova.h>

#ifdef DEBUG
#include <stdio.h>
#endif /* DEBUG */

extern int
db_update_grb (int id, int seqn, double ra, double dec, int *r_tar_id)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;

  int tar_id;
  int grb_id = id;
  int grb_seqn;

  char tar_name[10];

  double tar_ra = ra;
  double tar_dec = dec;

  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE tar_cursor CURSOR FOR
    SELECT tar_id, grb_seqn FROM grb WHERE grb_id =:grb_id;

  test_sql;

  EXEC SQL OPEN tar_cursor;

  EXEC SQL FETCH next FROM tar_cursor INTO:tar_id,:grb_seqn;

  if (!sqlca.sqlcode)
    {
      // observation exists, just update
      if (grb_seqn < seqn)
	{
	  grb_seqn = seqn;
	  EXEC SQL UPDATE grb
	    SET grb_seqn =:grb_seqn, tar_ra =:tar_ra,
	    tar_dec =:tar_dec, grb_last_update = now ()WHERE tar_id =:tar_id;
	  test_sql;
	}
    }
  // insert new observation
  else
    {
      EXEC SQL SELECT nextval ('tar_id') INTO:tar_id;
      test_sql;
      grb_seqn = seqn;
      sprintf (tar_name, "GRB %5i", id);
      EXEC SQL INSERT INTO grb (tar_id, tar_name, tar_ra, tar_dec,
				tar_comment, grb_id, grb_seqn, grb_date,
				grb_last_update)
	VALUES (:tar_id,:tar_name,:tar_ra,:tar_dec, 'GRB',:grb_id,:grb_seqn,
		now (), now ());
      test_sql;
    }

  EXEC SQL CLOSE tar_cursor;

  EXEC SQL END;

  test_sql;

  if (r_tar_id)
    *r_tar_id = tar_id;

  return 0;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}
