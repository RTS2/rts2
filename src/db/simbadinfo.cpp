#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2askchoice.h"

#include "rts2targetapp.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>
#include <sstream>

class Rts2SimbadInfo:public Rts2TargetApp
{
protected:
  virtual int processOption (int in_opt);

public:
    Rts2SimbadInfo (int in_argc, char **in_argv);
    virtual ~ Rts2SimbadInfo (void);

  virtual int doProcessing ();
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
Rts2SimbadInfo::doProcessing ()
{
  static double radius = 10.0 / 60.0;
  int ret;
  // ask for target ID..
  std::cout << "Default values are written at [].." << std::endl;
  ret = askForObject ("Target name, RA&DEC or anything else");
  if (ret)
    return ret;

  std::cout << target;

  Rts2AskChoice selection = Rts2AskChoice (this);
  selection.addChoice ('s', "Save");
  selection.addChoice ('q', "Quit");
  selection.addChoice ('o', "List observations around position");
  selection.addChoice ('t', "List targets around position");
  selection.addChoice ('i', "List images around position");

  while (1)
    {
      char sel_ret;
      sel_ret = selection.query (std::cout);
      switch (sel_ret)
	{
	case 's':
	case 'q':
	  return 0;
	case 'o':
	  askForDegrees ("Radius", radius);
	  target->printObservations (radius, std::cout);
	  break;
	case 't':
	  askForDegrees ("Radius", radius);
	  target->printTargets (radius, std::cout);
	  break;
	case 'i':
	  target->printImages (std::cout);
	  break;
	}
    }
}

int
main (int argc, char **argv)
{
  Rts2SimbadInfo app = Rts2SimbadInfo (argc, argv);
  return app.run ();
}
