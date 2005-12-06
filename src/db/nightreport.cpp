#include "../utils/rts2config.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2obsset.h"

#include <time.h>
#include <iostream>

class Rts2Night:public Rts2AppDb
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
    Rts2Night (int argc, char **argv);
    virtual ~ Rts2Night (void);

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int run ();
};

Rts2Night::Rts2Night (int in_argc, char **in_argv):
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
  addOption ('p', "photometer", 0, "print counts listing");
  addOption ('P', "photometer_summary", 0, "print counts summary row");
  addOption ('s', "statistics", 0, "print night statistics");
}

Rts2Night::~Rts2Night (void)
{
  delete tm_night;
  delete obs_set;
}

int
Rts2Night::processOption (int in_opt)
{
  int ret;
  struct tm tm_ret;
  switch (in_opt)
    {
    case 'f':
      ret = parseDate (optarg, &tm_ret);
      if (ret)
	return ret;
      t_from = mktime (&tm_ret);
      break;
    case 't':
      ret = parseDate (optarg, &tm_ret);
      if (ret)
	return ret;
      t_to = mktime (&tm_ret);
      break;
    case 'n':
      tm_night = new struct tm;
      ret = parseDate (optarg, tm_night);
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
      printCounts |= DISPLAY_ALL;
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
Rts2Night::init ()
{
  int ret;
  ret = Rts2AppDb::init ();
  if (ret)
    return ret;

  if (tm_night)
    {
      // let's calculate time from..t_from will contains start of night
      // local 12:00 will be at ~ give time..
      tm_night->tm_hour =
	(int) ln_range_degrees (180.0 -
				Rts2Config::instance ()->getObserver ()->
				lng) / 15;
      if (tm_night->tm_hour > 12)
	tm_night->tm_hour = 24 - tm_night->tm_hour;
      tm_night->tm_min = tm_night->tm_sec = 0;
      t_from = mktime (tm_night);
      if (t_from + 86400 > t_to)
	t_from -= 86400;
      t_to = t_from + 86400;
      if (t_from > t_to)
	t_from -= 86400;
    }
  return 0;
}

void
Rts2Night::printObsList ()
{
  if (printImages)
    obs_set->printImages (printImages);
  if (printCounts)
    obs_set->printCounts (printCounts);
  std::cout << *obs_set;
}


void
Rts2Night::printStatistics ()
{
  obs_set->printStatistics (std::cout);
}

int
Rts2Night::run ()
{
//  char *whereStr;
  Rts2Config *config;
//  Rts2SqlColumnObsState *obsState;
  config = Rts2Config::instance ();
  // from which date to which..
  std::cout << "From " << &t_from << " to " << &t_to << std::endl;

  obs_set = new Rts2ObsSet (&t_from, &t_to);

  printObsList ();

  if (printStat)
    printStatistics ();

  return 0;
}

Rts2Night *app;


int
main (int argc, char **argv)
{
  int ret;
  app = new Rts2Night (argc, argv);
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
