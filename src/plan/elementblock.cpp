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

#include "script.h"
#include "elementblock.h"

using namespace rts2script;
using namespace rts2image;

ElementBlock::ElementBlock (Script * in_script):Element (in_script)
{
	curr_element = blockElements.end ();
	loopCount = 0;
}

ElementBlock::~ElementBlock (void)
{
	std::list < Element * >::iterator iter;
	for (iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
		delete (*iter);
	}
	blockElements.clear ();
}

int ElementBlock::blockScriptRet (int ret)
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

void ElementBlock::afterBlockEnd ()
{
	loopCount = 0;
	curr_element = blockElements.begin ();
}

void ElementBlock::addElement (Element * element)
{
	blockElements.push_back (element);
	if (curr_element == blockElements.end ())
		curr_element = blockElements.begin ();
}

void ElementBlock::postEvent (Rts2Event * event)
{
	std::list < Element * >::iterator el_iter_sig;
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
	Element::postEvent (event);
}

int ElementBlock::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	int ret;
	while (1)
	{
		Element *ce = *curr_element;
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

int ElementBlock::nextCommand (Rts2DevClientCamera * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	int ret;
	if (endLoop () || blockElements.empty ())
		return NEXT_COMMAND_NEXT;

	while (1)
	{
		Element *ce = *curr_element;
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

int ElementBlock::nextCommand (Rts2DevClientPhot * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	int ret;
	if (endLoop () || blockElements.empty ())
		return NEXT_COMMAND_NEXT;

	while (1)
	{
		Element *ce = *curr_element;
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

void ElementBlock::exposureEnd ()
{
	if (curr_element != blockElements.end ())
		(*curr_element)->exposureEnd ();
}

int ElementBlock::processImage (Image * image)
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->processImage (image);
	return -1;
}

int ElementBlock::waitForSignal (int in_sig)
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->waitForSignal (in_sig);
	return 0;
}

void ElementBlock::cancelCommands ()
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->cancelCommands ();
}

void ElementBlock::beforeExecuting ()
{
	Element::beforeExecuting ();
	std::list < Element * >::iterator iter;
	for (iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
		(*iter)->beforeExecuting ();
	}
}

int ElementBlock::getStartPos ()
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->getStartPos ();
	return Element::getStartPos ();
}

int ElementBlock::getLen ()
{
	if (curr_element != blockElements.end ())
		return (*curr_element)->getLen ();
	return Element::getLen ();
}

int ElementBlock::idleCall ()
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
	return Element::idleCall ();
}

void ElementBlock::prettyPrint (std::ostream &os)
{
	for (std::list < Element *>::iterator iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
		os << "   ";
		(*iter)->prettyPrint (os);
		os << std::endl;
	}
}

void ElementBlock::printXml (std::ostream &os)
{
	for (std::list < Element *>::iterator iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
		os << "   ";
		(*iter)->printXml (os);
		os << std::endl;
	}
}

void ElementBlock::printScript (std::ostream &os)
{
	os << " { ";
	for (std::list < Element *>::iterator iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
	  	if (iter != blockElements.begin ())
			os << " ";
		(*iter)->printScript (os);
	}
	os << " }";
}

double ElementBlock::getExpectedDuration ()
{
	double ret = 0;
	for (std::list < Element *>::iterator iter = blockElements.begin (); iter != blockElements.end (); iter++)
		ret += (*iter)->getExpectedDuration ();
	return ret;
}

ElementSignalEnd::ElementSignalEnd (Script * in_script, int end_sig_num):ElementBlock (in_script)
{
	sig_num = end_sig_num;
}

ElementSignalEnd::~ElementSignalEnd (void)
{
}

int ElementSignalEnd::waitForSignal (int in_sig)
{
	// we get our signall..
	if (in_sig == sig_num)
	{
		sig_num = -1;
		return 1;
	}
	return ElementBlock::waitForSignal (in_sig);
}


ElementAcquired::ElementAcquired (Script * in_script, int in_tar_id):ElementBlock (in_script)
{
	elseBlock = NULL;
	tar_id = in_tar_id;
	state = NOT_CALLED;
}

ElementAcquired::~ElementAcquired (void)
{
	delete elseBlock;
}

bool ElementAcquired::endLoop ()
{
	return (getLoopCount () != 0);
}

void ElementAcquired::checkState ()
{
	if (state == NOT_CALLED)
	{
		int acquireState = 0;
		script->getMaster ()->postEvent (new Rts2Event (EVENT_GET_ACQUIRE_STATE, (void *) &acquireState));
		if (acquireState == 1)
			state = ACQ_OK;
		else
			state = ACQ_FAILED;
	}
}

void ElementAcquired::postEvent (Rts2Event * event)
{
	switch (state)
	{
		case ACQ_OK:
			ElementBlock::postEvent (event);
			break;
		case ACQ_FAILED:
			if (elseBlock)
			{
				elseBlock->postEvent (event);
				break;
			}
		case NOT_CALLED:
			Element::postEvent (event);
			break;
	}
}

int ElementAcquired::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	checkState ();
	if (state == ACQ_OK)
		return ElementBlock::defnextCommand (client, new_command, new_device);
	if (elseBlock)
		return elseBlock->defnextCommand (client, new_command, new_device);
	return NEXT_COMMAND_NEXT;
}

