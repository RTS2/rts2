#ifndef __RTS2_USER__
#define __RTS2_USER__

#include "status.h"

class Rts2User
{
  int centralId;
  int priority;
  char havePriority;
  char login[DEVICE_NAME_SIZE];
  char *status_txt;
public:
    Rts2User (int in_centralId, int in_priority, char in_priority_have,
	      const char *in_login, const char *in_status_txt);
    virtual ~ Rts2User (void);
  int update (int in_centralId, int new_priority, char new_priority_have,
	      const char *new_login, const char *new_status_txt);
};

#endif /* !__RTS2_USER__ */
