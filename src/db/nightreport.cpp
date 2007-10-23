/* 
 * Night reporting utility.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek,net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "imgdisplay.h"
#include "../utils/rts2config.h"
#include "../utils/libnova_cpp.h"
#include "../utilsdb/rts2appdb.h"
#include "../utilsdb/rts2obsset.h"

#include "imgdisplay.h"

#include <time.h>
#include <iostream>

/**
 * Produces night report output.
 *
 * @addgroup RTS2DbApps
 */
class Rts2NightReport:public Rts2AppDb
{
private:
  time_t t_from, t_to;
  struct ln_date *tm_night;
  int printImages;
  int printCounts;
  int printStat;
  void printObsList ();
  void printStatistics ();
  Rts2ObsSet *obs_set;

protected:
    virtual int processOption (int in_opt);
  virtual int init ();

public:
    Rts2NightReport (int argc, char **argv);
    virtual ~ Rts2NightReport (void);

  virtual int doProcessing ();
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
  addOption ('l', NULL, 0, "print full image names");
  addOption ('i', NULL, 0, "print image listing");
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
  switch (in_opt)
    {
    case 'f':
      return parseDate (optarg, &t_from);
    case 't':
      return parseDate (optarg, &t_to);
      break;
    case 'n':
      tm_night = new struct ln_date;
      return Rts2CliApp::parseDate (optarg, tm_night);
    case 'l':
      printImages |= DISPLAY_FILENAME;
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
	  if (!strcmp (optarg, "txt"))
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
Rts2NightReport::doProcessing ()
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

int
main (int argc, char **argv)
{
  Rts2NightReport app = Rts2NightReport (argc, argv);
  return app.run ();
}
