#ifndef __RTS_PHOT_INFO__
#define __RTS_PHOT_INFO__

struct phot_info
{
  char type[64];
  int count;
  int exposure;
  int filter;
};

#endif /* __RTS_PHOT_INFO__ */
