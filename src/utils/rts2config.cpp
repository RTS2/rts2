/* 
 * Configuration file read routines.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek,net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

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

  // SWIFT section
  swift_min_horizon = 0;
  getFloat ("swift", "min_horizon", swift_min_horizon);
  swift_soft_horizon = swift_min_horizon;
  getFloat ("swift", "soft_horizon", swift_soft_horizon);

  // GRD section
  grbd_follow_fake = true;
  getBoolean ("grbd", "follow_fake", grbd_follow_fake);
  grbd_validity = 0;
  getInteger ("grbd", "validity", grbd_validity);
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
