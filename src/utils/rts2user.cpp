#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>

#include "rts2user.h"
#include "status.h"

Rts2User::Rts2User (int in_centralId, int in_priority, char in_priority_have,
		    const char *in_login, const char *in_status_txt)
{
  centralId = in_centralId;
  priority = in_priority;
  havePriority = in_priority_have;
  strncpy (login, in_login, DEVICE_NAME_SIZE);
  login[DEVICE_NAME_SIZE - 1] = '\0';
  status_txt = new char[strlen (in_status_txt) + 1];
  strcpy (status_txt, in_status_txt);
}

Rts2User::~Rts2User (void)
{
  delete status_txt;
}

int
Rts2User::update (int in_centralId, int new_priority, char new_priority_have,
		  const char *new_login, const char *new_status_txt)
{
  if (in_centralId != centralId)
    return -1;
  priority = new_priority;
  havePriority = new_priority_have;
  strncpy (login, new_login, DEVICE_NAME_SIZE);
  login[DEVICE_NAME_SIZE - 1] = '\0';
  delete status_txt;
  status_txt = new char[strlen (new_status_txt) + 1];
  strcpy (status_txt, new_status_txt);
  return 0;
}
