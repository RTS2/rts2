#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/target.h"
#include "../utilsdb/rts2obsset.h"
#include "../utils/rts2config.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <stdlib.h>

std::ostream & operator << (std::ostream & _os, struct ln_lnlat_posn *_pos)
{
  struct ln_dms dms;
  ln_deg_to_dms (_pos->lng, &dms);
  _os << std::setw (3) << std::setfill ('0') << dms.degrees << ":"
    << std::setw (2) << std::setfill ('0') << dms.minutes << ":"
    << std::setprecision (4) << dms.seconds;
  return _os;
}

class Rts2TargetInfo:public Rts2AppDb
{
private:
  std::list < int >targets;
    std::list < char *>cameras;
  Target *target;
  struct ln_lnlat_posn *obs;
  int printTargetInfo ();
  int printObservations;
  int printImages;
  int printCounts;
public:
    Rts2TargetInfo (int argc, char **argv);
    virtual ~ Rts2TargetInfo (void);

  virtual int processOption (int in_opt);

  virtual int processArgs (const char *arg);
  virtual int init ();
  virtual int run ();
};

Rts2TargetInfo::Rts2TargetInfo (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  obs = NULL;
  printObservations = 0;
  printImages = 0;
  printCounts = 0;

  addOption ('c', "cameras", 1, "show scripts for given cameras");
  addOption ('o', "observations", 2,
	     "print observations (in given time range)");
  addOption ('i', "images", 2, "print images (in given time range)");
  addOption ('I', "images_summary", 0, "print image summary row");
  addOption ('p', "photometer", 2, "print counts (in given time range)");
  addOption ('P', "photometer_summary", 0, "print counts summary row");
}

Rts2TargetInfo::~Rts2TargetInfo ()
{
  cameras.clear ();
}

int
Rts2TargetInfo::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'c':
      cameras.push_back (optarg);
      break;
    case 'o':
      printObservations = 1;
      break;
    case 'i':
      printImages |= DISPLAY_ALL;
      break;
    case 'I':
      printImages |= DISPLAY_SUMMARY;
      break;
    case 'p':
      printCounts |= DISPLAY_ALL;
      break;
    case 'P':
      printCounts |= DISPLAY_SUMMARY;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2TargetInfo::processArgs (const char *arg)
{
  // try to create that target..
  int tar_id;
  char *test;
  tar_id = strtol (arg, &test, 10);
  if (*test)
    {
      std::cerr << "Invalid target id: " << arg << std::endl;
      return -1;
    }
  targets.push_back (tar_id);
  return 0;
}

int
Rts2TargetInfo::printTargetInfo ()
{
  std::cout << target;
  // print scripts..
  std::list < char *>::iterator cam_names;
  for (cam_names = cameras.begin (); cam_names != cameras.end (); cam_names++)
    {
      char *cam_name = *cam_names;
      int ret;
      char script[MAX_COMMAND_LENGTH];
      ret = target->getScript (cam_name, script);
      std::
	cout << "Script for camera " << cam_name << ":'" << script <<
	"' ret (" << ret << ")" << std::endl;
    }
  // print observations..
  if (printObservations)
    {
      Rts2ObsSet obsSet = Rts2ObsSet (target->getTargetID ());
      if (printImages)
	obsSet.printImages (printImages);
      if (printCounts)
	obsSet.printCounts (printCounts);
      std::cout << obsSet << std::endl;
    }
  else if (printImages || printCounts)
    {
      if (printImages)
	{
	  Rts2ImgSet imgset = Rts2ImgSet (target->getTargetID ());
	  imgset.load ();
	  imgset.print (std::cout, printImages);
	  imgset.clear ();
	}
    }
  return 0;
}

int
Rts2TargetInfo::init ()
{
  int ret;

  ret = Rts2AppDb::init ();
  if (ret)
    return ret;

  Rts2Config *config;
  config = Rts2Config::instance ();

  if (!obs)
    {
      obs = config->getObserver ();
    }

  return 0;
}

int
Rts2TargetInfo::run ()
{
  std::list < int >::iterator tar_iter;

  for (tar_iter = targets.begin (); tar_iter != targets.end (); tar_iter++)
    {
      int tar_id;
      tar_id = *tar_iter;
      target = createTarget (tar_id, obs);
      if (!target)
	{
	  std::cerr << "Cannot find target with id: " << tar_id << std::endl;
	  continue;
	}
      printTargetInfo ();
      delete target;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  int ret;
  Rts2TargetInfo *app;
  app = new Rts2TargetInfo (argc, argv);
  ret = app->init ();
  if (ret)
    return 1;
  app->run ();
  delete app;
}
