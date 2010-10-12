/*
 * Script support.
 * Copyright (C) 2005-2007,2009 Petr Kubanek <petr@kubanek.net>
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
#include "elementexe.h"
#include "elementtarget.h"
#include "elementblock.h"
#include "elementguiding.h"
#include "elementhex.h"
#include "elementwaitfor.h"
#include "operands.h"

#ifdef HAVE_PGSQL
#include "elementacquire.h"
#endif							 /* HAVE_PGSQL */

#include "../utilsdb/scriptcommands.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>

using namespace rts2script;

// test if next element is one that is given
bool Script::isNext (const char *element)
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

char *Script::nextElement ()
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

int Script::getNextParamString (char **val)
{
	*val = nextElement ();
	if (!*val)
		return -1;
	return 0;
}

int Script::getNextParamFloat (float *val)
{
	char *el;
	el = nextElement ();
	if (!*el)
		return -1;
	*val = atof (el);
	return 0;
}

int Script::getNextParamDouble (double *val)
{
	char *el;
	el = nextElement ();
	if (!*el)
		return -1;
	*val = atof (el);
	return 0;
}

int Script::getNextParamInteger (int *val)
{
	char *el;
	el = nextElement ();
	if (!*el)
		return -1;
	*val = atoi (el);
	return 0;
}

Script::Script (Rts2Block * _master):Rts2Object ()
{
	defaultDevice[0] = '\0';
	master = _master;
	executedCount = 0;
	lineOffset = 0;
	cmdBuf = NULL;
	cmdBufTop = NULL;
	wholeScript = std::string ("");
}

Script::Script (const char *script):Rts2Object ()
{
	defaultDevice[0] = '\0';
	master = NULL;
	executedCount = 0;
	lineOffset = 0;
	cmdBuf = new char[strlen (script) + 1];
	strcpy (cmdBuf, script);
	cmdBufTop = cmdBuf;
	wholeScript = std::string (script);
}

Script::~Script (void)
{
	// all operations with elements list should be ignored
	executedCount = -1;
	for (el_iter = begin (); el_iter != end (); el_iter++)
	{
		Element *el;
		el = *el_iter;
		delete el;
	}
	clear ();
	delete[] cmdBuf;
}

void Script::parseScript (Rts2Target *target, struct ln_equ_posn *target_pos)
{
	char *comment = NULL;
	Element *element;
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
		element = parseBuf (target, target_pos);
		if (!element)
			break;
		element->setLen (cmdBufTop - commandStart);
		lineOffset += cmdBufTop - commandStart;
		push_back (element);
	}

	// add comment if there was one
	if (comment)
	{
		element = new ElementComment (this, comment, commentNumber);
		std::ostringstream ws;
		ws << "#" << commentNumber << " ";
		if (wholeScript.length () > 0)
		{
			wholeScript += " ";
			lineOffset++;
		}
		wholeScript += ws.str ();
		commentNumber++;

		element->setLen (2);
		lineOffset += 3;
		push_back (element);
	}
}

int Script::setTarget (const char *cam_name, Rts2Target * target)
{
	std::string scriptText;
	struct ln_equ_posn target_pos;

	target->getPosition (&target_pos);

	strcpy (defaultDevice, cam_name);
	commentNumber = 1;
	wholeScript = std::string ("");

	int ret = 1;
	do
	{
		delete[] cmdBuf;

		try
		{
			ret = target->getScript (cam_name, scriptText);
		}
		catch (rts2core::Error &er)
		{
			logStream (MESSAGE_ERROR) << "cannot load script for device " << cam_name << " and target " << target->getTargetName () << "(# " << target->getTargetID () << sendLog;
			el_iter = begin ();
			return -1;
		}

		cmdBuf = new char[scriptText.length () + 1];
		strcpy (cmdBuf, scriptText.c_str ());
		parseScript (target, &target_pos);
		// ret == 0 if this was the last (or only) line of the script - see target->getScript call comments.
	} while (ret == 1);

	executedCount = 0;
	currElement = NULL;
	for (el_iter = begin (); el_iter != end (); el_iter++)
	{
		(*el_iter)->beforeExecuting ();
	}
	el_iter = begin ();
	return 0;
}

