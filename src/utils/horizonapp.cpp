#include "rts2app.h"
#include "rts2config.h"

#include <iostream>

/**
 * Class which will plot horizon from horizon file, possibly with
 * commands for GNUPlot.
 *
 * (C) 2007 Petr Kubanek <petr@kubanek.net>
 */

class HorizonApp:public Rts2App
{

public:
  HorizonApp (int in_argc, char **in_argv);

  virtual int init ();
  virtual int run ();
};

HorizonApp::HorizonApp (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
}

int
HorizonApp::init ()
{
  int ret;

  ret = Rts2App::init ();
  if (ret)
    return ret;

  ret = Rts2Config::instance ()->loadFile ();
  if (ret)
    return ret;

  return 0;
}

int
HorizonApp::run ()
{
  struct ln_hrz_posn hrz;

  hrz.alt = 0;

  ObjectCheck *checker = Rts2Config::instance ()->getObjectChecker ();

  std::cout
    << "set terminal x11 persist" << std::endl
    << " set xrange [0:360]" << std::endl;

  std::
    cout << "plot \"-\" u 1:2 smooth csplines lt 2 lw 2 t \"Horizon\"" <<
    std::endl;

  for (hrz.az = 0; hrz.az <= 360; hrz.az += 0.1)
    {
      std::cout << hrz.az << " " << checker->getHorizonHeight (&hrz,
							       0) << std::
	endl;
    }
  std::cout << "e" << std::endl;
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;
  HorizonApp app = HorizonApp (argc, argv);
  ret = app.init ();
  if (ret)
    return ret;
  return app.run ();
}
