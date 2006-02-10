#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2targetset.h"
#include "../utils/rts2config.h"

#include <iostream>

class Rts2TargetList:public Rts2AppDb
{
public:
  Rts2TargetList (int argc, char **argv);
    virtual ~ Rts2TargetList (void);

  virtual int processOption (int in_opt);

  virtual int processArgs (const char *arg);
  virtual int init ();
  virtual int run ();
};

Rts2TargetList::Rts2TargetList (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
}

Rts2TargetList::~Rts2TargetList ()
{
}

int
Rts2TargetList::processOption (int in_opt)
{
  switch (in_opt)
    {
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2TargetList::processArgs (const char *arg)
{
  return Rts2AppDb::processArgs (arg);
}

int
Rts2TargetList::init ()
{
  int ret;

  ret = Rts2AppDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();

  return 0;
}

int
Rts2TargetList::run ()
{
  Rts2TargetSetGrb tar_set = Rts2TargetSetGrb ();
  tar_set.printGrbList (std::cout);
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2TargetList *app;
  app = new Rts2TargetList (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}
