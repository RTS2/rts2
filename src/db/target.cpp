#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2targetset.h"
#include "../utils/rts2askchoice.h"
#include "../utils/rts2config.h"

#include <iostream>

#define OP_NONE		0x00
#define OP_ENABLE	0x03
#define OP_DISABLE	0x01
#define OP_MASK_EN	0x03
#define OP_PRIORITY	0x04
#define OP_BONUS	0x08
#define OP_BONUS_TIME	0x10
#define OP_SCRIPT	0x20

class CamScript
{
public:
  const char *cameraName;
  const char *script;

    CamScript (const char *in_cameraName, const char *in_script)
  {
    cameraName = in_cameraName;
    script = in_script;
  }
};

class Rts2TargetApp:public Rts2AppDb
{
private:
  int op;
    std::list < int >tar_ids;
  Rts2TargetSet *target_set;

  float new_priority;
  float new_bonus;
  time_t new_bonus_time;

  const char *camera;
    std::list < CamScript > new_scripts;

  int runInteractive ();
public:
    Rts2TargetApp (int argc, char **argv);
    virtual ~ Rts2TargetApp (void);

  virtual int processOption (int in_opt);

  virtual int processArgs (const char *arg);
  virtual int init ();
  virtual int run ();
};

Rts2TargetApp::Rts2TargetApp (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  target_set = NULL;
  op = OP_NONE;
  new_priority = nan ("f");
  new_bonus = nan ("f");

  camera = NULL;

  addOption ('e', "enable", 0, "enable given targets");
  addOption ('d', "disable", 0,
	     "disable given targets (they will not be picked up by selector");
  addOption ('p', "priority", 1, "set target (fixed) priority");
  addOption ('b', "bonus", 1, "set target bonus to this value");
  addOption ('B', "bonus_time", 1, "set target bonus time to this value");
  addOption ('C', "camera", 1, "next script will be set for given camera");
  addOption ('s', "script", 1, "set script for target and camera");
}

Rts2TargetApp::~Rts2TargetApp ()
{
  delete target_set;
}

int
Rts2TargetApp::processOption (int in_opt)
{
  int ret;
  struct tm tm_ret;
  switch (in_opt)
    {
    case 'e':
      if (op & OP_DISABLE)
	return -1;
      op |= OP_ENABLE;
      break;
    case 'd':
      if (op & OP_ENABLE)
	return -1;
      op |= OP_DISABLE;
      break;
    case 'p':
      new_priority = atof (optarg);
      op |= OP_PRIORITY;
      break;
    case 'b':
      new_bonus = atof (optarg);
      op |= OP_BONUS;
      break;
    case 'B':
      ret = Rts2App::parseDate (optarg, &tm_ret);
      if (ret)
	return ret;
      new_bonus_time = mktime (&tm_ret);
      op |= OP_BONUS_TIME;
      break;
    case 'C':
      if (camera)
	return -1;
      camera = optarg;
      break;
    case 's':
      if (!camera)
	return -1;
      new_scripts.push_back (CamScript (camera, optarg));
      camera = NULL;
      op |= OP_SCRIPT;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2TargetApp::processArgs (const char *arg)
{
  int new_tar_id;
  char *endptr;

  new_tar_id = strtol (arg, &endptr, 10);
  if (*endptr != '\0')
    return -1;

  tar_ids.push_back (new_tar_id);
  return 0;
}

int
Rts2TargetApp::init ()
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
Rts2TargetApp::runInteractive ()
{
  Rts2AskChoice selection = Rts2AskChoice (this);
  selection.addChoice ('e', "Enable target(s)");
  selection.addChoice ('d', "Disable target(s)");
  selection.addChoice ('o', "List observations around position");
  selection.addChoice ('t', "List targets around position");
  selection.addChoice ('n', "Choose new target");
  selection.addChoice ('s', "Save");
  selection.addChoice ('q', "Quit");
  while (1)
    {
      char sel_ret;
      sel_ret = selection.query (std::cout);
      switch (sel_ret)
	{
	case 'e':

	  break;
	case 'd':

	  break;
	case 'o':

	  break;
	case 't':

	  break;
	case 'n':

	  break;
	case 'q':
	  return 0;
	case 's':
	  return target_set->save ();
	}
    }
}

int
Rts2TargetApp::run ()
{
  target_set = new Rts2TargetSet (tar_ids);
  if ((op & OP_MASK_EN) == OP_ENABLE)
    {
      target_set->setTargetEnabled (true);
    }
  if ((op & OP_MASK_EN) == OP_DISABLE)
    {
      target_set->setTargetEnabled (false);
    }
  if (op & OP_PRIORITY)
    {
      target_set->setTargetPriority (new_priority);
    }
  if (op & OP_BONUS)
    {
      target_set->setTargetBonus (new_bonus);
    }
  if (op & OP_BONUS_TIME)
    {
      target_set->setTargetBonusTime (&new_bonus_time);
    }
  if (op & OP_SCRIPT)
    {
      for (std::list < CamScript >::iterator iter = new_scripts.begin ();
	   iter != new_scripts.end (); iter++)
	{
	  target_set->setTargetScript (iter->cameraName, iter->script);
	}
    }

  if (op == OP_NONE)
    {
      return runInteractive ();
    }

  return target_set->save ();
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2TargetApp *app;
  app = new Rts2TargetApp (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  ret = app->run ();
  delete app;
  return ret;
}
