#ifndef __RTS_SELECTOR__
#define __RTS_SELECTOR__
#include "target.h"
#include <time.h>

#define SELECTOR_AIRMASS	1
#define SELECTOR_ALTITUDE	2
#define SELECTOR_HETE		3
#define SELECTOR_GPS		4
#define SELECTOR_ELL            5	// prefer things with ell target

#ifdef __cplusplus
extern "C"
{
#endif

  extern int
    get_next_plan (Target * plan, int selector_type,
		   time_t * obs_time, int number, float exposure, int state,
		   float lon, float lat);
  extern void free_plan (Target * plan);

#ifdef __cplusplus
};
#endif

#endif /* __RTS_SELECTOR__ */
