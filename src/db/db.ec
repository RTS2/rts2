/*!
 * @file Observation database access.
 *
 * @author petr
 */

#include "db.h"
#include <libnova.h>
#include <malloc.h>
#include <string.h>

#define test_sql if (sqlca.sqlcode < 0) goto err

exec sql include sqlca;

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
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

extern int
db_end_observation (int tar_id, int obs_id, const time_t * end_time)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int t_id = tar_id;
  int o_id = obs_id;
  long int e_time = *end_time;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  EXEC SQL UPDATE observations SET obs_duration =
    abstime (:e_time) - obs_start WHERE obs_id =:o_id;

  test_sql;

  EXEC SQL UPDATE targets SET tar_lastobs =
    abstime (:e_time) WHERE tar_id =:t_id;

  test_sql;

  EXEC SQL END;

  test_sql;

  return 0;
err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

extern int
db_add_darkfield (char *path, const time_t * exposure_time, int
		  exposure_length, int temp, char *camera_name)
{
  EXEC SQL BEGIN DECLARE SECTION;

  char *image_path = path;
  float chip_temp = temp;
  long int exp_time = *exposure_time;
  long int exp_length = exposure_length;
  char *d_camera_name = camera_name;

  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  EXEC SQL INSERT INTO darks (dark_name, dark_date, dark_exposure,
			      dark_temperature, camera_name)
    VALUES (:image_path,
	    abstime (:exp_time),:exp_length,:chip_temp,:d_camera_name);

  EXEC SQL END;

  test_sql;

  return 0;

err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}

extern int
db_update_grb (int id, int seqn, double ra, double dec, time_t * date,
	       int *r_tar_id)
{
  EXEC SQL BEGIN DECLARE SECTION;

  int tar_id;
  int grb_id = id;
  int grb_seqn;

  char tar_name[10];

  double tar_ra = ra;
  double tar_dec = dec;

  long int grb_date = *date;

  EXEC SQL END DECLARE SECTION;

  EXEC SQL BEGIN;

  test_sql;

  EXEC SQL DECLARE tar_cursor CURSOR FOR
    SELECT targets.tar_id, grb_seqn FROM targets, grb WHERE targets.tar_id =
    grb.tar_id AND grb_id =:grb_id;

  test_sql;

  EXEC SQL OPEN tar_cursor;

  EXEC SQL FETCH next FROM tar_cursor INTO:tar_id,:grb_seqn;

  test_sql;

  if (!sqlca.sqlcode)
    {
      // observation exists, just update
      if (grb_seqn < seqn)
	{
	  grb_seqn = seqn;
	  EXEC SQL UPDATE targets
	    SET tar_ra =:tar_ra,
	    tar_dec =:tar_dec WHERE targets.tar_id =:tar_id;
	  test_sql;
	  EXEC SQL UPDATE grb
	    SET grb_seqn =:grb_seqn, grb_date =
	    abstime (:grb_date), grb_last_update =
	    now () WHERE grb.tar_id =:tar_id;
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
      EXEC SQL INSERT INTO targets (tar_id, type_id, tar_name, tar_ra,
				    tar_dec, tar_comment) VALUES (:tar_id,
								  'G',:tar_name,:tar_ra,:tar_dec,
								  'GRB');
      test_sql;

      EXEC SQL INSERT INTO grb (tar_id, grb_id, grb_seqn, grb_date,
				grb_last_update)
	VALUES (:tar_id,:grb_id,:grb_seqn, abstime (:grb_date), now ());
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

/*!
 * Get last dark from database
 *
 * @return set *path (malloc) and retuns 0 when dark is founded, -1 otherwise
 */
int
db_get_darkfield (char *camera_name, int exposure_length, int
		  camera_temperature, char **path)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char d_path[250];
  char *d_camera_name = camera_name;
  float d_exposure_length = exposure_length;
  int d_camera_temperature = camera_temperature;
  EXEC SQL END DECLARE SECTION;
  EXEC SQL BEGIN;
  EXEC SQL DECLARE dark_cursor CURSOR FOR
    SELECT dark_name FROM darks WHERE camera_name =:d_camera_name AND
    dark_exposure =:d_exposure_length AND abs (dark_temperature
					       -:d_camera_temperature) <
    25 ORDER BY dark_date DESC;
  test_sql;
  EXEC SQL OPEN dark_cursor;
  EXEC SQL FETCH next FROM dark_cursor INTO:d_path;
  test_sql;
  if (!sqlca.sqlcode)
    {
      *path = (char *) malloc (strlen (d_path) + 1);
      strcpy (*path, d_path);
      EXEC SQL CLOSE dark_cursor;
      EXEC SQL END;
      return 0;
    }
  *path = NULL;
  EXEC SQL CLOSE dark_cursor;
  test_sql;
  EXEC SQL END;
  return -1;
err:
#ifdef DEBUG
  printf ("err: %li %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  return -1;
}
