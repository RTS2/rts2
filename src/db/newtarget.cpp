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

class Rts2NewTarget:public Rts2TargetApp
{
public:
  Rts2NewTarget (int in_argc, char **in_argv);
    virtual ~ Rts2NewTarget (void);

  virtual int processOption (int in_opt);

  virtual int run ();
};


Rts2NewTarget::Rts2NewTarget (int in_argc, char **in_argv):
Rts2TargetApp (in_argc, in_argv)
{
}

Rts2NewTarget::~Rts2NewTarget (void)
{
}


int
Rts2NewTarget::processOption (int in_opt)
{
  switch (in_opt)
    {
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2NewTarget::run ()
{
  int n_tar_id = 0;
  int ret;
  std::string target_name;
  // ask for target ID..
  std::cout << "Default values are written at [].." << std::endl;
  ret = askForObject ("Target name, RA&DEC or anything else");
  if (ret)
    return ret;
  Rts2AskChoice selection = Rts2AskChoice (this);
  selection.addChoice ('s', "Save");
  selection.addChoice ('q', "Quit");
  selection.addChoice ('o', "List observations around position");
  selection.addChoice ('t', "List targets around position");
  while (1)
    {
      char sel_ret;
      sel_ret = selection.query (std::cout);
      // 10 arcmin radius
      double radius = 10.0 / 60.0;
      if (sel_ret == 's')
	break;
      switch (sel_ret)
	{
	case 'q':
	  return 0;
	case 'o':
	  askForDouble ("Radius", radius);
	  target->printObservations (radius, std::cout);
	  break;
	case 't':
	  askForDouble ("Radius", radius);
	  target->printTargets (radius, std::cout);
	  break;
	}
    }
  askForInt ("Target ID", n_tar_id);
  target_name = target->getTargetName ();
  askForString ("Target NAME", target_name);
  target->setTargetName (target_name.c_str ());

  target->setTargetType (TYPE_OPORTUNITY);
  if (n_tar_id > 0)
    ret = target->save (n_tar_id);
  else
    ret = target->save ();

  std::cout << target;

  if (ret)
    {
      std::cerr << "Error when saving target." << std::endl;
    }
  return ret;
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2NewTarget *app;
  app = new Rts2NewTarget (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}
