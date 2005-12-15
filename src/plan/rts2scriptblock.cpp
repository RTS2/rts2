#include "rts2scriptblock.h"

Rts2ScriptElementBlock::Rts2ScriptElementBlock (Rts2Script * in_script):Rts2ScriptElement
  (in_script)
{
  curr_element = blockElements.end ();
  loopCount = 0;
}

Rts2ScriptElementBlock::~Rts2ScriptElementBlock (void)
{
  std::list < Rts2ScriptElement * >::iterator iter;
  for (iter = blockElements.begin (); iter != blockElements.end (); iter++)
    {
      delete (*iter);
    }
  blockElements.clear ();
}

int
Rts2ScriptElementBlock::blockScriptRet (int ret)
{
  switch (ret)
    {
    case 0:
    case NEXT_COMMAND_CHECK_WAIT:
    case NEXT_COMMAND_PRECISION_FAILED:
    case NEXT_COMMAND_PRECISION_OK:
    case NEXT_COMMAND_WAIT_ACQUSITION:
      curr_element++;
      if (curr_element == blockElements.end ())
	{
	  loopCount++;
	  if (endLoop ())
	    return ret;
	  curr_element = blockElements.begin ();
	}
      break;
    }
  return ret | NEXT_COMMAND_MASK_BLOCK;
}

void
Rts2ScriptElementBlock::addElement (Rts2ScriptElement * element)
{
  blockElements.push_back (element);
  if (curr_element == blockElements.end ())
    curr_element = blockElements.begin ();
}

void
Rts2ScriptElementBlock::postEvent (Rts2Event * event)
{
  std::list < Rts2ScriptElement * >::iterator el_iter_sig;
  switch (event->getType ())
    {
    case EVENT_ACQUIRE_QUERY:
    case EVENT_SIGNAL_QUERY:
      for (el_iter_sig = blockElements.begin ();
	   el_iter_sig != blockElements.end (); el_iter_sig++)
	{
	  (*curr_element)->postEvent (new Rts2Event (event));
	}
      break;
    default:
      if (curr_element != blockElements.end ())
	(*curr_element)->postEvent (new Rts2Event (event));
      break;
    }
  Rts2ScriptElement::postEvent (event);
}

int
Rts2ScriptElementBlock::defnextCommand (Rts2DevClient * client,
					Rts2Command ** new_command,
					char new_device[DEVICE_NAME_SIZE])
{
  int ret;
  while (1)
    {
      ret = (*curr_element)->defnextCommand (client, new_command, new_device);
      if (ret != NEXT_COMMAND_NEXT)
	break;
      curr_element++;
      if (curr_element == blockElements.end ())
	{
	  loopCount++;
	  if (endLoop ())
	    return NEXT_COMMAND_NEXT;
	  curr_element = blockElements.begin ();
	}
    }
  return blockScriptRet (ret);
}

int
Rts2ScriptElementBlock::nextCommand (Rts2DevClientCamera * client,
				     Rts2Command ** new_command,
				     char new_device[DEVICE_NAME_SIZE])
{
  int ret;
  while (1)
    {
      ret = (*curr_element)->nextCommand (client, new_command, new_device);
      if (ret != NEXT_COMMAND_NEXT)
	break;
      curr_element++;
      if (curr_element == blockElements.end ())
	{
	  loopCount++;
	  if (endLoop ())
	    return NEXT_COMMAND_NEXT;
	  curr_element = blockElements.begin ();
	}
    }
  return blockScriptRet (ret);
}

int
Rts2ScriptElementBlock::nextCommand (Rts2DevClientPhot * client,
				     Rts2Command ** new_command,
				     char new_device[DEVICE_NAME_SIZE])
{
  int ret;
  if (endLoop () || blockElements.empty ())
    return NEXT_COMMAND_NEXT;

  while (1)
    {
      ret = (*curr_element)->nextCommand (client, new_command, new_device);
      if (ret != NEXT_COMMAND_NEXT)
	break;
      curr_element++;
      if (curr_element == blockElements.end ())
	{
	  loopCount++;
	  if (endLoop ())
	    return NEXT_COMMAND_NEXT;
	  curr_element = blockElements.begin ();
	}
    }
  return blockScriptRet (ret);
}

int
Rts2ScriptElementBlock::processImage (Rts2Image * image)
{
  if (curr_element != blockElements.end ())
    return (*curr_element)->processImage (image);
  return -1;
}

int
Rts2ScriptElementBlock::waitForSignal (int in_sig)
{
  if (curr_element != blockElements.end ())
    return (*curr_element)->waitForSignal (in_sig);
  return 0;
}

