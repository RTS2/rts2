#ifndef __RTS2_GRBC__
#define __RTS2_GRBC__

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef int (*process_grb_event_t) (int id, int seqn, double ra, double dec,
				      time_t * date);

#ifdef __cplusplus
};
#endif

#endif /* __RTS2_GRBC__ */
