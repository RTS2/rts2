#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__
/**
 * Class which enable access to configuration values, stored in ini file.
 *
 * @author petr
 */

#include <stdio.h>
#include <libnova/libnova.h>

class Rts2Config
{
private:
  FILE * fp;
  static Rts2Config *pInstance;

  struct ln_lnlat_posn observer;

public:
    Rts2Config ();
    virtual ~ Rts2Config (void);
  static Rts2Config *instance ();
  int loadFile (char *filename = NULL);
  int getString (const char *section, const char *param, char *buf, int bufl);
  int getInteger (const char *section, const char *param, int &value);
  int getDouble (const char *section, const char *param, double &value);
  int getBoolean (const char *section, const char *param);

  // some special functions..
  struct ln_lnlat_posn *getObserver ();
};

#endif /*! __RTS2_CONFIG__ */
