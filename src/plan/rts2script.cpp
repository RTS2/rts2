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
Rts2Script::getNextParamDouble (double *val)
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
  Rts2ScriptElement *element;
  cmdBuf = new char[strlen (scriptText) + 1];
  strcpy (cmdBuf, scriptText);
  strcpy (defaultDevice, in_defaultDevice);
  cmdBufTop = cmdBuf;
  do
    {
      element = parseBuf ();
      if (!element)
	break;
      elements.push_back (element);
    }
  while (1);
}

Rts2Script::~Rts2Script (void)
{
  std::list < Rts2ScriptElement * >::iterator el_iter;
  for (el_iter = elements.begin (); el_iter != elements.end (); el_iter++)
    {
      Rts2ScriptElement *el;
      el = *el_iter;
      delete el;
    }
  elements.clear ();
  delete[]cmdBuf;
}

Rts2ScriptElement *
Rts2Script::parseBuf ()
{
  char *commandStart;
  char *devSep;
  char new_device[DEVICE_NAME_SIZE];
  int ret;

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
	return NULL;
      return new Rts2ScriptElementExpose (this, exp_time);
    }
  else if (!strcmp (commandStart, COMMAND_DARK))
    {
      float exp_time;
      ret = getNextParamFloat (&exp_time);
      if (ret)
	return NULL;
      return new Rts2ScriptElementDark (this, exp_time);
    }
  else if (!strcmp (commandStart, COMMAND_FILTER))
    {
      int filter;
      ret = getNextParamInteger (&filter);
      if (ret)
	return NULL;
      return new Rts2ScriptElementFilter (this, filter);
    }
  else if (!strcmp (commandStart, COMMAND_CHANGE))
    {
      double ra;
      double dec;
      if (getNextParamDouble (&ra) || getNextParamDouble (&dec))
	return NULL;
      return new Rts2ScriptElementChange (this, ra, dec);
    }
  return NULL;
}

int
Rts2Script::nextCommand (Rts2Block * in_master, Rts2DevClientCamera * camera,
			 Rts2Command ** new_command,
			 char new_device[DEVICE_NAME_SIZE])
{
  std::list < Rts2ScriptElement * >::iterator el_iter;
  Rts2ScriptElement *nextElement;
  int ret;

  el_iter = elements.begin ();
  if (el_iter == elements.end ())
    // command not found, end of script,..
    return -1;

  nextElement = *el_iter;
  ret = nextElement->nextCommand (in_master, camera, new_command, new_device);
  elements.erase (el_iter);
  delete nextElement;
  return ret;
}
