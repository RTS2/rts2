#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <libnova/libnova.h>

#include "maps.h"
#include "status.h"
#include "telescope.h"

#define TEL_STATUS  	Tstat->serial_number[63]
#define MOVE_TIMEOUT	200


class Rts2DevTelescopeBridge:public Rts2DevTelescope
{
private:
  int _Tstat_handle, _Tctrl_handle;
  struct T9_ctrl *Tctrl;
  struct T9_stat *Tstat;
  double get_loc_sid_time ();
  time_t timeout;
  time_t startTime;
public:
    Rts2DevTelescopeBridge (int argc, char **argv);
    virtual ~ Rts2DevTelescopeBridge (void);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int endMove ();
  virtual int startPark ();
  virtual int isParking ();
  virtual int endPark ();
  virtual int stopMove ();
};

Rts2DevTelescopeBridge::Rts2DevTelescopeBridge (int argc, char **argv):
Rts2DevTelescope (argc, argv)
{
  _Tctrl_handle = -1;
  _Tstat_handle = -1;
  timeout = 0;
  startTime = 0;
}

Rts2DevTelescopeBridge::~Rts2DevTelescopeBridge (void)
{
  close (_Tctrl_handle);
  close (_Tstat_handle);
}

int
Rts2DevTelescopeBridge::init ()
{
  int ret;
  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

  _Tstat_handle = open (STATFILE, O_RDWR, 0755);
  Tstat =
    (struct T9_stat *) mmap (NULL, sizeof (struct T9_stat), PROT_READ,
			     MAP_SHARED, _Tstat_handle, 0);
  if (Tstat == MAP_FAILED)
    {
      syslog (LOG_ERR, "mmap %s failed (%m)", STATFILE);
      return -1;
    }

  syslog (LOG_DEBUG, "%s mapped to %p\n", STATFILE, Tstat);

  _Tctrl_handle = open (CTRLFILE, O_RDWR, 0755);
  Tctrl =
    (struct T9_ctrl *) mmap (NULL, sizeof (struct T9_ctrl),
			     PROT_WRITE | PROT_READ, MAP_SHARED,
			     _Tctrl_handle, 0);
  if (Tctrl == MAP_FAILED)
    {
      syslog (LOG_ERR, "mmap %s failed: %m", CTRLFILE);
      return -1;
    }

  syslog (LOG_DEBUG, "%s mapped to %p", CTRLFILE, Tctrl);
  return 0;
}

int
Rts2DevTelescopeBridge::ready ()
{
  return 0;
}

int
Rts2DevTelescopeBridge::baseInfo ()
{
  strcpy (telType, Tstat->type);
  strcpy (telSerialNumber, Tstat->serial_number);
  telLongtitude = Tstat->longtitude;
  telLatitude = Tstat->latitude;
  telAltitude = Tstat->altitude;
  telParkDec = Tstat->park_dec;
  return 0;
}

double
Rts2DevTelescopeBridge::get_loc_sid_time ()
{
  return 15 * ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) +
    Tstat->longtitude;
}

int
Rts2DevTelescopeBridge::info ()
{
  telRa = Tstat->ra;
  telDec = Tstat->dec;

  telSiderealTime = get_loc_sid_time ();
  telLocalTime = Tstat->localtime;
  telFlip = Tstat->flip;

  telAxis[0] = Tstat->axis0_counts;
  telAxis[1] = Tstat->axis1_counts;

  return 0;
}

int
Rts2DevTelescopeBridge::startMove (double ra, double dec)
{
  Tctrl->power = 0;		//changed
  Tctrl->ra = ra;
  Tctrl->dec = dec;

  time (&startTime);
  timeout = startTime + MOVE_TIMEOUT;

  return 0;
}

int
Rts2DevTelescopeBridge::isMoving ()
{
  time_t now;
  time (&now);
  if (now - startTime < 3)
    return USEC_SEC;
  // finish due to error
  if (now > timeout)
    {
      return -1;
    }
  // calculate distance..
  if (getMoveTargetSep () > 3)
    {
      return USEC_SEC;
    }
  if (TEL_STATUS != TEL_POINT)
    {
      return USEC_SEC;
    }
  return -2;
}

int
Rts2DevTelescopeBridge::endMove ()
{
  return 0;
}

int
Rts2DevTelescopeBridge::startPark ()
{
  Tctrl->ra = get_loc_sid_time () - 30;
  Tctrl->dec = Tstat->dec;
  time (&startTime);
  timeout = startTime + MOVE_TIMEOUT;
  return 0;
}

int
Rts2DevTelescopeBridge::isParking ()
{
  return isMoving ();
}

int
Rts2DevTelescopeBridge::endPark ()
{
  Tctrl->power = 1;		//changed
  return 0;
}

int
Rts2DevTelescopeBridge::stopMove ()
{
  // should do the work
  Tctrl->power = 0;		//changed
  Tctrl->ra = telRa;
  Tctrl->dec = telDec;

  time (&timeout);
  timeout--;
  return Rts2DevTelescope::stopMove ();
}

Rts2DevTelescopeBridge *device;

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
  device = new Rts2DevTelescopeBridge (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize telescope bridge - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
