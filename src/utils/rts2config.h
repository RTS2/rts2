#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__
/**
 * Class which enable access to configuration values, stored in ini file.
 *
 * @author petr
 */

#include <stdio.h>
#include <libnova/libnova.h>
#include <string>

#include "objectcheck.h"

class Rts2Config
{
private:
  FILE * fp;
  static Rts2Config *pInstance;

  struct ln_lnlat_posn observer;
  ObjectCheck *checker;
  double calibrationAirmassDistance;
  double calibrationLunarDist;
  int calibrationValidTime;
  int calibrationMaxDelay;
  float calibrationMinBonus;
  float calibrationMaxBonus;
public:
    Rts2Config ();
    virtual ~ Rts2Config (void);
  static Rts2Config *instance ();
  int loadFile (char *filename = NULL);
  int getString (const char *section, const char *param, char *buf, int bufl);
  int getString (const char *section, const char *param, std::string ** buf);
  int getInteger (const char *section, const char *param, int &value);
  int getFloat (const char *section, const char *param, float &value);
  int getDouble (const char *section, const char *param, double &value);
  int getBoolean (const char *section, const char *param);
  void getBoolean (const char *section, const char *param, bool & value);

  // some special functions..
  struct ln_lnlat_posn *getObserver ();
  ObjectCheck *getObjectChecker ();
  double getCalibrationAirmassDistance ()
  {
    return calibrationAirmassDistance;
  }
  int getDeviceMinFlux (const char *device, double &minFlux);
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
};

#endif /*! __RTS2_CONFIG__ */
