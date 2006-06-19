/**
 * Copyright (C) 2005 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "filterd.h"
#include "libfli.h"
#include "libfli-filter-focuser.h"

#include <signal.h>

class Rts2DevFilterdFli:public Rts2DevFilterd
{
private:
  flidev_t dev;
  flidomain_t deviceDomain;

  long filter_count;

  int fliDebug;

protected:
    virtual int getFilterNum (void);
  virtual int setFilterNum (int new_filter);


public:
    Rts2DevFilterdFli (int in_argc, char **in_argv);
    virtual ~ Rts2DevFilterdFli (void);
  virtual int processOption (int in_opt);
  virtual int init (void);

  virtual int homeFilter ();
};



Rts2DevFilterdFli::Rts2DevFilterdFli (int in_argc, char **in_argv):
Rts2DevFilterd (in_argc, in_argv)
{
  deviceDomain = FLIDEVICE_FILTERWHEEL | FLIDOMAIN_USB;
  fliDebug = FLIDEBUG_NONE;
  addOption ('D', "domain", 1,
	     "CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
  addOption ('b', "fli_debug", 1,
	     "FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");
}

Rts2DevFilterdFli::~Rts2DevFilterdFli (void)
{
  FLIClose (dev);
}

int
Rts2DevFilterdFli::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'D':
      deviceDomain = FLIDEVICE_CAMERA;
      if (!strcasecmp ("USB", optarg))
	deviceDomain |= FLIDOMAIN_USB;
      else if (!strcasecmp ("LPT", optarg))
	deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
      else if (!strcasecmp ("SERIAL", optarg))
	deviceDomain |= FLIDOMAIN_SERIAL;
      else if (!strcasecmp ("INET", optarg))
	deviceDomain |= FLIDOMAIN_INET;
      else
	return -1;
      break;
    case 'b':
      switch (atoi (optarg))
	{
	case 1:
	  fliDebug = FLIDEBUG_FAIL;
	  break;
	case 2:
	  fliDebug = FLIDEBUG_FAIL | FLIDEBUG_WARN;
	  break;
	case 3:
	  fliDebug = FLIDEBUG_ALL;
	  break;
	default:
	  return -1;
	}
      break;
    default:
      return Rts2DevFilterd::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevFilterdFli::init (void)
{
  LIBFLIAPI ret;
  char **names;
  char *nam_sep;

  ret = Rts2DevFilterd::init ();
  if (ret)
    return ret;

  if (fliDebug)
    FLISetDebugLevel (NULL, fliDebug);

  ret = FLIList (deviceDomain, &names);
  if (ret)
    return -1;

  if (names[0] == NULL)
    {
      syslog (LOG_ERR, "Rts2DevFilterdFli::init No device found!");
      return -1;
    }

  nam_sep = strchr (names[0], ';');
  if (nam_sep)
    *nam_sep = '\0';

  ret = FLIOpen (&dev, names[0], deviceDomain);
  FLIFreeList (names);
  if (ret)
    return -1;

  ret = FLIGetFilterCount (dev, &filter_count);
  if (ret)
    return -1;

  return 0;
}

int
Rts2DevFilterdFli::getFilterNum (void)
{
  long ret_f;
  LIBFLIAPI ret;
  ret = FLIGetFilterPos (dev, &ret_f);
  if (ret)
    return -1;
  return (int) ret_f;
}

int
Rts2DevFilterdFli::setFilterNum (int new_filter)
{
  LIBFLIAPI ret;
  if (new_filter < 0 || new_filter >= filter_count)
    return -1;

  ret = FLISetFilterPos (dev, new_filter);
  if (ret)
    return -1;
  return 0;
}

int
Rts2DevFilterdFli::homeFilter ()
{
  return setFilterNum (FLI_FILTERPOSITION_HOME);
}

Rts2DevFilterdFli *device = NULL;

void
killSignal (int sig)
{
  delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevFilterdFli (argc, argv);

  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize fli filterd - exiting!\n");
      exit (1);
    }
  device->run ();
  delete device;
}
