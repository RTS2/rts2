/*!
 * @file Observation database access.
 *
 * @author petr
 */

#include "db.h"
#include <libnova.h>

extern int
db_connect (void)
{
  EXEC SQL CONNECT TO stars;
  return sqlca.sqlcode;
}

extern int
db_disconnect (void)
{
  EXEC SQL DISCONNECT all;
  return 0;
}

extern int
db_start_observation (int id, const time_t * c_start, int *obs_id)
{
#define test_sql if (sqlca.sqlcode < 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
  int tar_id = id;
  int obs;

  long int obs_start = *c_start;

  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  EXEC SQL SELECT nextval ('obs_id') INTO:obs;

  test_sql;

  *obs_id = obs;

  EXEC SQL INSERT INTO observations (tar_id, obs_id, obs_start) VALUES
    (:tar_id,:obs, abstime (:obs_start));

  EXEC SQL END;

  test_sql;

  return 0;

err:
#ifdef DEBUG
  printf ("err: %i %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

extern int
db_end_observation (int tar_id, int obs_id, const time_t * end_time,
		    int duration)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int t_id = tar_id;
  int o_id = obs_id;
  long int e_time = *end_time;
  int dur = duration;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  EXEC SQL UPDATE observations SET obs_duration =:dur WHERE obs_id =:o_id;

  EXEC SQL UPDATE targets SET tar_lastobs =
    abstime (:e_time) WHERE tar_id =:t_id;

  EXEC SQL END;

  test_sql;

  return 0;
err:
#ifdef DEBUG
  printf ("err: %i %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

extern int
db_add_darkfield (char *path, const time_t * exposure_time, float
		  exposure_length, float temp)
{
  EXEC SQL BEGIN DECLARE SECTION;

  char *image_path = path;
  float chip_temp = temp;
  long int exp_time = *exposure_time;
  long int exp_length = exposure_length;

  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  EXEC SQL INSERT INTO darks (dark_name, dark_date, dark_exposure,
			      dark_temperature)
    VALUES (:image_path, abstime (:exp_time),:exp_length,:chip_temp);

  EXEC SQL END;

  test_sql;

  return 0;

err:
#ifdef DEBUG
  printf ("err: %i %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

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
