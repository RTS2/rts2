#include "rts2config.h"

Rts2Config *
  Rts2Config::pInstance = NULL;

void
Rts2Config::getSpecialValues ()
{
  std::string horizon_file;

  // get some commonly used values
  observer.lat = 0;
  observer.lng = 0;
  getDouble ("observatory", "longtitude", observer.lng);
  getDouble ("observatory", "latitude", observer.lat);
  // load horizont file..
  getString ("observatory", "horizont", horizon_file);
  checker = new ObjectCheck (horizon_file.c_str ());
  getInteger ("imgproc", "astrometry_timeout", astrometryTimeout);
  getDouble ("calibration", "airmass_distance", calibrationAirmassDistance);
  getDouble ("calibration", "lunar_dist", calibrationLunarDist);
  getInteger ("calibration", "valid_time", calibrationValidTime);
  getInteger ("calibration", "max_delay", calibrationMaxDelay);
  getFloat ("calibration", "min_bonus", calibrationMinBonus);
  getFloat ("calibration", "max_bonus", calibrationMaxBonus);
}

Rts2Config::Rts2Config ():Rts2ConfigRaw ()
{
  observer.lat = 0;
  observer.lng = 0;
  checker = NULL;
  // default to 120 seconds
  astrometryTimeout = 120;
  calibrationAirmassDistance = 0.1;
  calibrationLunarDist = 20.0;
  calibrationValidTime = 3600;
  calibrationMaxDelay = 7200;
  calibrationMinBonus = 1.0;
  calibrationMaxBonus = 300.0;
}

Rts2Config::~Rts2Config (void)
{
  delete checker;
}

Rts2Config *
Rts2Config::instance ()
{
  if (!pInstance)
    pInstance = new Rts2Config ();
  return pInstance;
}

struct ln_lnlat_posn *
Rts2Config::getObserver ()
{
  return &observer;
}

ObjectCheck *
Rts2Config::getObjectChecker ()
{
  return checker;
}

int
Rts2Config::getDeviceMinFlux (const char *device, double &minFlux)
{
  return getDouble (device, "minflux", minFlux);
}
