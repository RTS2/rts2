#ifndef __RTS_TELESCOPE__
#define __RTS_TELESCOPE__

struct telescope_info
{
  char name[64];
  char serial_number[64];
  double ra;
  double dec;
  int moving;
  double park_dec;
  // geographic informations
  double longtitude;
  double latitude;
  // time information
  double siderealtime;
  double localtime;
};

extern int telescope_init (const char *device_name, int telescope_id);
extern void telescope_done ();

extern int telescope_info (struct telescope_info *info);

extern int telescope_move_to (double ra, double dec);
extern int telescope_set_to (double ra, double dec);
extern int telescope_correct (double ra, double dec);
extern int telescope_stop ();

extern int telescope_park ();
extern int telescope_off ();

#endif /* __RTS_TELESCOPE__ */
