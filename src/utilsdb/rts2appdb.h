#ifndef __RTS2_APPDB__
#define __RTS2_APPDB__

#include "../utils/rts2app.h"

class Rts2AppDb:public Rts2App
{
  char *connectString;
  char *configFile;
public:
    Rts2AppDb (int argc, char **argv);
    virtual ~ Rts2AppDb (void);

  virtual int processOption (int in_opt);
  int initDB ();
  virtual int init ();
};

#endif /* !__RTS2_APPDB__ */
