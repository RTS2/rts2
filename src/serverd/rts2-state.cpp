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

//#include <stdio.h>
#include <time.h>
//#include <stdlib.h>
#include <iostream>
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
protected:
    virtual void help ();
  virtual int processOption (int in_opt);
public:
    Rts2StateApp (int in_argc, char **in_argv);

  virtual int init ();
  virtual int run ();
};

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
    std::cout << "Position: " << LibnovaPos (obs)
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
      std::cout
	<< "Current: " << Rts2CentralState (curr_type)
	<< " " << LibnovaDate (&currTime)
	<< " next: " << Rts2CentralState (next_type)
	<< " " << LibnovaDate (&ev_time) << std::endl;
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
