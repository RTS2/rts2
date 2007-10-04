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

/**
 * @file
 * Holds RTS2 extension of Rts2ConfigRaw class. This class is used to acess
 * RTS2-specific values from the configuration file.
 */

#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__

#include "rts2configraw.h"
#include "objectcheck.h"

#include <libnova/libnova.h>

/**
 * Represent full Config class, which includes support for Libnova types and
 * holds some special values. The phylosophy behind this class is to allow
 * quick access, without need to parse configuration file or to search for
 * configuration file string entries. Each value in configuration file should
 * be mapped to apporpriate privat member variable of this class, and public
 * access function should be provided.
 *
 * @ingroup RTS2Block
 */
class Rts2Config:public Rts2ConfigRaw
{
private:
  static Rts2Config *pInstance;
  struct ln_lnlat_posn observer;
  ObjectCheck *checker;
  int astrometryTimeout;
  double calibrationAirmassDistance;
  double calibrationLunarDist;
  int calibrationValidTime;
  int calibrationMaxDelay;
  float calibrationMinBonus;
  float calibrationMaxBonus;

  float swift_min_horizon;
  float swift_soft_horizon;
protected:
    virtual void getSpecialValues ();

public:
    Rts2Config ();
    virtual ~ Rts2Config (void);

  static Rts2Config *instance ();

  // some special functions..
  double getCalibrationAirmassDistance ()
  {
    return calibrationAirmassDistance;
  }
  int getDeviceMinFlux (const char *device, double &minFlux);

  int getAstrometryTimeout ()
  {
    return astrometryTimeout;
  }

  int getObsProcessTimeout ()
  {
    return astrometryTimeout;
  }

  int getDarkProcessTimeout ()
  {
    return astrometryTimeout;
  }

  int getFlatProcessTimeout ()
  {
    return astrometryTimeout;
  }

  double getCalibrationLunarDist ()
  {
    return calibrationLunarDist;
  }
  int getCalibrationValidTime ()
  {
    return calibrationValidTime;
  }
  int getCalibrationMaxDelay ()
  {
    return calibrationMaxDelay;
  }
  float getCalibrationMinBonus ()
  {
    return calibrationMinBonus;
  }
  float getCalibrationMaxBonus ()
  {
    return calibrationMaxBonus;
  }
  struct ln_lnlat_posn *getObserver ();
  ObjectCheck *getObjectChecker ();

  /**
   * Returns value bellow which SWIFT target will not be considered for observation.
   *
   * @return Heigh bellow which Swift FOV will not be considered (in degrees).
   * 
   * @callergraph
   */
  float getSwiftMinHorizon ()
  {
    return swift_min_horizon;
  }

  /**
   * Returns Swift soft horizon. Swift target, which was selected (because it
   * is above Swift min_horizon), but which have altitude bellow soft horizon,
   * will be assigned altitide of swift horizon.
   * 
   * @callergraph
   */
  float getSwiftSoftHorizon ()
  {
    return swift_soft_horizon;
  }
};

#endif /* !__RTS2_CONFIG__ */
