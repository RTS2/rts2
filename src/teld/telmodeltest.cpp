#include "model/telmodel.h"

#include <iostream>

class Rts2DevTelescopeModelTest:public Rts2DevTelescope
{
public:
  Rts2DevTelescopeModelTest ():Rts2DevTelescope (0, NULL)
  {
  }
};

class TelModelTest:public Rts2App
{
private:
  char *modelFile;
  Rts2TelModel *model;

  void test (double ra, double dec);

protected:
    virtual int processOption (int in_opt);

public:
    TelModelTest (int in_argc, char **in_argv);

  virtual int init ();
  virtual int run ();
};

TelModelTest::TelModelTest (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
  Rts2Config *config;
  config = Rts2Config::instance ();
  modelFile = NULL;
  model = NULL;
  addOption ('m', "model-file", 1, "Model file to use");
}

int
TelModelTest::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'm':
      modelFile = optarg;
      break;
    default:
      return Rts2App::processOption (in_opt);
    }
  return 0;
}

int
TelModelTest::init ()
{
  int ret;
  ret = Rts2App::init ();
  if (ret)
    return ret;

  model = new Rts2TelModel (new Rts2DevTelescopeModelTest (), modelFile);
  ret = model->load ();
  return ret;
}

void
TelModelTest::test (double ra, double dec)
{
  struct ln_equ_posn pos;
  pos.ra = ra;
  pos.dec = dec;
  model->apply (&pos);
  std::cout << "RA: " << ra << " DEC: " << dec << " -> " << pos.
    ra << " " << pos.dec << std::endl;
}

int
TelModelTest::run ()
{
  test (10, 20);
  test (30, 50);
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;
  TelModelTest app (argc, argv);
  ret = app.init ();
  if (ret)
    return ret;
  return app.run ();
}
