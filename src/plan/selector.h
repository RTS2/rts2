#ifndef __RTS_SELECTOR__
#define __RTS_SELECTOR__
#include <time.h>

#define TARGET_LIGHT	1
#define TARGET_DARK	2
#define TARGET_FLAT	3

#define SELECTOR_AIRMASS	1
#define SELECTOR_ALTITUDE	2

struct target
{
  int type;
  int id;
  int obs_id;
  double ra;
  double dec;
  time_t ctime;
  struct target *next;
};

extern int
get_next_plan (struct target *plan, int selector_type,
	       struct target *curr_plan, time_t c_time, int number, float exposure);
extern int make_plan (struct target **plan, float exposure);
extern void free_plan (struct target *plan);

#endif /* __RTS_SELECTOR__ */
