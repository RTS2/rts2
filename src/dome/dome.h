#ifndef __RTS2_DOME__
#define __RTS2_DOME__

#include "dome_info.h"

extern int dome_init (char *device);
extern int dome_done ();

extern int dome_info (struct dome_info **info);

extern int dome_observing ();
extern int dome_standby ();
extern int dome_off ();

#endif /* ! __RTS2_DOME__ */
