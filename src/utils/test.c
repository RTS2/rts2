#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "hms.h"



int
main (int argc, char **argv)
{
  char hms[60];
  int i;
  double value;
  printf ("10.2 - %f\n", hmstod ("10.2"));
  printf ("10:58:67 - %f\n", hmstod ("10:58:67"));

  for (i = 0; i <= 3600; i++)
    {
      value = -11 - i / 3600.0;
      dtohms (value, hms);
      printf ("%f - %s - %f - %f - %f\n ", value, hms, hmstod (hms),
	      (value * 10000), hmstod (hms) * 10000);

      assert (round (value * 10000) == round (hmstod (hms) * 10000));
    }
  return 0;
}
