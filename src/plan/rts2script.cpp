#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2script.h"
#include "rts2scriptblock.h"
#include "rts2scriptguiding.h"
#include "../utilsdb/target.h"
#include <string.h>
#include <ctype.h>

// test if next element is one that is given
bool
Rts2Script::isNext (const char *element)
{
  // skip spaces..
  size_t el_len = strlen (element);
  while (isspace (*cmdBufTop))
    cmdBufTop++;
  if (!strncmp (element, cmdBufTop, el_len))
    {
      // eat us..
      cmdBufTop += el_len;
      if (*cmdBufTop)
	{
	  *cmdBufTop = '\0';
	  cmdBufTop++;
	}
      return true;
    }
  return false;
}

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

Rts2Script::Rts2Script (Rts2Conn * in_connection, Target * target):
Rts2Object ()
{
  Rts2ScriptElement *element;
  char scriptText[MAX_COMMAND_LENGTH];
  target->getScript (in_connection->getName (), scriptText);
  cmdBuf = new char[strlen (scriptText) + 1];
  strcpy (cmdBuf, scriptText);
  strcpy (defaultDevice, in_connection->getName ());
  connection = in_connection;
  cmdBufTop = cmdBuf;
  do
    {
      element = parseBuf (target);
      if (!element)
	break;
      elements.push_back (element);
    }
  while (1);
  executedCount = 0;
  currScriptElement = NULL;
  el_iter = elements.begin ();
}

Rts2Script::~Rts2Script (void)
{
  // all operations with elements list should be ignored
  executedCount = -1;
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
  std::list < Rts2ScriptElement * >::iterator el_iter_sig;
  Rts2ScriptElement *el;
  int ret;
  switch (event->getType ())
    {
    case EVENT_SIGNAL:
      for (el_iter_sig = el_iter; el_iter_sig != elements.end ();
	   el_iter_sig++)
	{
	  el = *el_iter_sig;
	  ret = el->waitForSignal (*(int *) event->getArg ());
	  if (ret)
	    {
	      // we find first signal..put as at begining
	      el_iter = el_iter_sig;
	      *((int *) event->getArg ()) = -1;
	    }
	}
      break;
    case EVENT_ACQUIRE_QUERY:
    case EVENT_SIGNAL_QUERY:
      for (el_iter_sig = el_iter; el_iter_sig != elements.end ();
	   el_iter_sig++)
	{
	  el = *el_iter_sig;
	  el->postEvent (new Rts2Event (event));
	}
      break;
    default:
      if (el_iter != elements.end ())
	(*el_iter)->postEvent (new Rts2Event (event));
      break;
    }
  Rts2Object::postEvent (event);
}

Rts2ScriptElement *
Rts2Script::parseBuf (Target * target)
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
  else if (!strcmp (commandStart, COMMAND_BINNING))
    {
      int bin;
      if (getNextParamInteger (&bin))
	return NULL;
      return new Rts2ScriptElementBinning (this, bin);
    }
  else if (!strcmp (commandStart, COMMAND_BOX))
    {
      int x, y, w, h;
      if (getNextParamInteger (&x)
	  || getNextParamInteger (&y)
	  || getNextParamInteger (&w) || getNextParamInteger (&h))
	return NULL;
      return new Rts2ScriptElementBox (this, x, y, w, h);
    }
  else if (!strcmp (commandStart, COMMAND_CENTER))
    {
      int w, h;
      if (getNextParamInteger (&w) || getNextParamInteger (&h))
	return NULL;
      return new Rts2ScriptElementCenter (this, w, h);
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
      // target is already acquired
      if (target->isAcquired ())
	return new Rts2ScriptElement (this);
      return new Rts2ScriptElementAcquire (this, precision, expTime);
    }
  else if (!strcmp (commandStart, COMMAND_WAIT_ACQUIRE))
    {
      return new Rts2ScriptElementWaitAcquire (this,
					       target->getObsTargetID ());
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
  else if (!strcmp (commandStart, COMMAND_HAM))
    {
      int repNumber;
      float exposure;
      if (getNextParamInteger (&repNumber) || getNextParamFloat (&exposure))
	return NULL;
      return new Rts2ScriptElementAcquireHam (this, repNumber, exposure);
    }
  else if (!strcmp (commandStart, COMMAND_STAR_SEARCH))
    {
      int repNumber;
      double precision;
      float exposure;
      double scale;
      if (getNextParamInteger (&repNumber) || getNextParamDouble (&precision)
	  || getNextParamFloat (&exposure) || getNextParamDouble (&scale))
	return NULL;
      return new Rts2ScriptElementAcquireStar (this, repNumber, precision,
					       exposure, scale, scale);
    }
  else if (!strcmp (commandStart, COMMAND_PHOT_SEARCH))
    {
      double searchRadius;
      double searchSpeed;
      if (getNextParamDouble (&searchRadius)
	  || getNextParamDouble (&searchSpeed))
	return NULL;
      return new Rts2ScriptElementSearch (this, searchRadius, searchSpeed);
    }
  else if (!strcmp (commandStart, COMMAND_BLOCK_WAITSIG))
    {
      int waitSig;
      char *el;
      Rts2ScriptElementBlock *blockEl;
      Rts2ScriptElement *newElement;
      if (getNextParamInteger (&waitSig))
	return NULL;
      // test for block start..
      el = nextElement ();
      // error, return NULL
      if (*el != '{')
	return NULL;
      blockEl = new Rts2SEBSignalEnd (this, waitSig);
      // parse block..
      while (1)
	{
	  newElement = parseBuf (target);
	  // "}" will result in NULL, which we capture here
	  if (!newElement)
	    break;
	  blockEl->addElement (newElement);
	}
      // block can end by script end as well..
      return blockEl;
    }
  else if (!strcmp (commandStart, COMMAND_BLOCK_ACQ))
    {
      char *el;
      Rts2ScriptElement *newElement;
      Rts2SEBAcquired *acqIfEl;
      // test for block start..
      el = nextElement ();
      // error, return NULL
      if (*el != '{')
	return NULL;
      acqIfEl = new Rts2SEBAcquired (this, target->getObsTargetID ());
      // parse block..
      while (1)
	{
	  newElement = parseBuf (target);
	  // "}" will result in NULL, which we capture here
	  if (!newElement)
	    break;
	  acqIfEl->addElement (newElement);
	}
      // test for if..
      if (isNext (COMMAND_BLOCK_ELSE))
	{
	  el = nextElement ();
	  if (*el != '{')
	    return NULL;
	  // parse block..
	  while (1)
	    {
	      newElement = parseBuf (target);
	      // "}" will result in NULL, which we capture here
	      if (!newElement)
		break;
	      acqIfEl->addElseElement (newElement);
	    }
	}
      // block can end by script end as well..
      return acqIfEl;
    }
  // end of block..
  else if (!strcmp (commandStart, "}"))
    {
      return NULL;
    }
  else if (!strcmp (commandStart, COMMAND_GUIDING))
    {
      float init_exposure;
      int end_signal;
      if (getNextParamFloat (&init_exposure)
	  || getNextParamInteger (&end_signal))
	return NULL;
      return new Rts2ScriptElementGuiding (this, init_exposure, end_signal);
    }
  return NULL;
}

int
Rts2Script::processImage (Rts2Image * image)
{
  if (executedCount < 0 || el_iter == elements.end ())
    return -1;
  return (*el_iter)->processImage (image);
}
