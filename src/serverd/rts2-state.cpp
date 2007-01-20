/* 
 * State - display day states for rts2.
 * Copyright (C) 2003 Petr Kubanek <petr@kubanek,net>
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

#include <time.h>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "riseset.h"
#include "../utils/libnova_cpp.h"
#include "../utils/rts2app.h"
#include "../utils/rts2config.h"
#include "../utils/rts2centralstate.h"

/*!
 * Prints current state on STDERR
 * States are (as described in ../../include/state.h):
 * 
 * 0 - day
 * 1 - evening
 * 2 - dusk
 * 3 - night
 * 4 - dawn
 * 5 - morning
 *
 * Configuration file CONFIG_FILE is used
 */
class Rts2StateApp:public Rts2App
{
private:
  int verbose;			// verbosity level
  double lng;
  double lat;
  time_t currTime;
  double JD;
  void printAltTable (std::ostream & _os);
  void printDayStates (std::ostream & _os);
protected:
    virtual void help ();
  virtual int processOption (int in_opt);
public:
    Rts2StateApp (int in_argc, char **in_argv);

  virtual int init ();
  virtual int run ();
};

void
Rts2StateApp::printAltTable (std::ostream & _os)
{
  int i;
  double jd_start = ((int) JD) - 0.5;
  struct ln_hrz_posn hrz;
  struct ln_equ_posn pos;
  double jd;
  int old_precison = _os.precision (0);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);

  _os
    << std::endl
    << "** SOON POSITION table from "
    << LibnovaDate (jd_start) << " **" << std::endl << std::endl << "H   ";

  char old_fill = _os.fill ('0');
  for (i = 0; i <= 24; i++)
    {
      _os << "  " << std::setw (2) << i;
    }
  _os.fill (old_fill);
  _os << std::endl;

  // print sun position
  std::ostringstream _os3;

  _os << "SAL ";
  _os3 << "SAZ ";

  _os3.precision (0);
  _os3.setf (std::ios_base::fixed, std::ios_base::floatfield);

  jd = jd_start;
  for (i = 0; i <= 24; i++, jd += 1.0 / 24.0)
    {
      ln_get_solar_equ_coords (jd, &pos);
      ln_get_hrz_from_equ (&pos, Rts2Config::instance ()->getObserver (), jd,
			   &hrz);
      _os << " " << std::setw (3) << hrz.alt;
      _os3 << " " << std::setw (3) << hrz.az;
    }

  _os << std::endl << _os3.str () << std::endl << std::endl;

  _os.setf (old_settings);
  _os.precision (old_precison);
}

void
Rts2StateApp::printDayStates (std::ostream & _os)
{
  time_t ev_time, curr_time;
  int curr_type, next_type;

  curr_time = currTime;

  while (curr_time < (currTime + 87000.0))
    {
      if (next_event
	  (Rts2Config::instance ()->getObserver (), &curr_time, &curr_type,
	   &next_type, &ev_time))
	{
	  std::cerr << "Error getting next type" << std::endl;
	  return;
	}
      if (curr_time == currTime)
	{
	  _os
	    << LibnovaDate (&currTime)
	    << " " << Rts2CentralState (curr_type)
	    << " (current)" << std::endl;
	}
      _os
	<< LibnovaDate (&ev_time)
	<< " " << Rts2CentralState (next_type) << std::endl;

      curr_time = ev_time + 1;
    }
}


void
Rts2StateApp::help ()
{
  std::cout << "Observing state display tool." << std::endl;
  Rts2App::help ();
}

int
Rts2StateApp::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'a':
      lat = atof (optarg);
      break;
    case 'c':
      verbose = -1;
      break;
    case 'l':
      lng = atof (optarg);
      break;
    case 't':
      currTime = atoi (optarg);
      if (!currTime)
	{
	  std::cerr << "Invalid time: " << optarg << std::endl;
	  return -1;
	}
      break;
    case 'v':
      verbose++;
      break;
    default:
      return Rts2App::processOption (in_opt);
    }
  return 0;
}

Rts2StateApp::Rts2StateApp (int in_argc, char **in_argv):Rts2App (in_argc,
	 in_argv)
{
  lng = lat = -1000;
  verbose = 0;

  time (&currTime);

  addOption ('a', "latitude", 1, "set latitude (overwrites config file)");
  addOption ('l', "longtitude", 1,
	     "set longtitude (overwrites config file). Negative for west from Greenwich)");
  addOption ('c', "clear", 0,
	     "just print current state (one number) and exists");
  addOption ('t', "time", 1, "set time (int unix time)");
  addOption ('v', "verbose", 0, "be verbose");
}

int
Rts2StateApp::init ()
{
  Rts2Config *config;
  int ret;

  ret = Rts2App::init ();
  if (ret)
    return ret;

  JD = ln_get_julian_from_timet (&currTime);

  config = Rts2Config::instance ();

  if (config->loadFile () == -1)
    {
      std::cerr << "Cannot read configuration file, exiting." << std::endl;
      return -1;
    }

  if (lng != -1000)
    config->getObserver ()->lng = lng;
  if (lat != -1000)
    config->getObserver ()->lat = lat;

  return 0;
}

int
Rts2StateApp::run ()
{
  int ret;
  time_t ev_time;
  int curr_type, next_type;

  struct ln_lnlat_posn *obs;

  ret = init ();
  if (ret)
    return ret;

  obs = Rts2Config::instance ()->getObserver ();

  if (verbose > 0)
    std::cout
      << "Position: "
      << LibnovaPos (obs)
      << " Time: " << LibnovaDate (&currTime) << std::endl;

  if (next_event (obs, &currTime, &curr_type, &next_type, &ev_time))
    {
      std::cerr << "Error getting next type" << std::endl;
      return -1;
    }
  switch (verbose)
    {
    case -1:
      std::cout << curr_type << std::endl;
      break;
    default:
      printAltTable (std::cout);
      printDayStates (std::cout);
      std::cout << std::endl;
      break;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  Rts2StateApp app (argc, argv);
  return app.run ();
}