int ElementAcquired::nextCommand (Rts2DevClientCamera * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	checkState ();
	if (state == ACQ_OK)
		return ElementBlock::nextCommand (client, new_command, new_device);
	if (elseBlock)
		return elseBlock->defnextCommand (client, new_command, new_device);
	return NEXT_COMMAND_NEXT;
}

int ElementAcquired::nextCommand (Rts2DevClientPhot * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	checkState ();
	if (state == ACQ_OK)
		return ElementBlock::nextCommand (client, new_command, new_device);
	if (elseBlock)
		return elseBlock->defnextCommand (client, new_command, new_device);
	return NEXT_COMMAND_NEXT;
}

int ElementAcquired::processImage (Image * image)
{
	switch (state)
	{
		case ACQ_OK:
			return ElementBlock::processImage (image);
		case ACQ_FAILED:
			if (elseBlock)
				return elseBlock->processImage (image);
		default:
			return Element::processImage (image);
	}
}

int ElementAcquired::waitForSignal (int in_sig)
{
	switch (state)
	{
		case ACQ_OK:
			return ElementBlock::waitForSignal (in_sig);
		case ACQ_FAILED:
			if (elseBlock)
				return elseBlock->waitForSignal (in_sig);
		default:
			return Element::waitForSignal (in_sig);
	}
}

void ElementAcquired::cancelCommands ()
{
	switch (state)
	{
		case ACQ_OK:
			ElementBlock::cancelCommands ();
			break;
		case ACQ_FAILED:
			if (elseBlock)
			{
				elseBlock->cancelCommands ();
				break;
			}
		case NOT_CALLED:
			Element::cancelCommands ();
			break;
	}
}

void ElementAcquired::addElseElement (Element * element)
{
	if (!elseBlock)
	{
		elseBlock = new ElementElse (script);
	}
	elseBlock->addElement (element);
}

void ElementAcquired::printScript (std::ostream &os)
{
	os << COMMAND_BLOCK_ACQ;
	ElementBlock::printScript (os);
	if (elseBlock)
	{
		os << " " COMMAND_BLOCK_ELSE " ";
		elseBlock->printScript (os);
	}
}

bool ElementElse::endLoop ()
{
	return (getLoopCount () != 0);
}

void ElementFor::prettyPrint (std::ostream &os)
{
	os << "for " << max << std::endl;
	ElementBlock::prettyPrint (os);
}

void ElementFor::printXml (std::ostream &os)
{
	os << "<for count='" << max << "'>" << std::endl;
	ElementBlock::printXml (os);
	os << "</for>";
}

void ElementFor::printScript (std::ostream &os)
{
	os << COMMAND_BLOCK_FOR << " " << max;
	ElementBlock::printScript (os);
}

void ElementFor::printJson (std::ostream &os)
{
	os << "\"cmd\":\"" COMMAND_BLOCK_FOR "\",\"count\":" << max << ",\"block\":[{";
	for (std::list < Element *>::iterator iter = blockElements.begin (); iter != blockElements.end (); iter++)
	{
	  	if (iter != blockElements.begin ())
			os << "},{";
		(*iter)->printJson (os);
	}
	os << "}]";
}

double ElementFor::getExpectedDuration ()
{
	return max * ElementBlock::getExpectedDuration ();
}

int ElementWhileSod::nextCommand (Rts2DevClientCamera * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	rts2core::Value *val = client->getConnection ()->getValue ("que_exp_num");

	int ret;
	if (endLoop () || blockElements.empty ())
		return NEXT_COMMAND_NEXT;

	while (1)
	{
		Element *ce = *curr_element;
		ret = ce->nextCommand (client, new_command, new_device);
		if (ret == 0 && curr_element == blockElements.begin ())
		{
			// test if exposures are qued..
			if (val && val->getValueInteger () > 1)
			{
				*new_command = NULL;
				return blockScriptRet (NEXT_COMMAND_KEEP);
			}
		}
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

void ElementWhileSod::printScript (std::ostream &os)
{
	os << COMMAND_WAIT_SOD " " << endSod;
	ElementBlock::printScript (os);
}

void ElementWhile::printScript (std::ostream &os)
{
	os << COMMAND_BLOCK_WHILE " " << max_cycles;
	ElementBlock::printScript (os);
}

bool ElementWhile::endLoop ()
{
	return getLoopCount () >= max_cycles || condition->getDouble () == 0;
}

void ElementDo::printScript (std::ostream &os)
{
	os << COMMAND_BLOCK_DO << " " << max_cycles << (*condition);
	ElementBlock::printScript (os);
}

bool ElementDo::endLoop ()
{
	if (getLoopCount () == 0)
		return false;
	return getLoopCount () >= max_cycles || condition->getDouble () == 0;
}

void ElementOnce::printScript (std::ostream &os)
{
	os << COMMAND_BLOCK_ONCE;
	ElementBlock::printScript (os);
}

bool ElementOnce::endLoop ()
{
	// only execute if both local (block) and script count are 0
	if (script->getLoopCount () == 0 && getLoopCount () == 0)
		return false;
	return true;
}