void
Rts2ScriptElementBlock::cancelCommands ()
{
  if (curr_element != blockElements.end ())
    return (*curr_element)->cancelCommands ();
}

Rts2SEBSignalEnd::Rts2SEBSignalEnd (Rts2Script * in_script, int end_sig_num):Rts2ScriptElementBlock
  (in_script)
{
  sig_num = end_sig_num;
}

Rts2SEBSignalEnd::~Rts2SEBSignalEnd (void)
{
}

int
Rts2SEBSignalEnd::waitForSignal (int in_sig)
{
  // we get our signall..
  if (in_sig == sig_num)
    {
      sig_num = -1;
      return 1;
    }
  return Rts2ScriptElementBlock::waitForSignal (in_sig);
}

Rts2SEBAcquired::Rts2SEBAcquired (Rts2Script * in_script, int in_tar_id):Rts2ScriptElementBlock
  (in_script)
{
  elseBlock = NULL;
  tar_id = in_tar_id;
  state = NOT_CALLED;
}

Rts2SEBAcquired::~Rts2SEBAcquired (void)
{
  delete elseBlock;
}

bool
Rts2SEBAcquired::endLoop ()
{
  return (loopCount != 0);
}

void
Rts2SEBAcquired::checkState ()
{
  if (state == NOT_CALLED)
    {
      int acquireState = 0;
      script->getMaster ()->
	postEvent (new
		   Rts2Event (EVENT_GET_ACQUIRE_STATE,
			      (void *) &acquireState));
      if (acquireState == 1)
	state = ACQ_OK;
      else
	state = ACQ_FAILED;
    }
}

void
Rts2SEBAcquired::postEvent (Rts2Event * event)
{
  switch (state)
    {
    case ACQ_OK:
      Rts2ScriptElementBlock::postEvent (event);
      break;
    case ACQ_FAILED:
      if (elseBlock)
	{
	  elseBlock->postEvent (event);
	  break;
	}
    case NOT_CALLED:
      Rts2ScriptElement::postEvent (event);
      break;
    }
}

int
Rts2SEBAcquired::defnextCommand (Rts2DevClient * client,
				 Rts2Command ** new_command,
				 char new_device[DEVICE_NAME_SIZE])
{
  checkState ();
  if (state == ACQ_OK)
    return Rts2ScriptElementBlock::defnextCommand (client, new_command,
						   new_device);
  if (elseBlock)
    return elseBlock->defnextCommand (client, new_command, new_device);
  return NEXT_COMMAND_NEXT;
}

int
Rts2SEBAcquired::nextCommand (Rts2DevClientCamera * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE])
{
  checkState ();
  if (state == ACQ_OK)
    return Rts2ScriptElementBlock::nextCommand (client, new_command,
						new_device);
  if (elseBlock)
    return elseBlock->defnextCommand (client, new_command, new_device);
  return NEXT_COMMAND_NEXT;
}

int
Rts2SEBAcquired::nextCommand (Rts2DevClientPhot * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE])
{
  checkState ();
  if (state == ACQ_OK)
    return Rts2ScriptElementBlock::nextCommand (client, new_command,
						new_device);
  if (elseBlock)
    return elseBlock->defnextCommand (client, new_command, new_device);
  return NEXT_COMMAND_NEXT;
}

int
Rts2SEBAcquired::processImage (Rts2Image * image)
{
  switch (state)
    {
    case ACQ_OK:
      return Rts2ScriptElementBlock::processImage (image);
    case ACQ_FAILED:
      if (elseBlock)
	return elseBlock->processImage (image);
    default:
      return Rts2ScriptElement::processImage (image);
    }
}

int
Rts2SEBAcquired::waitForSignal (int in_sig)
{
  switch (state)
    {
    case ACQ_OK:
      return Rts2ScriptElementBlock::waitForSignal (in_sig);
    case ACQ_FAILED:
      if (elseBlock)
	return elseBlock->waitForSignal (in_sig);
    default:
      return Rts2ScriptElement::waitForSignal (in_sig);
    }
}

void
Rts2SEBAcquired::cancelCommands ()
{
  switch (state)
    {
    case ACQ_OK:
      Rts2ScriptElementBlock::cancelCommands ();
      break;
    case ACQ_FAILED:
      if (elseBlock)
	{
	  elseBlock->cancelCommands ();
	  break;
	}
    case NOT_CALLED:
      Rts2ScriptElement::cancelCommands ();
      break;
    }
}

void
Rts2SEBAcquired::addElseElement (Rts2ScriptElement * element)
{
  if (!elseBlock)
    {
      elseBlock = new Rts2SEBElse (script);
    }
  elseBlock->addElement (element);
}

bool
Rts2SEBElse::endLoop ()
{
  return (loopCount != 0);
}
