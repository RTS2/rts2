/*!
 * @file Observation database access.
 *
 * @author petr
 */

#define _GNU_SOURCE

#include "db.h"
#include <libnova/libnova.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t db_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

EXEC SQL include sqlca;

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

extern void
db_lock (void)
{
#ifdef DEBUG
  printf ("before locking\n");
#endif
  pthread_mutex_lock (&db_lock_mutex);
  EXEC SQL BEGIN;
#ifdef DEBUG
  printf ("after locking\n");
#endif
}

extern void
db_unlock (void)
{
#ifdef DEBUG
  printf ("before unlocking\n");
#endif
  EXEC SQL END;
  pthread_mutex_unlock (&db_lock_mutex);
#ifdef DEBUG
  printf ("after unlocking\n");
#endif
}

extern int
db_get_media_path (int media_id, char *path, size_t path_len)
{
#define test_sql if (sqlca.sqlcode != 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
  VARCHAR med_path[50];
  int med_id = media_id;
  EXEC SQL END DECLARE SECTION;
  db_lock ();
  EXEC SQL SELECT med_path INTO:med_path FROM medias WHERE med_id =:med_id;
  test_sql;
  db_unlock ();
  strncpy (path, med_path.arr, path_len);
  path[path_len] = '\0';
  return 0;
#undef test_sql
err:
#ifdef DEBUG
  printf ("err db_get_media_path: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
  path[0] = '\0';
  return -1;
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
  db_lock ();
  EXEC SQL SELECT nextval ('obs_id') INTO:obs;
  test_sql;
  *obs_id = obs;
  EXEC SQL INSERT INTO observations (tar_id, obs_id, obs_start) VALUES
    (:tar_id,:obs, abstime (:obs_start));
  test_sql;
  db_unlock ();
  return 0;
err:
#ifdef DEBUG
  printf ("err db_start_observation: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
  return -1;
}

extern int
db_end_observation (int obs_id, const time_t * end_time)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int o_id = obs_id;
  long int e_time = *end_time;
  EXEC SQL END DECLARE SECTION;
  db_lock ();
  EXEC SQL UPDATE observations SET obs_duration =
    abstime (:e_time) - obs_start WHERE obs_id =:o_id;
  test_sql;
  db_unlock ();
  return 0;
err:
#ifdef DEBUG
  printf ("err db_end_observation: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
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
  db_lock ();
  EXEC SQL INSERT INTO darks (dark_name, dark_date, dark_exposure,
			      dark_temperature, epoch_id, camera_name)
    VALUES (:image_path,
	    abstime (:exp_time),:exp_length,:chip_temp, '002',:d_camera_name);
  test_sql;
  db_unlock ();
  return 0;
err:
#ifdef DEBUG
  printf ("err db_add_darkfield: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
  return -1;
}

extern int
db_update_grb (int id, int *seqn, double *ra, double *dec, time_t * date,
	       int *r_tar_id)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int tar_id;
  int grb_id = id;
  int grb_seqn;
  char tar_name[10];
  double tar_ra;
  double tar_dec;
  long int grb_date = *date;
  EXEC SQL END DECLARE SECTION;

  db_lock ();
  EXEC SQL DECLARE tar_cursor CURSOR FOR
    SELECT targets.tar_id, tar_ra, tar_dec, grb_seqn FROM targets,
    grb WHERE targets.tar_id = grb.tar_id AND grb_id =:grb_id;
  test_sql;
  EXEC SQL OPEN tar_cursor;
  EXEC SQL FETCH next FROM tar_cursor INTO:tar_id,:tar_ra,:tar_dec,:grb_seqn;
  test_sql;
  if (!sqlca.sqlcode)
    {
      // observation exists, just update
      if (grb_seqn < *seqn)
	{
	  grb_seqn = *seqn;
	  tar_ra = *ra;
	  tar_dec = *dec;
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
      else
	{
	  *seqn = grb_seqn;
	  *ra = tar_ra;
	  *dec = tar_dec;
	}
    }
  // insert new observation
  else
    {
      EXEC SQL SELECT nextval ('tar_id') INTO:tar_id;
      test_sql;
      grb_seqn = *seqn;
      sprintf (tar_name, "GRB %5i", id);
      tar_ra = *ra;
      tar_dec = *dec;
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
  db_unlock ();
  if (r_tar_id)
    *r_tar_id = tar_id;
  return 0;
err:
#ifdef DEBUG
  printf ("err db_update_grb: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
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
  db_lock ();
  EXEC SQL DECLARE dark_cursor CURSOR FOR
    SELECT dark_name FROM darks WHERE camera_name =:d_camera_name AND
    dark_exposure =:d_exposure_length AND abs (dark_temperature
					       -:d_camera_temperature) <
    25 AND now () - dark_date < interval '6 hours' ORDER BY dark_date DESC;
  test_sql;
  EXEC SQL OPEN dark_cursor;
  EXEC SQL FETCH next FROM dark_cursor INTO:d_path;
  test_sql;
  if (!sqlca.sqlcode)
    {
      *path = (char *) malloc (strlen (d_path) + 1);
      strcpy (*path, d_path);
      EXEC SQL CLOSE dark_cursor;
      db_unlock ();
      return 0;
    }
  *path = NULL;
  EXEC SQL CLOSE dark_cursor;
  test_sql;
  db_unlock ();
  return -1;
err:
#ifdef DEBUG
  printf ("err db_get_darkfield: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
  return -1;
}

int
db_last_observation (int tar_id)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int obs_tar_id = tar_id;
  int obs_start;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL DECLARE obs_cursor_last_observation CURSOR FOR
    SELECT MAX (EXTRACT (EPOCH FROM obs_start)) FROM observations
    WHERE tar_id =:obs_tar_id;
  EXEC SQL OPEN obs_cursor_last_observation;
  EXEC SQL FETCH NEXT FROM obs_cursor_last_observation INTO:obs_start;
  if (sqlca.sqlcode)
    {
      EXEC SQL CLOSE obs_cursor_last_observation;
      return -1;
    }
  EXEC SQL CLOSE obs_cursor_last_observation;
  return time (NULL) - obs_start;
}

int
db_last_night_images_count (int tar_id)
{
  EXEC SQL BEGIN DECLARE SECTION;
  int obs_tar_id = tar_id;
  int res;
  EXEC SQL END DECLARE SECTION;

  EXEC SQL SELECT COUNT (*) INTO:res FROM observations, images WHERE
    tar_id =:obs_tar_id AND images.obs_id = observations.obs_id
    AND night_num (img_date) = night_num (now ());
  if (sqlca.sqlcode)
    {
      return -1;
    }
  return res;
}

// returns time to last good image in seconds, -1 if there is no good image
// for camera
int
db_last_good_image (char *camera_name)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char *i_camera_name = camera_name;
  int i_last_date;
  EXEC SQL END DECLARE SECTION;
#define test_sql if (sqlca.sqlcode < 0) goto err

  db_lock ();
  EXEC SQL DECLARE obs_cursor_last_good_image CURSOR FOR
    SELECT MAX (EXTRACT (EPOCH FROM img_date)) FROM images WHERE camera_name
    =:i_camera_name;
  EXEC SQL OPEN obs_cursor_last_good_image;
  EXEC SQL FETCH next FROM obs_cursor_last_good_image INTO:i_last_date;
  if (sqlca.sqlcode)
    {
      EXEC SQL CLOSE obs_cursor_last_good_image;
      db_unlock ();
      printf ("No such image for %s\n", i_camera_name);
      // no such image
      return -1;
    }
  EXEC SQL CLOSE obs_cursor_last_good_image;
  test_sql;
  db_unlock ();

  return time (NULL) - i_last_date;
err:
#ifdef DEBUG
  printf ("err db_last_good_image: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
#endif /* DEBUG */
  db_unlock ();
  return -1;
}
