/*! @file Test file
* $Id$
* @author petr
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#include "config.h"
#include "hms.h"
#include "mkpath.h"

int
main (int argc, char **argv)
{
//  char hms[60];
//  int i;
  double value;
  char *loc;
  printf ("10.2 - %f\n", hmstod ("10.2"));
  printf ("10a58V67 - %f\n", hmstod ("10a58V67"));

  value = hmstod ("-11aa11:57a");
  assert (errno != 0);

/*  for (i = 0; i <= 3600; i++)
    {
      value = -11 - i / 3600.0;
      dtohms (value, hms);
      printf ("%f - %s - %f - %f - %f\n ", value, hms, hmstod (hms),
	      (value * 10000), hmstod (hms) * 10000);
      assert ((round (value * 10000) == round (hmstod (hms) * 10000))
	      && (errno == 0));
    } */

  assert (mkpath ("test/test1/test2/test3/", 0777) == -1);
  assert (mkpath ("aa/bb/cc/dd", 0777) == 0);
  printf ("ret %i\n", read_config ("/usr/local/etc/rts2.conf"));

  printf ("ret %f\n", get_double_default ("longtitude", 1));
  printf ("value: %f %s\n", value, loc);
  printf ("ret %f\n", get_double_default ("latitude", 1));
  printf ("value: %f %s\n", value, loc);
  get_device_string ("C0", "name", &loc);
  printf ("C0.rotang: %f\n", get_device_double_default ("C0", "rotang", 10));
  printf ("ret: %s %s\n", loc,
	  get_device_string_default ("C1", "name", "moje"));
  printf ("ret: %f\n", get_double_default ("day_horizont", 25));
  printf ("ret: %f\n", get_double_default ("night_horizont", 25));
  printf ("ret: %f\n",
	  get_device_double_default ("hete", "dark_frequency", 25));
  return 0;
}
