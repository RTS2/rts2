#ifndef __RTS_CAMERA__
#define __RTS_CAMERA__

#include "camera_info.h"

#ifdef  __cplusplus
extern "C"
{
#endif

  extern int camera_init (char *device_name, int camera_id);
  extern void camera_done ();

  extern int camera_info (struct camera_info *info);

  extern int camera_fan (int fan_state);

  extern int camera_expose (int chip, float *exposure, int light);
  extern int camera_end_expose (int chip);

  extern int camera_binning (int chip_id, int vertical, int horizontal);

  extern int camera_readout_line (int chip_id, short start, short length,
				  void *data);
  extern int camera_dump_line (int chip_id);
  extern int camera_end_readout (int chip_id);

  extern int camera_cool_max ();	/* try to max temperature */
  extern int camera_cool_hold ();	/* hold on that temperature */
  extern int camera_cool_shutdown ();	/* ramp to ambient */
  extern int camera_cool_setpoint (float coolpoint);	/* set direct setpoint */
  extern int camera_set_filter (int filter);	/* set filter */

#ifdef __cplusplus
};
#endif

#endif /* __RTS_CAMERA__ */
