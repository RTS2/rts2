#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libnova/libnova.h>

#include "telescope.h"
#include "../utils/rts2config.h"

/*!
 * Dummy teld for testing purposes.
 */

class Rts2DevTelescopeDummy:public Rts2DevTelescope
{
private:
  double newRa;
  double newDec;
  long countLong;
protected:

    virtual int isMoving ()
  {
    if (countLong > 12)
      return -2;
    countLong++;
    return USEC_SEC;
  }

  virtual int isMovingFixed ()
  {
    return isMoving ();
  }

  virtual int isSearching ()
  {
    return USEC_SEC;
  }

  virtual int isParking ()
  {
    return isMoving ();
  }
public:
Rts2DevTelescopeDummy (int in_argc, char **in_argv):Rts2DevTelescope (in_argc,
		    in_argv)
  {
    newRa = 0;
    newDec = 0;

    telLongtitude = 0;
    telLatitude = 0;
    telAltitude = 0;
  }

  virtual int init ()
  {
    int ret;
    ret = Rts2DevTelescope::init ();
    if (ret)
      return ret;
    Rts2Config *config;
    config = Rts2Config::instance ();
    config->loadFile ();
    telLatitude = config->getObserver ()->lat;
    telLongtitude = config->getObserver ()->lng;
    return 0;
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
    telSiderealTime = getLocSidTime ();
    return Rts2DevTelescope::info ();
  }

  virtual int startMove (double tar_ra, double tar_dec)
  {
    newRa = tar_ra;
    newDec = tar_dec;
    countLong = 0;
    return 0;
  }

  virtual int startMoveFixed (double tar_az, double tar_alt)
  {
    newRa = tar_az;
    newDec = tar_alt;
    countLong = 0;
    return 0;
  }

  virtual int startSearch ()
  {
    return 0;
  }

  virtual int change (double chng_ra, double chng_dec)
  {
    newRa += chng_ra;
    newDec += chng_dec;
    countLong = 0;
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

  virtual int endPark ()
  {
    return 0;
  }
};

int
main (int argc, char **argv)
{
  Rts2DevTelescopeDummy *device = new Rts2DevTelescopeDummy (argc, argv);
  int ret = device->run ();
  delete device;
  return ret;
}
