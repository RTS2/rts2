#ifndef __RTS_SELECTOR__
#define __RTS_SELECTOR__
#include <time.h>

#define TARGET_LIGHT	1
#define TARGET_DARK	2
#define TARGET_FLAT	3

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

int make_plan (struct target **plan);
void free_plan (struct target *plan);

#endif /* __RTS_SELECTOR__ */
