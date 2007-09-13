#include <string.h>

#include "rts2user.h"
#include "status.h"

Rts2User::Rts2User (int in_centralId, int in_priority, char in_priority_have,
		    const char *in_login)
{
  centralId = in_centralId;
  priority = in_priority;
  havePriority = in_priority_have;
  strncpy (login, in_login, DEVICE_NAME_SIZE);
  login[DEVICE_NAME_SIZE - 1] = '\0';
}

Rts2User::~Rts2User (void)
{
}

int
Rts2User::update (int in_centralId, int new_priority, char new_priority_have,
		  const char *new_login)
{
  if (in_centralId != centralId)
    return -1;
  priority = new_priority;
  havePriority = new_priority_have;
  strncpy (login, new_login, DEVICE_NAME_SIZE);
  login[DEVICE_NAME_SIZE - 1] = '\0';
  return 0;
}
