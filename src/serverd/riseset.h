#ifndef __RTS_RISESET__
#define __RTS_RISESET__

#include <libnova.h>
#include <time.h>
int next_event (struct ln_lnlat_posn *observer, time_t * start_time,
		int *curr_type, int *type, time_t * ev_time);
#endif /* __RTS_RISESET__ */
