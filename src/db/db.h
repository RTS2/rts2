#ifndef __RTS__DB__
#define __RTS__DB__

#include <time.h>

int db_connect (void);
int db_disconnect (void);

void db_lock ();
void db_unlock ();

int db_get_media_path (int media_id, char *path, size_t path_len);

int db_start_observation (int id, const time_t * start, int *obs_id);
int db_end_observation (int obs_id, const time_t * end_time);

int db_update_grb (int id, int seqn, double ra, double dec, time_t * date,
		   int *r_tar_id);

int db_add_darkfield (char *path, const time_t * exposure_time, int
		      exposure_length, int temp, char *camera_name);

int db_get_darkfield (char *camera_name, int exposure_length,
		      int camera_temperature, char **path);

extern int db_last_good_image (char *camera_name);
extern int db_last_observation (int tar_id);
extern int db_last_night_images_count (int tar_id);

#endif /* __RTS_DB__ */
