#ifndef __RTS_DOME_INFO__
#define __RTS_DOME_INFO__

struct dome_info
{
  char type[64];
  float temperature;
  float humidity;
  int dome;
  int power_telescope;
  int power_cameras;
};

#endif /* __RTS_DOME_INFO__ */
