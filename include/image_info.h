#ifndef __RTS_IMAGE_INFO__
#define __RTS_IMAGE_INFO__

#include "camera_info.h"
#include "telescope_info.h"

#include <time.h>

struct image_info 
{
  struct telescope_info telescope;
  struct camera_info camera;
  time_t exposure_time;
  float exposure_length;
  int target_type;
  int target_id;
  int observation_id;
};

#endif /* __RTS_IMAGE_INFO__ */
