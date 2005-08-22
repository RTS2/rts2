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

Rts2Script::Rts2Script (char *scriptText, Rts2Conn * in_connection):Rts2Object
  ()
{
  Rts2ScriptElement *
    element;
  cmdBuf = new char[strlen (scriptText) + 1];
  strcpy (cmdBuf, scriptText);
  strcpy (defaultDevice, in_connection->getName ());
  connection = in_connection;
  cmdBufTop = cmdBuf;
  do
    {
      element = parseBuf ();
      if (!element)
	break;
      elements.push_back (element);
    }
  while (1);
  executedCount = 0;
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

void
Rts2Script::postEvent (Rts2Event * event)
{
  std::list < Rts2ScriptElement * >::iterator el_iter, el_iter_tmp,
    el_iter_for_erase;
  Rts2ScriptElement *el;
  int ret;
  switch (event->getType ())
    {
    case EVENT_SIGNAL:
      for (el_iter = elements.begin (); el_iter != elements.end (); el_iter++)
	{
	  el = *el_iter;
	  ret = el->waitForSignal (*(int *) event->getArg ());
	  if (ret)
	    {
	      // we find first signal..take from list out all previsous script elements, put as at begining
	      for (el_iter_tmp = elements.begin (); el_iter_tmp != el_iter;)
		{
		  el = *el_iter_tmp;
		  el_iter_for_erase = el_iter_tmp;
		  el_iter_tmp++;
		  delete el;
		  elements.erase (el_iter_for_erase);
		}
	      *((int *) event->getArg ()) = -1;
	    }
	}
      break;
    default:
      if (elements.size () > 0)
	(*elements.begin ())->postEvent (new Rts2Event (event));
      break;
    }
  Rts2Object::postEvent (event);
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
  else if (!strcmp (commandStart, COMMAND_WAIT))
    {
      return new Rts2ScriptElementWait (this);
    }
  else if (!strcmp (commandStart, COMMAND_ACQUIRE))
    {
      double precision;
      float expTime;
      if (getNextParamDouble (&precision) || getNextParamFloat (&expTime))
	return NULL;
      return new Rts2ScriptElementAcquire (this, precision, expTime);
    }
  else if (!strcmp (commandStart, COMMAND_WAIT_ACQUIRE))
    {
      return new Rts2ScriptElementWaitAcquire (this);
    }
  else if (!strcmp (commandStart, COMMAND_MIRROR_MOVE))
    {
      int mirror_pos;
      if (getNextParamInteger (&mirror_pos))
	return NULL;
      return new Rts2ScriptElementMirror (this, new_device, mirror_pos);
    }
  else if (!strcmp (commandStart, COMMAND_PHOTOMETER))
    {
      int filter;
      float exposure;
      int count;
      if (getNextParamInteger (&filter) || getNextParamFloat (&exposure)
	  || getNextParamInteger (&count))
	return NULL;
      return new Rts2ScriptElementPhotometer (this, filter, exposure, count);
    }
  else if (!strcmp (commandStart, COMMAND_SEND_SIGNAL))
    {
      int signalNum;
      if (getNextParamInteger (&signalNum))
	return NULL;
      return new Rts2ScriptElementSendSignal (this, signalNum);
    }
  else if (!strcmp (commandStart, COMMAND_WAIT_SIGNAL))
    {
      int signalNum;
      if (getNextParamInteger (&signalNum))
	return NULL;
      if (signalNum <= 0)
	return NULL;
      return new Rts2ScriptElementWaitSignal (this, signalNum);
    }
  return NULL;
}

int
Rts2Script::nextCommand (Rts2DevClientCamera * camera,
			 Rts2Command ** new_command,
			 char new_device[DEVICE_NAME_SIZE])
{
  std::list < Rts2ScriptElement * >::iterator el_iter;
  Rts2ScriptElement *nextElement;
  int ret;

  el_iter = elements.begin ();
  while (1)
    {
      if (el_iter == elements.end ())
	// command not found, end of script,..
	return NEXT_COMMAND_END_SCRIPT;
      nextElement = *el_iter;
      ret = nextElement->nextCommand (camera, new_command, new_device);
      if (ret != NEXT_COMMAND_NEXT)
	break;
      delete nextElement;
      el_iter++;
      elements.erase (elements.begin ());
    }
  switch (ret)
    {
    case 0:
    case NEXT_COMMAND_CHECK_WAIT:
    case NEXT_COMMAND_PRECISION_FAILED:
    case NEXT_COMMAND_PRECISION_OK:
    case NEXT_COMMAND_WAIT_ACQUSITION:
      elements.erase (el_iter);
      delete nextElement;
      break;
    case NEXT_COMMAND_WAITING:
      *new_command = NULL;
      break;
    case NEXT_COMMAND_KEEP:
    case NEXT_COMMAND_RESYNC:
    case NEXT_COMMAND_ACQUSITION_IMAGE:
    case NEXT_COMMAND_WAIT_SIGNAL:
    case NEXT_COMMAND_WAIT_MIRROR:
      // keep us
      break;
    }
  if (ret != NEXT_COMMAND_NEXT)
    executedCount++;
  return ret;
}

int
Rts2Script::nextCommand (Rts2DevClientPhot * phot,
			 Rts2Command ** new_command,
			 char new_device[DEVICE_NAME_SIZE])
{
  std::list < Rts2ScriptElement * >::iterator el_iter;
  Rts2ScriptElement *nextElement;
  int ret;

  el_iter = elements.begin ();
  while (1)
    {
      if (el_iter == elements.end ())
	// command not found, end of script,..
	return NEXT_COMMAND_END_SCRIPT;
      nextElement = *el_iter;
      ret = nextElement->nextCommand (phot, new_command, new_device);
      if (ret != NEXT_COMMAND_NEXT)
	break;
      delete nextElement;
      el_iter++;
      elements.erase (elements.begin ());
    }
  switch (ret)
    {
    case 0:
    case NEXT_COMMAND_CHECK_WAIT:
    case NEXT_COMMAND_PRECISION_FAILED:
    case NEXT_COMMAND_PRECISION_OK:
    case NEXT_COMMAND_WAIT_ACQUSITION:
      elements.erase (el_iter);
      delete nextElement;
      break;
    case NEXT_COMMAND_WAITING:
      *new_command = NULL;
      break;
    case NEXT_COMMAND_KEEP:
    case NEXT_COMMAND_RESYNC:
    case NEXT_COMMAND_ACQUSITION_IMAGE:
    case NEXT_COMMAND_WAIT_SIGNAL:
    case NEXT_COMMAND_WAIT_MIRROR:
      // keep us
      break;
    }
  if (ret != NEXT_COMMAND_NEXT)
    executedCount++;
  return ret;
}

int
Rts2Script::processImage (Rts2Image * image)
{
  if (elements.size () == 0)
    return -1;
  return (*elements.begin ())->processImage (image);
}
