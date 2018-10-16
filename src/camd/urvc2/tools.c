#define _TOOLS_C

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define GETBASEADDR_BUFLEN 128

int
getbaseaddr (int parport_num)
{
  FILE *fufu;
  char buffer[GETBASEADDR_BUFLEN];
  int a;

  snprintf (buffer, GETBASEADDR_BUFLEN,
	    "/proc/sys/dev/parport/parport%d/base-addr", parport_num);

  fufu = fopen (buffer, "r");
  if (fufu == NULL)
    return 888;
  if(fscanf (fufu, "%d", &a) != 1) a = 888;
  fclose (fufu);

  return a;
}

void
TimerDelay (unsigned long delay)
{
//  end_realtime();
  usleep (delay);
//  begin_realtime();
}

unsigned int
ccd_c2ad (double t)
{
  double r = 3.0 * exp (0.94390589891 * (25.0 - t) / 25.0);
  unsigned int ret = 4096.0 / (10.0 / r + 1.0);
  if (ret > 4096)
    ret = 4096;
  return ret;
}

double
ccd_ad2c (unsigned int ad)
{
  double r;
  if (ad == 0)
    return 1000;
  r = 10.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 25.0 * (log (r / 3.0) / 0.94390589891);
}

double
ambient_ad2c (unsigned int ad)
{
  double r;
  r = 3.0 / (4096.0 / (double) ad - 1.0);
  return 25.0 - 45.0 * (log (r / 3.0) / 2.0529692213);
}
