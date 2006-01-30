#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"

#include "rts2targetapp.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>
#include <sstream>

class Rts2SimbadInfo:public Rts2TargetApp
{
public:
  Rts2SimbadInfo (int in_argc, char **in_argv);
    virtual ~ Rts2SimbadInfo (void);

  virtual int processOption (int in_opt);

  virtual int run ();
};

Rts2SimbadInfo::Rts2SimbadInfo (int in_argc, char **in_argv):
Rts2TargetApp (in_argc, in_argv)
{
}

Rts2SimbadInfo::~Rts2SimbadInfo ()
{
}

int
Rts2SimbadInfo::processOption (int in_opt)
{
  switch (in_opt)
    {
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2SimbadInfo::run ()
{
  int ret;
  // ask for target ID..
  std::cout << "Default values are written at [].." << std::endl;
  ret = askForObject ("Target name, RA&DEC or anything else");
  if (ret)
    return ret;

  std::cout << target;

  return ret;
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2SimbadInfo *app;
  app = new Rts2SimbadInfo (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}
