#ifndef __RTS_TELESCOPE__
#define __RTS_TELESCOPE__

#include "telescope_info.h"

extern int telescope_init (const char *device_name, int telescope_id);
extern void telescope_done ();

extern int telescope_base_info (struct telescope_info *info);
extern int telescope_info (struct telescope_info *info);

extern int telescope_move_to (double ra, double dec);
extern int telescope_set_to (double ra, double dec);
extern int telescope_correct (double ra, double dec);
extern int telescope_change (double ra, double dec);
extern int telescope_start_move (char direction);
extern int telescope_stop_move (char direction);
extern int telescope_stop ();

extern int telescope_park ();
extern int telescope_home ();
extern int telescope_off ();

#endif /* __RTS_TELESCOPE__ */
