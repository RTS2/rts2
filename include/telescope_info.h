#ifndef __RTS_TELESCOPE_INFO__
#define __RTS_TELESCOPE_INFO__

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
  int correction_mark;
};

#endif /* __RTS_TELESCOPE_INFO__ */
