/*! @file Test file
* $Id$
* @author petr
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <libnova/libnova.h>

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

  struct ln_lnlat_posn *observer;
  observer = conf->getObserver ();
  printf ("long %f\n", observer->lng);
  printf ("lat %f\n", observer->lat);
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

  std::vector < std::string > filt = conf->getCameraFilter ("C0");
  for (std::vector < std::string >::iterator iter = filt.begin ();
       iter != filt.end (); iter++)
    {
      printf ("filter: %s\n", (*iter).c_str ());
    }

  std::vector < std::string > filt2 = conf->getCameraFilter ("C1");
  for (std::vector < std::string >::iterator iter = filt2.begin ();
       iter != filt2.end (); iter++)
    {
      printf ("filter: %s\n", (*iter).c_str ());
    }

  printf ("ret: %i ", conf->getString ("CNF1", "script", buf, 20));
  printf ("val %s\n", buf);
  printf ("ret: %i ", conf->getString ("observatory", "horizont", buf, 20));

  checker = new ObjectCheck (buf);

  for (value = 0; value < 360; value += 7.5)
    {
      printf ("%f -20 is_good: %i\n", value,
	      checker->is_good (0, value, -20));
      printf ("%f 0 is_good: %i\n", value, checker->is_good (18, value, 0));
      printf ("%f 30 is_good: %i\n", value, checker->is_good (18, value, 30));
    }

  delete checker;
  delete conf;

  return 0;
}
