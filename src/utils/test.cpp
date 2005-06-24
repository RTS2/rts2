/*! @file Test file
* $Id$
* @author petr
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#include "config.h"
#include "mkpath.h"

#include "objectcheck.h"

int
main (int argc, char **argv)
{
  double value;
  char buf[20];
  ObjectCheck *checker;

  assert (mkpath ("test/test1/test2/test3/", 0777) == -1);
  assert (mkpath ("aa/bb/cc/dd", 0777) == 0);
  printf ("ret %i\n", read_config ("/usr/local/etc/rts2.conf"));

  printf ("ret %f\n", get_double_default ("longtitude", 1));
  printf ("ret %f\n", get_double_default ("latitude", 1));
  printf ("C0.rotang: %f\n", get_device_double_default ("C0", "rotang", 10));
  printf ("ret: %s\n", get_device_string_default ("C1", "name", "moje"));
  printf ("ret: %f\n", get_double_default ("day_horizont", 25));
  printf ("ret: %f\n", get_double_default ("night_horizont", 25));
  printf ("ret: %f\n",
	  get_device_double_default ("hete", "dark_frequency", 25));

  printf ("ret: %s\n", get_string_default ("epoch", "000"));
  printf ("ret: %s\n",
	  get_sub_device_string_default ("CNF1", "script", "G", "AA"));
  printf ("ret: %s\n",
	  get_sub_device_string_default ("CNF1", "script", "S", "AA"));

  checker =
    new ObjectCheck (get_string_default ("horizont", "/etc/rts2/horizont"));

  for (value = 0; value < 360; value += 7.5)
    {
      printf ("%f -20 is_good: %i\n", value,
	      checker->is_good (0, value, -20));
      printf ("%f 0 is_good: %i\n", value, checker->is_good (0, value, 0));
      printf ("%f 80 is_good: %i\n", value, checker->is_good (0, value, 80));
    }

  delete checker;

  return 0;
}