void Script::postEvent (Rts2Event * event)
{
	std::list < Element * >::iterator el_iter_sig;
	Element *el;
	int ret;
	switch (event->getType ())
	{
		case EVENT_SIGNAL:
			for (el_iter_sig = el_iter; el_iter_sig != end ();
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
			for (el_iter_sig = el_iter; el_iter_sig != end ();
				el_iter_sig++)
			{
				el = *el_iter_sig;
				el->postEvent (new Rts2Event (event));
			}
			break;
		default:
			if (el_iter != end ())
				(*el_iter)->postEvent (new Rts2Event (event));
			break;
	}
	Rts2Object::postEvent (event);
}

Element *Script::parseBuf (Rts2Target * target, struct ln_equ_posn *target_pos)
{
	char *devSep;
	char *el;
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
		return new ElementExpose (this, exp_time);
	}
	else if (!strcmp (commandStart, COMMAND_DARK))
	{
		float exp_time;
		ret = getNextParamFloat (&exp_time);
		if (ret)
			return NULL;
		return new ElementDark (this, exp_time);
	}
	else if (!strcmp (commandStart, COMMAND_BOX))
	{
		int x, y, w, h;
		if (getNextParamInteger (&x)
			|| getNextParamInteger (&y)
			|| getNextParamInteger (&w) || getNextParamInteger (&h))
			return NULL;
		return new ElementBox (this, x, y, w, h);
	}
	else if (!strcmp (commandStart, COMMAND_CENTER))
	{
		int w, h;
		if (getNextParamInteger (&w) || getNextParamInteger (&h))
			return NULL;
		return new ElementCenter (this, w, h);
	}
	else if (!strcmp (commandStart, COMMAND_CHANGE))
	{
		double ra;
		double dec;
		if (getNextParamDouble (&ra) || getNextParamDouble (&dec))
			return NULL;
		return new ElementChange (this, new_device, ra, dec);
	}
	else if (!strcmp (commandStart, COMMAND_WAIT))
	{
		return new ElementWait (this);
	}
	else if (!strcmp (commandStart, COMMAND_WAIT_ACQUIRE))
	{
		return new ElementWaitAcquire (this,
			target->getObsTargetID ());
	}
	else if (!strcmp (commandStart, COMMAND_PHOTOMETER))
	{
		int filter;
		float exposure;
		int count;
		if (getNextParamInteger (&filter) || getNextParamFloat (&exposure)
			|| getNextParamInteger (&count))
			return NULL;
		return new ElementPhotometer (this, filter, exposure, count);
	}
	else if (!strcmp (commandStart, COMMAND_SEND_SIGNAL))
	{
		int signalNum;
		if (getNextParamInteger (&signalNum))
			return NULL;
		return new ElementSendSignal (this, signalNum);
	}
	else if (!strcmp (commandStart, COMMAND_WAIT_SIGNAL))
	{
		int signalNum;
		if (getNextParamInteger (&signalNum))
			return NULL;
		if (signalNum <= 0)
			return NULL;
		return new ElementWaitSignal (this, signalNum);
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
			return new ElementNone (this);
		return new ElementAcquire (this, precision, expTime, target_pos);
	}
	else if (!strcmp (commandStart, COMMAND_HAM))
	{
		int repNumber;
		float exposure;
		if (getNextParamInteger (&repNumber) || getNextParamFloat (&exposure))
			return NULL;
		return new ElementAcquireHam (this, repNumber, exposure, target_pos);
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
		return new ElementAcquireStar (this, repNumber, precision, exposure, scale, scale, target_pos);
	}
	#endif						 /* HAVE_PGSQL */
	else if (!strcmp (commandStart, COMMAND_BLOCK_WAITSIG))
	{
		int waitSig;
		ElementBlock *blockEl;
		if (getNextParamInteger (&waitSig))
			return NULL;
		// test for block start..
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		blockEl = new ElementSignalEnd (this, waitSig);
		parseBlock (blockEl, target, target_pos);
		return blockEl;
	}
	else if (!strcmp (commandStart, COMMAND_BLOCK_ACQ))
	{
		ElementAcquired *acqIfEl;
		// test for block start..
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		if (target)
			acqIfEl = new ElementAcquired (this, target->getObsTargetID ());
		else
			acqIfEl = new ElementAcquired (this, 1);
		parseBlock (acqIfEl, target, target_pos);
		// test for if..
		if (isNext (COMMAND_BLOCK_ELSE))
		{
			el = nextElement ();
			if (*el != '{')
				return NULL;
			Element *newElement;
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
		return new ElementGuiding (this, init_exposure, end_signal);
	}
	else if (!strcmp (commandStart, COMMAND_BLOCK_FOR))
	{
		int max;
		ElementFor *forEl;
		// test for block start..
		if (getNextParamInteger (&max))
			return NULL;
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		forEl = new ElementFor (this, max);
		parseBlock (forEl, target, target_pos);
		return forEl;
	}
	else if (!strcmp (commandStart, COMMAND_BLOCK_WHILE))
	{
		int max;
		rts2operands::Operand *condition = parseOperand (target, target_pos);
		if (getNextParamInteger (&max))
			return NULL;
		el = nextElement ();
		if (*el != '{')
			return NULL;
		ElementWhile *whileEl = new ElementWhile (this, condition, max);
		parseBlock (whileEl, target, target_pos);
		return whileEl;
	}
	else if (!strcmp (commandStart, COMMAND_BLOCK_DO))
	{
		int max;
		if (getNextParamInteger (&max))
			return NULL;
		el = nextElement ();
		if (*el != '{')
			return NULL;
		ElementDo *doEl = new ElementDo (this, max);
		parseBlock (doEl, target, target_pos);
		el = nextElement ();
		if (strcmp (el, COMMAND_BLOCK_WHILE))
			return NULL;
		rts2operands::Operand *condition = parseOperand (target, target_pos);
		doEl->setCondition (condition);
		return doEl;
	}
	else if (!strcmp (commandStart, COMMAND_WAIT_SOD))
	{
		int endSod;
		ElementWhileSod *forEl;
		// test for block start..
		if (getNextParamInteger (&endSod))
			return NULL;
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		forEl = new ElementWhileSod (this, endSod);
		// parse block..
		parseBlock (forEl, target, target_pos);
		return forEl;
	}
	else if (!strcmp (commandStart, COMMAND_WAITFOR))
	{
		char *val;
		double tarval, range;
		if (getNextParamString (&val) || getNextParamDouble (&tarval)
			|| getNextParamDouble (&range))
			return NULL;
		return new ElementWaitFor (this, new_device, val, tarval, range);
	}
	else if (!strcmp (commandStart, COMMAND_SLEEP))
	{
		double sec;
		if (getNextParamDouble (&sec))
			return NULL;
		return new ElementSleep (this, sec);
	}
	else if (!strcmp (commandStart, COMMAND_WAIT_FOR_IDLE))
	{
		return new ElementWaitForIdle (this);
	}
	else if (!strcmp (commandStart, COMMAND_TARGET_DISABLE))
	{
		return new ElementDisable (this, target);
	}
	else if (!strcmp (commandStart, COMMAND_TAR_TEMP_DISAB))
	{
		char *distime;
		if (getNextParamString (&distime))
			return NULL;
		return new ElementTempDisable (this, target, distime);
	}
	else if (!strcmp (commandStart, COMMAND_TAR_TEMP_DISAB))
	{
		int seconds;
		int bonus;
		if (getNextParamInteger (&seconds) || getNextParamInteger (&bonus))
			return NULL;
		return new ElementTarBoost (this, target, seconds, bonus);
	}
	else if (!strcmp (commandStart, COMMAND_HEX))
	{
		double ra_size;
		double dec_size;
		if (getNextParamDouble (&ra_size) || getNextParamDouble (&dec_size))
			return NULL;
		ElementHex *hexEl;
		// test for block start..
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		hexEl = new ElementHex (this, new_device, ra_size, dec_size);
		parseBlock (hexEl, target, target_pos);
		return hexEl;
	}
	else if (!strcmp (commandStart, COMMAND_FXF))
	{
		double ra_size;
		double dec_size;
		if (getNextParamDouble (&ra_size) || getNextParamDouble (&dec_size))
			return NULL;
		ElementFxF *ffEl;
		// test for block start..
		el = nextElement ();
		// error, return NULL
		if (*el != '{')
			return NULL;
		ffEl = new ElementFxF (this, new_device, ra_size, dec_size);
		parseBlock (ffEl, target, target_pos);
		return ffEl;
	}
	else if (!strcmp (commandStart, COMMAND_EXE))
	{
		char *exe;
		if (getNextParamString (&exe))
			return NULL;
		return new Execute (this, getMaster (), exe);
	}

	// setValue fallback
	else if (strchr (commandStart, '='))
	{
		return new ElementChangeValue (this, new_device, commandStart);
	}
	return NULL;
}


void Script::parseBlock (ElementBlock *el, Rts2Target * target, struct ln_equ_posn *target_pos)
{
	while (1)
	{
		Element *newElement = parseBuf (target, target_pos);
		// "}" will result in NULL, which we capture here
		if (!newElement)
			return;
		newElement->setLen (cmdBufTop - commandStart);
		el->addElement (newElement);
	}
}

rts2operands::Operand * Script::parseOperand (Rts2Target *target, struct ln_equ_posn *target_pos, rts2operands::Operand *op)
{
	while (isspace (*cmdBufTop))
		cmdBufTop++;

	if (*cmdBufTop == '(' )
	{
		do
			cmdBufTop++;
		while (isspace (*cmdBufTop));
		// find end of operand..
		return parseOperand (target, target_pos);
	}

	while (*cmdBufTop != '\0')
	{
		while (isspace (*cmdBufTop))
			cmdBufTop++;

		char *start = cmdBufTop;

		if (*cmdBufTop == ')' )
		{
			if (op == NULL)
				throw ParsingError ("missing expression inside ()");
			cmdBufTop++;
			return op;
		}
		else if (*cmdBufTop == '(' )
		{
			do
				cmdBufTop++;
			while (isspace (*cmdBufTop));
			// find end of operand..
			return parseOperand (target, target_pos);
		}
		else if (*cmdBufTop == '=' || *cmdBufTop == '>' || *cmdBufTop == '<')
		{
			if (op == NULL)
				throw ParsingError ("expected operand before operator");

			rts2operands::comparators cmp;

			char cmp1 = cmdBufTop[0];
			cmdBufTop++;
			char cmp2 = cmdBufTop[0];
			if (cmp2 == '=')
			{
				if (cmp1 == '=')
					cmp = rts2operands::CMP_EQUAL;
				else if (cmp1 == '<')
					cmp = rts2operands::CMP_LESS_EQU;
				else
					cmp = rts2operands::CMP_GREAT_EQU;
				cmdBufTop++;
			}
			else if (cmp1 == '<')
			{
				cmp = rts2operands::CMP_LESS;
			}
			else if (cmp1 == '>')
			{
				cmp = rts2operands::CMP_GREAT;
			}
			else 
			{
				throw ParsingError ("In conditions, only == is allowed");
			}
			rts2operands::Operand *op2 = parseOperand (target, target_pos);
			return new rts2operands::OperandsLREquation (op, cmp, op2);
		}
		// find out what character represents..
		else if (isdigit (*cmdBufTop) || (*cmdBufTop == '.' && isdigit (cmdBufTop[1])))
		{
			while (isdigit (*cmdBufTop) || *cmdBufTop == '.')
				cmdBufTop++;
			char *num = new char[cmdBufTop - start + 1];
			strncpy (num, start, cmdBufTop - start);
			num[cmdBufTop - start] = '\0';
			// units..
			double multi = rts2_nan("f");
			if (*cmdBufTop == 'm')
				multi = 1/60.0;
			else if (*cmdBufTop == 's')
				multi = 1/3600.0;
			else if (*cmdBufTop == 'h')
				multi = 15;
			else if (*cmdBufTop == 'd')
				multi = 1;
	
			if (!isnan (multi))
				*cmdBufTop++;
			else
				multi = 1;
			op = new rts2operands::Number (atof (num) * multi);
			delete[] num;
		}
		else if (isalpha (*cmdBufTop) || *cmdBufTop == '.')
		{
			// it is either function or variable name..
			char *point = NULL;
			while (isalnum (*cmdBufTop) || *cmdBufTop == '.' || *cmdBufTop == '_')
			{
				if (*cmdBufTop == '.' && !point)
					point = cmdBufTop;
				cmdBufTop++;
			}
			if (point != NULL)
			{
				char *_val = new char[cmdBufTop - point];
				strncpy (_val, point + 1, cmdBufTop - point - 1);
				_val[cmdBufTop - point - 1] = '\0';
				// use default device name
				if (point == start)
				{
					op = new rts2operands::SystemValue (getMaster (), defaultDevice, _val);
				}
				else
				{
					// split by .
					*point = '\0';
					op = new rts2operands::SystemValue (getMaster (), start, _val);
				}
				delete[] _val;
			}
		}
		else
		{
			throw ParsingError ("cannot parse sequence");
		}
	}
	return op;
}

void Script::exposureEnd ()
{
	if (executedCount < 0 || el_iter == end ())
		return;
	return (*el_iter)->exposureEnd ();
}

int Script::processImage (Rts2Image * image)
{
	if (executedCount < 0 || el_iter == end ())
		return -1;
	return (*el_iter)->processImage (image);
}

int Script::idle ()
{
	if (el_iter != end ())
		return (*el_iter)->idleCall ();
	return NEXT_COMMAND_KEEP;
}

void Script::prettyPrint (std::ostream &os, printType pt)
{
	switch (pt)
	{
		case PRINT_TEXT:
			for (el_iter = begin (); el_iter != end (); el_iter++)
			{
				(*el_iter)->prettyPrint (os);
				os << std::endl;
			}
			break;
		case PRINT_XML:
			os << "<script device='" << defaultDevice << "'>" << std::endl;
			for (el_iter = begin (); el_iter != end (); el_iter++)
			{
				(*el_iter)->printXml (os);
				os << std::endl;
			}
			os << "</script>" << std::endl;
			break;
		case PRINT_SCRIPT:
			for (el_iter = begin (); el_iter != end (); el_iter++)
			{
				if (el_iter != begin ())
					os << " ";
				(*el_iter)->printScript (os);
			}
			break;
	}
}

std::list <Element *>::iterator Script::findElement (const char *name, std::list <Element *>::iterator start)
{
	std::list <Element *>::iterator iter;
	for (iter = start; iter != end (); iter++)
	{
		std::ostringstream os;
		(*iter)->printScript (os);
		if (os.str ().find (name) == 0)
			return iter;
	}
	return iter;
}
