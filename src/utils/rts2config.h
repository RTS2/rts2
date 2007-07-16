#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__

#include "rts2configraw.h"
#include "objectcheck.h"

#include <libnova/libnova.h>

/**
 * Represent full Config class, which includes support for Libnova types and holds some special values.
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
};

#endif /* !__RTS2_CONFIG__ */
