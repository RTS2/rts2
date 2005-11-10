#include "rts2scriptblock.h"

Rts2ScriptElementBlock::Rts2ScriptElementBlock (Rts2Script * in_script):Rts2ScriptElement
  (in_script)
{
  curr_element = blockElements.end ();
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
