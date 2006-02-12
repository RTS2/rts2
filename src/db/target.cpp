#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2targetset.h"
#include "../utils/rts2config.h"

#include <iostream>

#define OP_ENABLE	0x03
#define OP_DISABLE	0x01
#define OP_MASK_EN	0x03
#define OP_BONUS	0x04
#define OP_BONUS_TIME	0x08

class Rts2Target:public Rts2AppDb
{
  int op;
    std::list < int >tar_ids;
public:
    Rts2Target (int argc, char **argv);
    virtual ~ Rts2Target (void);

  virtual int processOption (int in_opt);

  virtual int processArgs (const char *arg);
  virtual int init ();
  virtual int run ();
};

Rts2Target::Rts2Target (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  op = 0;
  addOption ('e', "enable", 0, "enable given targets");
  addOption ('d', "disable", 0,
	     "disable given targets (they will not be picked up by selector");
  addOption ('b', "bonus", 1, "set target bonus to this value");
  addOption ('B', "bonus_time", 1, "set target bonus time to this value");
}

Rts2Target::~Rts2Target ()
{
}

int
Rts2Target::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'e':
      if (op & OP_DISABLE)
	return -1;
      op |= OP_ENABLE;
      return 0;
    case 'd':
      if (op & OP_ENABLE)
	return -1;
      op |= OP_DISABLE;
      return 0;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2Target::processArgs (const char *arg)
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
Rts2Target::init ()
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
Rts2Target::run ()
{
  Rts2TargetSet target_set = Rts2TargetSet (tar_ids);
  if ((op & OP_MASK_EN) == OP_ENABLE)
    {
      target_set.setTargetEnabled (true);
    }
  if ((op & OP_MASK_EN) == OP_DISABLE)
    {
      target_set.setTargetEnabled (false);
    }

  return target_set.save ();
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2Target *app;
  app = new Rts2Target (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  ret = app->run ();
  delete app;
  return ret;
}
