#ifndef __RTS_SELECTOR__
#define __RTS_SELECTOR__
#include <time.h>

#define SELECTOR_AIRMASS	1
#define SELECTOR_ALTITUDE	2
#define SELECTOR_HETE		3
#define SELECTOR_GPS		4

struct target
{
  int type;
  int id;
  int obs_id;
  double ra;
  double dec;
  time_t ctime;
  int tolerance;
  int moved;
  struct target *next;
};

extern int
get_next_plan (struct target *plan, int selector_type,
	       time_t * obs_time, int number, float exposure, int state,
	       float lon, float lat);
extern int make_plan (struct target **plan, float exposure, float lon,
		      float lat);
extern void free_plan (struct target *plan);

#endif /* __RTS_SELECTOR__ */
