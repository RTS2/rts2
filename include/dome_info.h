#ifndef __RTS_DOME_INFO__
#define __RTS_DOME_INFO__

struct dome_info
{
  char type[64];
  int state;
  float temperature;
  float humidity;
};

#endif /* __RTS_DOME_INFO__ */
