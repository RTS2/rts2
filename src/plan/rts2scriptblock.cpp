/*
 * Wait for some event or for a number of seconds.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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
				if (getNextLoop ())
					return ret;
				curr_element = blockElements.begin ();
			}
			break;
	}
	return ret | NEXT_COMMAND_MASK_BLOCK;
}


void
Rts2ScriptElementBlock::afterBlockEnd ()
{
	loopCount = 0;
	curr_element = blockElements.begin ();
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
				(*el_iter_sig)->postEvent (new Rts2Event (event));
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
		Rts2ScriptElement *ce = *curr_element;
		ret = ce->defnextCommand (client, new_command, new_device);
		if (ret != NEXT_COMMAND_NEXT)
			break;
		curr_element++;
		if (curr_element == blockElements.end ())
		{
			loopCount++;
			if (getNextLoop ())
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
	if (endLoop () || blockElements.empty ())
		return NEXT_COMMAND_NEXT;

	while (1)
	{
		Rts2ScriptElement *ce = *curr_element;
		ret = ce->nextCommand (client, new_command, new_device);
		if (ret != NEXT_COMMAND_NEXT)
			break;
		curr_element++;
		if (curr_element == blockElements.end ())
		{
			loopCount++;
			if (getNextLoop ())
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
		Rts2ScriptElement *ce = *curr_element;
		ret = ce->nextCommand (client, new_command, new_device);
		if (ret != NEXT_COMMAND_NEXT)
			break;
		curr_element++;
		if (curr_element == blockElements.end ())
		{
			loopCount++;
			if (getNextLoop ())
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


void
Rts2ScriptElementBlock::beforeExecuting ()
{
	Rts2ScriptElement::beforeExecuting ();
	std::list < Rts2ScriptElement * >::iterator iter;
	for (iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
		(*iter)->beforeExecuting ();
	}
}


int
Rts2ScriptElementBlock::getStartPos ()
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->getStartPos ();
	return Rts2ScriptElement::getStartPos ();
}


int
Rts2ScriptElementBlock::getLen ()
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->getLen ();
	return Rts2ScriptElement::getLen ();
}


int
Rts2ScriptElementBlock::idleCall ()
{
	if (curr_element != blockElements.end ())
	{
		int ret = (*curr_element)->idleCall ();
		if (ret != NEXT_COMMAND_NEXT)
			return blockScriptRet (ret);
		curr_element++;
		if (curr_element == blockElements.end ())
		{
			loopCount++;
			if (getNextLoop ())
				return NEXT_COMMAND_NEXT;
			curr_element = blockElements.begin ();
		}
		// trigger nextCommand call, which will call nextCommand, which will execute command from block
		return NEXT_COMMAND_NEXT;
	}
	return Rts2ScriptElement::idleCall ();
}


Rts2SEBSignalEnd::Rts2SEBSignalEnd (Rts2Script * in_script, int end_sig_num):
Rts2ScriptElementBlock (in_script)
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


bool Rts2SEBAcquired::endLoop ()
{
	return (getLoopCount () != 0);
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


bool Rts2SEBElse::endLoop ()
{
	return (getLoopCount () != 0);
}
