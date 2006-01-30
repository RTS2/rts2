#ifndef __RTS2_TARGETAPP__
#define __RTS2_TARGETAPP__

#include <libnova/libnova.h>

#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"

class Rts2TargetApp:public Rts2AppDb
{
protected:
  Target * target;
  struct ln_lnlat_posn *obs;
  int askForObject (const char *desc);
public:
    Rts2TargetApp (int argc, char **argv);
    virtual ~ Rts2TargetApp (void);

  virtual int init ();
};

#endif /* !__RTS2_TARGETAPP__ */
