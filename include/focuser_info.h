#ifndef __RTS_FOCUSER_INFO__
#define __RTS_FOCUSER_INFO__

struct focuser_info
{
  char camera[20];
  char type[20];
  int pos;
  float temp;
};

#endif /* __RTS_FOCUSER_INFO__ */
