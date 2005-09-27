#ifndef __RTS_IMAGE_INFO__
#define __RTS_IMAGE_INFO__

#include "camera_info.h"
#include "telescope_info.h"
#include "dome_info.h"

#include <libnova/libnova.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define TARGET_LIGHT		1
#define TARGET_DARK		2
#define TARGET_FLAT		3
#define TARGET_FLAT_DARK	4

typedef struct
{
  struct ln_equ_posn image_pos;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
  int hi_precision;
  int ntries;
  int processed;
} hi_precision_t;

struct image_info
{
  char *camera_name;
  char *telescope_name;
  char *dome_name;
  struct telescope_info telescope;
  struct camera_info camera;
  struct dome_info dome;
  struct timeval exposure_tv;
  float exposure_length;
  int target_type;
  int target_id;
  int observation_id;
  int obs_type;
  hi_precision_t *hi_precision;
  // image will be use for feedback to telescope; astrometry and
  // correction will be aplied right after image
  // acqusition, image will not go to any que for
  // processing
  int binnings[22];
  // axes binning
  int good_count;
  // number of observation with images.. (which went past centering
  // step)
};

#endif /* __RTS_IMAGE_INFO__ */
