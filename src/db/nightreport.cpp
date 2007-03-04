#include "imgdisplay.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2obsset.h"

#include "imgdisplay.h"

#include <time.h>
#include <iostream>

class Rts2NightReport:public Rts2AppDb
{
private:
  time_t t_from, t_to;
  struct tm *tm_night;
  int printImages;
  int printCounts;
  int printStat;
  void printObsList ();
  void printStatistics ();
  Rts2ObsSet *obs_set;
public:
    Rts2NightReport (int argc, char **argv);
    virtual ~ Rts2NightReport (void);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int run ();
};

Rts2NightReport::Rts2NightReport (int in_argc, char **in_argv):
Rts2AppDb (in_argc, in_argv)
{
  time (&t_to);
  // 24 hours before..
  t_from = t_to - 86400;
  tm_night = NULL;
  obs_set = NULL;
  printImages = 0;
  printCounts = 0;
  printStat = 0;
  addOption ('f', "from", 1,
	     "date from which take measurements; default to current date - 24 hours");
  addOption ('t', "to", 1,
	     "date to which show measurements; default to from + 24 hours");
  addOption ('n', "night-date", 1, "report for night around given date");
  addOption ('i', "images", 0, "print image listing");
  addOption ('I', "images_summary", 0, "print image summary row");
  addOption ('p', "photometer", 2,
	     "print counts listing; can be followed by format, txt for plain");
  addOption ('P', "photometer_summary", 0, "print counts summary row");
  addOption ('s', "statistics", 0, "print night statistics");
}

Rts2NightReport::~Rts2NightReport (void)
{
  delete tm_night;
  delete obs_set;
}

int
Rts2NightReport::processOption (int in_opt)
{
  int ret;
  struct tm tm_ret;
  switch (in_opt)
    {
    case 'f':
      ret = Rts2App::parseDate (optarg, &tm_ret);
      if (ret)
	return ret;
      t_from = mktime (&tm_ret);
      break;
    case 't':
      ret = Rts2App::parseDate (optarg, &tm_ret);
      if (ret)
	return ret;
      t_to = mktime (&tm_ret);
      break;
    case 'n':
      tm_night = new struct tm;
      ret = Rts2App::parseDate (optarg, tm_night);
      if (ret)
	return ret;
      break;
    case 'i':
      printImages |= DISPLAY_ALL;
      break;
    case 'I':
      printImages |= DISPLAY_SUMMARY;
      break;
    case 'p':
      if (optarg)
	{
	  if (strcmp (optarg, "txt"))
	    printCounts = DISPLAY_SHORT;
	  else
	    printCounts |= DISPLAY_ALL;
	}
      else
	{
	  printCounts |= DISPLAY_ALL;
	}
      break;
    case 'P':
      printCounts |= DISPLAY_SUMMARY;
      break;
    case 's':
      printStat = 1;
      break;
    default:
      return Rts2AppDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2NightReport::init ()
{
  int ret;
  ret = Rts2AppDb::init ();
  if (ret)
    return ret;

  if (tm_night)
    {
      // let's calculate time from..t_from will contains start of night
      // local 12:00 will be at ~ give time..
      Rts2Night night =
	Rts2Night (tm_night, Rts2Config::instance ()->getObserver ());
      t_from = *(night.getFrom ());
      t_to = *(night.getTo ());
    }
  return 0;
}

void
Rts2NightReport::printObsList ()
{
  if (printImages)
    obs_set->printImages (printImages);
  if (printCounts)
    obs_set->printCounts (printCounts);
  std::cout << *obs_set;
}


void
Rts2NightReport::printStatistics ()
{
  obs_set->printStatistics (std::cout);
}

int
Rts2NightReport::run ()
{
//  char *whereStr;
  Rts2Config *config;
//  Rts2SqlColumnObsState *obsState;
  config = Rts2Config::instance ();
  // from which date to which..
  std::
    cout << "From " << Timestamp (t_from) << " to " << Timestamp (t_to) <<
    std::endl;

  obs_set = new Rts2ObsSet (&t_from, &t_to);

  printObsList ();

  if (printStat)
    printStatistics ();

  return 0;
}

Rts2NightReport *app;


int
main (int argc, char **argv)
{
  int ret;
  app = new Rts2NightReport (argc, argv);
  ret = app->init ();
  if (ret)
    {
      std::cerr << "Cannot init app" << std::endl;
      return 1;
    }
  ret = app->run ();
  delete app;
  return ret;
}
