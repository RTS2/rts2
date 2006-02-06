#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2plan.h"
#include "../utilsdb/rts2planset.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2config.h"

#include <signal.h>
#include <iostream>

#include <list>

class Rts2PlanApp:public Rts2AppDb
{
private:
  enum
  { NO_OP, OP_DUMP, OP_LOAD, OP_GENERATE } operation;
  int dumpPlan ();
  int loadPlan ();
  struct tm *tm_night;
protected:
    virtual int processOption (int in_opt);
public:
    Rts2PlanApp (int in_argc, char **in_argv);
    virtual ~ Rts2PlanApp (void);

  virtual int run ();
};

Rts2PlanApp::Rts2PlanApp (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  Rts2Config *config;
  config = Rts2Config::instance ();

  operation = NO_OP;
  tm_night = NULL;

  addOption ('n', "night", 1, "work with this night");
  addOption ('D', "dump_plan", 0, "dump plan to standart output");
  addOption ('L', "load_plan", 0, "load plan from standart input");
  addOption ('G', "generate", 0, "generate plan based on targets");
}

Rts2PlanApp::~Rts2PlanApp (void)
{
  delete tm_night;
}

int
Rts2PlanApp::dumpPlan ()
{
  Rts2PlanSet *plan_set;
  if (tm_night)
    {
      Rts2Night night =
	Rts2Night (tm_night, Rts2Config::instance ()->getObserver ());
      plan_set = new Rts2PlanSet (night.getFrom (), night.getTo ());
      std::cout << "Night " << night << std::endl;
    }
  else
    {
      plan_set = new Rts2PlanSet ();
    }
  std::cout << (*plan_set) << std::endl;
  delete plan_set;
  return 0;
}

int
Rts2PlanApp::loadPlan ()
{
  while (1)
    {
      Rts2Plan plan;
      int ret;
      std::cin >> plan;
      if (std::cin.fail ())
	break;
      ret = plan.save ();
      if (ret)
	std::cerr << "Error loading plan" << std::endl;
    }
  return 0;
}

int
Rts2PlanApp::processOption (int in_opt)
{
  int ret;
  switch (in_opt)
    {
    case 'n':
      tm_night = new struct tm;
      ret = parseDate (optarg, tm_night);
      if (ret)
	return ret;
      break;
    case 'D':
      if (operation != NO_OP)
	return -1;
      operation = OP_DUMP;
      break;
    case 'L':
      if (operation != NO_OP)
	return -1;
      operation = OP_LOAD;
      break;
    case 'G':
      if (operation != NO_OP)
	return -1;
      operation = OP_GENERATE;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2PlanApp::run ()
{
  switch (operation)
    {
    case NO_OP:
      std::cout << "Please select mode of operation.." << std::endl;
      // interactive..
      return 0;
    case OP_DUMP:
      return dumpPlan ();
    case OP_LOAD:
      return loadPlan ();
    case OP_GENERATE:

      return 0;
    }
  return -1;
}

Rts2PlanApp *app;

void
killSignal (int sig)
{
  if (app)
    delete app;
}

int
main (int argc, char **argv)
{
  int ret;
  app = new Rts2PlanApp (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = app->init ();
  if (ret)
    {
      delete app;
      return 0;
    }
  ret = app->run ();
  delete app;
  return ret;
}
