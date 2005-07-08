/*! @file Test file
* $Id$
* @author petr
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#include "mkpath.h"

#include "objectcheck.h"
#include "rts2config.h"

int
main (int argc, char **argv)
{
  double value;
  int i_value;

  char buf[20];
  ObjectCheck *checker;

  assert (mkpath ("test/test1/test2/test3/", 0777) == -1);
  assert (mkpath ("aa/bb/cc/dd", 0777) == 0);

  Rts2Config *conf = new Rts2Config ();

  printf ("ret %i\n", conf->loadFile ("test.ini"));
  printf ("ret %i ", conf->getDouble ("observation", "longtitude", value));
  printf ("val %f\n", value);
  printf ("ret %i ", conf->getDouble ("observation", "latitude", value));
  printf ("val %f\n", value);
  printf ("C0.rotang: %i ", conf->getDouble ("C0", "rotang", value));
  printf ("val %f\n", value);
  printf ("ret: %i ", conf->getString ("C1", "name", buf, 20));
  printf ("val %s\n", buf);
  printf ("ret: %i ", conf->getDouble ("centrald", "day_horizont", value));
  printf ("val %f\n", value);
  printf ("ret: %i ", conf->getDouble ("centrald", "night_horizont", value));
  printf ("val %f\n", value);
  printf ("ret: %i ", conf->getInteger ("hete", "dark_frequency", i_value));
  printf ("val %i\n", i_value);

  printf ("ret: %i ", conf->getString ("CNF1", "script", buf, 20));
  printf ("val %s\n", buf);
  printf ("ret: %i ", conf->getString ("observation", "horizont", buf, 20));
  checker = new ObjectCheck (buf);

  for (value = 0; value < 360; value += 7.5)
    {
      printf ("%f -20 is_good: %i\n", value,
	      checker->is_good (0, value, -20));
      printf ("%f 0 is_good: %i\n", value, checker->is_good (0, value, 0));
      printf ("%f 80 is_good: %i\n", value, checker->is_good (0, value, 80));
    }

  delete checker;
  delete conf;

  return 0;
}
