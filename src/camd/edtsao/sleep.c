/* sleep.c
 */
/* copyright 1991, 1993, 1995, 1999 John B. Roll jr.
 */


#include "xos.h"
#include <sys/time.h>

long
SAOSleep (x)
     int x;
{
  time_t then;
  time_t here;

  time (&here);
  then = here + x;

  while (here < then)
    time (&here);
}

void
SAOusleep (t)
     double t;
{
  struct timeval timeout;

  timeout.tv_sec = (int) t;
  timeout.tv_usec = (int) ((t - timeout.tv_sec) * 1000000);

  select (0, NULL, NULL, NULL, &timeout);
}
