#ifndef __RTS_DOME_INFO__
#define __RTS_DOME_INFO__

struct dome_info
{
  char model[64];
  float temperature;
  float humidity;
  int power_telescope;
  int power_cameras;
  int dome;
};

#endif /* __RTS_DOME_INFO__ */
