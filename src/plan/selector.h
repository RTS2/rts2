#ifndef __RTS2_SELECTOR__
#define __RTS2_SELECTOR__
#include "target.h"
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  { SELECTOR_AIRMASS, SELECTOR_ALTITUDE, SELECTOR_HETE,
    SELECTOR_GPS, SELECTOR_ELL
  } selector_t;

  extern int
    get_next_plan (Target * plan, int selector_type,
		   time_t * obs_time, int number, float exposure, int state,
		   float lon, float lat);
  extern void free_plan (Target * plan);

#ifdef __cplusplus
};
#endif

#endif /* __RTS2_SELECTOR__ */
