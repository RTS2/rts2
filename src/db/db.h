#ifndef __RTS_PLAN_DB__
#define __RTS_PLAN_DB__

#include <time.h>

int db_connect (void);
int db_disconnect (void);

int db_start_observation (int id, const time_t * start, int *obs_id);
int db_end_observation (int id, int duration);

int db_update_grb (int id, int seqn, double ra, double dec, int *r_tar_id);

int db_add_darkfield (char *path, const time_t * exposure_time, float
		      exposure_length, float temp);

#endif /* __RTS_PLAN_DB__ */
