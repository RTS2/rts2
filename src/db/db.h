#ifndef __RTS__DB__
#define __RTS__DB__

#include <time.h>

int db_connect (void);
int db_disconnect (void);

int db_start_observation (int id, const time_t * start, int *obs_id);
int db_end_observation (int tar_id, int obs_id, const time_t * end_time);

int db_update_grb (int id, int seqn, double ra, double dec, int *r_tar_id);

int db_add_darkfield (char *path, const time_t * exposure_time, int
		      exposure_length, int temp, char *camera_name);

int db_get_darkfield (char *camera_name, int exposure_length,
		      int camera_temperature, char **path);

#endif /* __RTS_DB__ */
