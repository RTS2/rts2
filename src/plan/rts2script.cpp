/*
 * Script support.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2script.h"
#include "rts2setarget.h"
#include "rts2scriptblock.h"
#include "rts2scriptguiding.h"
#include "rts2sehex.h"
#include "rts2swaitfor.h"
#include "../utilsdb/scriptcommands.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>

// test if next element is one that is given
bool Rts2Script::isNext (const char *element)
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
Rts2Script::getNextParamString (char **val)
{
	*val = nextElement ();
	if (!*val)
		return -1;
	return 0;
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


Rts2Script::Rts2Script (Rts2Block * in_master)
:Rts2Object ()
{
	master = in_master;
	executedCount = 0;
	lineOffset = 0;
	cmdBuf = NULL;
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
	delete[] cmdBuf;
}


int
Rts2Script::setTarget (const char *cam_name, Rts2Target * target)
{
	Rts2ScriptElement *element;
	std::string scriptText;
	struct ln_equ_posn target_pos;

	target->getPosition (&target_pos);

	strcpy (defaultDevice, cam_name);
	commentNumber = 1;
	wholeScript = std::string ("");

	int ret = 1;
	// offset on scripts over one line
	do
	{
		delete[] cmdBuf;

		ret = target->getScript (cam_name, scriptText);
		if (ret == -1)
			return -1;
		char *comment = NULL;
		int offset = 0;
		cmdBuf = new char[scriptText.length () + 1];
		strcpy (cmdBuf, scriptText.c_str ());
		// find any possible comment and mark it
		cmdBufTop = cmdBuf;
		while (*cmdBufTop && *cmdBufTop != '#')
			cmdBufTop++;
		if (*cmdBufTop == '#')
		{
			*cmdBufTop = '\0';
			cmdBufTop++;
			while (*cmdBufTop && isspace (*cmdBufTop))
				cmdBufTop++;
			comment = cmdBufTop;
		}

		// if we are adding and last character is not space, add space

		if (wholeScript.length () != 0 && wholeScript[wholeScript.length () - 1] != ' ')
		{
			lineOffset++;
			wholeScript += std::string (" ");
		}

		wholeScript += std::string (cmdBuf);

		cmdBufTop = cmdBuf;
		commandStart = cmdBuf;
		while (1)
		{
			element = parseBuf (target, &target_pos);
			if (!element)
				break;
			element->setLen (cmdBufTop - commandStart);
			offset += cmdBufTop - commandStart;
			elements.push_back (element);
		}
		// add comment if there was one
		if (comment)
		{
			element = new Rts2ScriptElementComment (this, comment, commentNumber);
			std::ostringstream ws;
			ws << "#" << commentNumber << " ";
			if (wholeScript.length () > 0)
			{
				wholeScript += " ";
				offset++;
			}
			wholeScript += ws.str ();
			commentNumber++;

			element->setLen (2);
			offset += 3;
			elements.push_back (element);
		}
		lineOffset += offset;
	} while (ret == 1);

	executedCount = 0;
	currScriptElement = NULL;
	for (el_iter = elements.begin (); el_iter != elements.end (); el_iter++)
	{
		element = *el_iter;
		element->beforeExecuting ();
	}
	el_iter = elements.begin ();
	return 0;
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
Rts2Script::parseBuf (Rts2Target * target, struct ln_equ_posn *target_pos)
{
	char *devSep;
	char new_device[DEVICE_NAME_SIZE];
	int ret;

	while (isspace (*cmdBufTop))
		cmdBufTop++;

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
	// we hit end of script buffer
	if (commandStart == '\0')
	{
		return NULL;
	}
	else if (!strcmp (commandStart, COMMAND_EXPOSURE))
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
		return new Rts2ScriptElementChange (this, new_device, ra, dec);
	}
	else if (!strcmp (commandStart, COMMAND_WAIT))
	{
		return new Rts2ScriptElementWait (this);
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
	#ifdef HAVE_PGSQL
	else if (!strcmp (commandStart, COMMAND_ACQUIRE))
	{
		double precision;
		float expTime;
		if (getNextParamDouble (&precision) || getNextParamFloat (&expTime))
			return NULL;
		// target is already acquired
		if (target->isAcquired ())
			return new Rts2ScriptElement (this);
		return new Rts2ScriptElementAcquire (this, precision, expTime,
			target_pos);
	}
	else if (!strcmp (commandStart, COMMAND_HAM))
	{
		int repNumber;
		float exposure;
		if (getNextParamInteger (&repNumber) || getNextParamFloat (&exposure))
			return NULL;
		return new Rts2ScriptElementAcquireHam (this, repNumber, exposure,
			target_pos);
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
			exposure, scale, scale,
			target_pos);
	}
	#endif						 /* HAVE_PGSQL */
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
			newElement = parseBuf (target, target_pos);
			// "}" will result in NULL, which we capture here
			if (!newElement)
				break;
			newElement->setLen (cmdBufTop - commandStart);
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
		if (target)
			acqIfEl = new Rts2SEBAcquired (this, target->getObsTargetID ());
		else
			acqIfEl = new Rts2SEBAcquired (this, 1);
		// parse block..
		while (1)
		{
			newElement = parseBuf (target, target_pos);
			// "}" will result in NULL, which we capture here
			if (!newElement)
				break;
			newElement->setLen (cmdBufTop - commandStart);
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
				newElement = parseBuf (target, target_pos);
				// "}" will result in NULL, which we capture here
				if (!newElement)
					break;
				newElement->setLen (cmdBufTop - commandStart);
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
	else if (!strcmp (commandStart, COMMAND_BLOCK_FOR))
	{
		char *el;
		int max;
		Rts2ScriptElement *newElement;
		Rts2SEBFor *forEl;
		// test for block start..
		if (getNextParamInteger (&max))
			return NULL;
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		forEl = new Rts2SEBFor (this, max);
		// parse block..
		while (1)
		{
			newElement = parseBuf (target, target_pos);
			// "}" will result in NULL, which we capture here
			if (!newElement)
				break;
			newElement->setLen (cmdBufTop - commandStart);
			forEl->addElement (newElement);
		}
		return forEl;
	}
	else if (!strcmp (commandStart, COMMAND_WAIT_SOD))
	{
		char *el;
		int endSod;
		Rts2ScriptElement *newElement;
		Rts2WhileSod *forEl;
		// test for block start..
		if (getNextParamInteger (&endSod))
			return NULL;
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		forEl = new Rts2WhileSod (this, endSod);
		// parse block..
		while (1)
		{
			newElement = parseBuf (target, target_pos);
			// "}" will result in NULL, which we capture here
			if (!newElement)
				break;
			newElement->setLen (cmdBufTop - commandStart);
			forEl->addElement (newElement);
		}
		return forEl;
	}
	else if (!strcmp (commandStart, COMMAND_WAITFOR))
	{
		char *val;
		double tarval, range;
		if (getNextParamString (&val) || getNextParamDouble (&tarval)
			|| getNextParamDouble (&range))
			return NULL;
		return new Rts2SWaitFor (this, new_device, val, tarval, range);
	}
	else if (!strcmp (commandStart, COMMAND_SLEEP))
	{
		double sec;
		if (getNextParamDouble (&sec))
			return NULL;
		return new Rts2SSleep (this, sec);
	}
	else if (!strcmp (commandStart, COMMAND_TARGET_DISABLE))
	{
		return new Rts2SETDisable (this, target);
	}
	else if (!strcmp (commandStart, COMMAND_TAR_TEMP_DISAB))
	{
		int seconds;
		if (getNextParamInteger (&seconds))
			return NULL;
		return new Rts2SETTempDisable (this, target, seconds);
	}
	else if (!strcmp (commandStart, COMMAND_TAR_TEMP_DISAB))
	{
		int seconds;
		int bonus;
		if (getNextParamInteger (&seconds) || getNextParamInteger (&bonus))
			return NULL;
		return new Rts2SETTarBoost (this, target, seconds, bonus);
	}
	else if (!strcmp (commandStart, COMMAND_HEX))
	{
		double ra_size;
		double dec_size;
		if (getNextParamDouble (&ra_size) || getNextParamDouble (&dec_size))
			return NULL;
		char *el;
		Rts2SEHex *hexEl;
		Rts2ScriptElement *newElement;
		// test for block start..
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		hexEl = new Rts2SEHex (this, new_device, ra_size, dec_size);
		// parse block..
		while (1)
		{
			newElement = parseBuf (target, target_pos);
			// "}" will result in NULL, which we capture here
			if (!newElement)
				break;
			newElement->setLen (cmdBufTop - commandStart);
			hexEl->addElement (newElement);
		}
		// block can end by script end as well..
		return hexEl;
	}
	else if (!strcmp (commandStart, COMMAND_FXF))
	{
		double ra_size;
		double dec_size;
		if (getNextParamDouble (&ra_size) || getNextParamDouble (&dec_size))
			return NULL;
		char *el;
		Rts2SEFF *ffEl;
		Rts2ScriptElement *newElement;
		// test for block start..
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		ffEl = new Rts2SEFF (this, new_device, ra_size, dec_size);
		// parse block..
		while (1)
		{
			newElement = parseBuf (target, target_pos);
			// "}" will result in NULL, which we capture here
			if (!newElement)
				break;
			newElement->setLen (cmdBufTop - commandStart);
			ffEl->addElement (newElement);
		}
		// block can end by script end as well..
		return ffEl;
	}

	// setValue fallback
	else if (strchr (commandStart, '='))
	{
		return new Rts2ScriptElementChangeValue (this, new_device, commandStart);
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


int
Rts2Script::idle ()
{
	if (el_iter != elements.end ())
		return (*el_iter)->idleCall ();
	return NEXT_COMMAND_KEEP;
}
