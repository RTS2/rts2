#ifndef __RTS2_CLIAPP__
#define __RTS2_CLIAPP__

#include "rts2app.h"

class Rts2CliApp:public Rts2App
{
protected:
  virtual int doProcessing () = 0;
  virtual void afterProcessing ();

public:
    Rts2CliApp (int in_argc, char **in_argv);

  virtual int run ();
};

#endif /* !__RTS2_CLIAPP__ */
