#ifndef __RTS_PHOT_INFO__
#define __RTS_PHOT_INFO__

struct phot_info
{
  char type[64];
  int count;
  int integration;
  int filter;
};

#endif /* __RTS_PHOT_INFO__ */
