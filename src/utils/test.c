#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#include "hms.h"



int
main (int argc, char **argv)
{
  char hms[60];
  int i;
  double value;
  printf ("10.2 - %f\n", hmstod ("10.2"));
  printf ("10a58V67 - %f\n", hmstod ("10a58V67"));

  value = hmstod ("-11aa11:57a");
  assert (errno != 0);

  for (i = 0; i <= 3600; i++)
    {
      value = -11 - i / 3600.0;
      dtohms (value, hms);
      printf ("%f - %s - %f - %f - %f\n ", value, hms, hmstod (hms),
	      (value * 10000), hmstod (hms) * 10000);
      assert ((round (value * 10000) == round (hmstod (hms) * 10000))
	      && (errno == 0));
    }

  return 0;
}
