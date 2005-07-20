#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2script.h"
#include "../utilsdb/target.h"
#include <string.h>
#include <ctype.h>

char *
Rts2Script::nextElement ()
{
  char *elementStart;
  while (isspace (*cmdBufTop))
    cmdBufTop++;
  elementStart = cmdBufTop;
  while (!isspace (*cmdBufTop) && *cmdBufTop)
    cmdBufTop++;
  if (*cmdBufTop)
    {
      *cmdBufTop = '\0';
      cmdBufTop++;
    }
  return elementStart;
}

int
Rts2Script::getNextParamFloat (float *val)
{
  char *el;
  el = nextElement ();
  if (!*el)
    return -1;
  *val = atof (el);
  return 0;
}

int
Rts2Script::getNextParamInteger (int *val)
{
  char *el;
  el = nextElement ();
  if (!*el)
    return -1;
  *val = atoi (el);
  return 0;
}

Rts2Script::Rts2Script (char *scriptText,
			const char in_defaultDevice[DEVICE_NAME_SIZE])
{
  cmdBuf = new char[strlen (scriptText) + 1];
  strcpy (cmdBuf, scriptText);
  strcpy (defaultDevice, in_defaultDevice);
  cmdBufTop = cmdBuf;
}

Rts2Script::~Rts2Script (void)
{
  delete[]cmdBuf;
}

int
Rts2Script::nextCommand (Rts2Block * in_master, Rts2DevClientCamera * camera,
			 Rts2Command ** new_command,
			 char new_device[DEVICE_NAME_SIZE])
{
  char *commandStart;
  char *devSep;
  int ret;

  *new_command = NULL;
  // find whole command
  commandStart = nextElement ();
  // if we include device name
  devSep = index (commandStart, '.');
  if (devSep)
    {
      *devSep = '\0';
      strncpy (new_device, commandStart, DEVICE_NAME_SIZE);
      new_device[DEVICE_NAME_SIZE - 1] = '\0';
      commandStart = devSep;
      commandStart++;
    }
  else
    {
      strncpy (new_device, defaultDevice, DEVICE_NAME_SIZE);
    }
  if (!strcmp (commandStart, COMMAND_EXPOSURE))
    {
      float exp_time;
      ret = getNextParamFloat (&exp_time);
      if (ret)
	return -1;
      *new_command =
	new Rts2CommandExposure (in_master, camera, EXP_LIGHT, exp_time);
      return 0;
    }
  else if (!strcmp (commandStart, COMMAND_DARK))
    {
      float exp_time;
      ret = getNextParamFloat (&exp_time);
      if (ret)
	return -1;
      *new_command =
	new Rts2CommandExposure (in_master, camera, EXP_DARK, exp_time);
      return 0;
    }
  else if (!strcmp (commandStart, COMMAND_FILTER))
    {
      int filter;
      ret = getNextParamInteger (&filter);
      if (ret)
	return -1;
      *new_command = new Rts2CommandFilter (in_master, filter);
      return 0;
    }
  // command not found, end of script,..
  return -1;
}
