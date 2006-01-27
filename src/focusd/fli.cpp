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

/*!
 * @file FLI focuser driver
 *
 * @author petr
 *
 * You will need FLIlib and option to ./configure (--with-fli=<llibflidir>)
 * to get that running. You can get libfli on http://www.moronski.com/fli
 */

#include "focuser.h"

#include "libfli.h"

class Rts2DevFocuserFli:public Rts2DevFocuser
{
private:
  flidev_t dev;
  char *deviceName;
  flidomain_t deviceDomain;

  int fliDebug;
public:
    Rts2DevFocuserFli (int in_argc, char **in_argv);
    virtual ~ Rts2DevFocuserFli (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int stepOut (int num);
  virtual int home ();
  virtual int isFocusing ();
};

Rts2DevFocuserFli::Rts2DevFocuserFli (int in_argc, char **in_argv):
Rts2DevFocuser (in_argc, in_argv)
{
  focTemp = nan ("f");

  deviceName = NULL;
  deviceDomain = FLIDEVICE_FOCUSER | FLIDOMAIN_USB;
  fliDebug = FLIDEBUG_NONE;
  addOption ('f', "device_name", 1, "device name");
  addOption ('D', "domain", 1,
	     "CCD Domain (default to USB; possible values: USB|LPT|SERIAL|INET)");
  addOption ('b', "fli_debug", 1,
	     "FLI debug level (1, 2 or 3; 3 will print most error message to stdout)");
}

Rts2DevFocuserFli::~Rts2DevFocuserFli (void)
{
  FLIClose (dev);
}

int
Rts2DevFocuserFli::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'f':
      deviceName = optarg;
      break;
    case 'D':
      deviceDomain = FLIDEVICE_FOCUSER;
      if (!strcasecmp ("USB", optarg))
	deviceDomain |= FLIDOMAIN_USB;
      else if (!strcasecmp ("LPT", optarg))
	deviceDomain |= FLIDOMAIN_PARALLEL_PORT;
      else if (!strcasecmp ("SERIAL", optarg))
	deviceDomain |= FLIDOMAIN_SERIAL;
      else if (!strcasecmp ("INET", optarg))
	deviceDomain |= FLIDOMAIN_INET;
      else if (!strcasecmp ("SERIAL_19200", optarg))
	deviceDomain |= FLIDOMAIN_SERIAL_19200;
      else if (!strcasecmp ("SERIAL_1200", optarg))
	deviceDomain |= FLIDOMAIN_SERIAL_1200;
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
	}
      break;
    default:
      return Rts2DevFocuser::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevFocuserFli::init ()
{
  LIBFLIAPI ret;
  int ret_f;

  ret_f = Rts2DevFocuser::init ();
  if (ret_f)
    return ret_f;

  if (fliDebug)
    FLISetDebugLevel (NULL, FLIDEBUG_ALL);

  ret = FLIOpen (&dev, deviceName, deviceDomain);
  if (ret)
    return -1;

  return 0;
}

int
Rts2DevFocuserFli::ready ()
{
  return info ();
}

int
Rts2DevFocuserFli::baseInfo ()
{
  LIBFLIAPI ret;

  ret = FLIGetModel (dev, focType, 20);
  if (ret)
    return -1;
  return 0;
}

int
Rts2DevFocuserFli::info ()
{
  LIBFLIAPI ret;

  long steps;

  ret = FLIGetStepperPosition (dev, &steps);
  if (ret)
    return -1;

  focPos = (int) steps;

  return 0;
}

int
Rts2DevFocuserFli::stepOut (int num)
{
  LIBFLIAPI ret;
  ret = FLIStepMotorAsync (dev, (long) num);
  if (ret)
    return -1;
  return 0;
}

int
Rts2DevFocuserFli::home ()
{
  LIBFLIAPI ret;
  ret = FLIHomeFocuser (dev);
  if (ret)
    return -1;
  return Rts2DevFocuser::home ();
}

int
Rts2DevFocuserFli::isFocusing ()
{
  LIBFLIAPI ret;
  long rem;

  ret = FLIGetStepsRemaining (dev, &rem);
  if (ret)
    return -1;
  if (rem == 0)
    return -2;
  return USEC_SEC / 100;
}

int
main (int argc, char **argv)
{
  Rts2DevFocuserFli *device = new Rts2DevFocuserFli (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize focuser - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
