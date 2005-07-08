#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__
/**
 * Class which enable access to configuration values, stored in ini file.
 *
 * @author petr
 */

#include <stdio.h>

class Rts2Config
{
private:
  FILE * fp;
public:
  Rts2Config ();
  virtual ~ Rts2Config (void);
  int loadFile (char *filename);
  int getString (char *section, char *param, char *buf, int bufl);
  int getInteger (char *section, char *param, int &value);
  int getDouble (char *section, char *param, double &value);
};

#endif /*! __RTS2_CONFIG__ */
