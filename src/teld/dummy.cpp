#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <signal.h>

#include "telescope.h"

/*!
 * Dummy teld for testing purposes.
 */

class Rts2DevTelescopeDummy:public Rts2DevTelescope
{
private:
  double newRa;
  double newDec;
  long countLong;
public:
    Rts2DevTelescopeDummy (int argc, char **argv):Rts2DevTelescope (argc,
								    argv)
  {
    newRa = 0;
    newDec = 0;

    telLongtitude = 0;
    telLatitude = 0;
    telAltitude = 0;
  }
  virtual int ready ()
  {
    return 0;
  }

  virtual int baseInfo ()
  {
    return 0;
  }

  virtual int info ()
  {
    telRa = newRa;
    telDec = newDec;
    telSiderealTime =
      15 * ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) +
      telLongtitude;
    return 0;
  }

  virtual int startMove (double tar_ra, double tar_dec)
  {
    newRa = tar_ra;
    newDec = tar_dec;
    countLong = 0;
    return 0;
  }

  virtual int isMoving ()
  {
    if (countLong > 12)
      return -2;
    countLong++;
    return USEC_SEC;
  }

  virtual int endMove ()
  {
    return 0;
  }

  virtual int stop ()
  {
    newRa = 1;
    newDec = 1;
    return 0;
  }

  virtual int startPark ()
  {
    newRa = 2;
    newDec = 2;
    countLong = 0;
    return 0;
  }

  virtual int isParking ()
  {
    return isMoving ();
  }

  virtual int endPark ()
  {
    return 0;
  }
};

Rts2DevTelescopeDummy *device;

void
killSignal (int sig)
{
  if (device)
    delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevTelescopeDummy (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize dummy telescope - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
