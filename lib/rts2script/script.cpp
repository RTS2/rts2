/*
 * Script support.
 * Copyright (C) 2005-2007,2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "rts2script/script.h"

#include "elementexe.h"
#include "elementhex.h"
#include "elementwaitfor.h"
#include "configuration.h"

#include "rts2script/operands.h"
#include "rts2script/elementblock.h"
#include "rts2script/elementtarget.h"

#ifdef RTS2_HAVE_PGSQL
#include "rts2script/elementacquire.h"
#endif							 /* RTS2_HAVE_PGSQL */

#include "rts2db/scriptcommands.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>

using namespace rts2script;
using namespace rts2image;

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

Script::Script (int scriptLoopCount, rts2core::Block * _master):Object ()
{
	defaultDevice[0] = '\0';
	master = _master;
	loopCount = scriptLoopCount;
	executedCount = 0;
	lineOffset = 0;
	cmdBuf = NULL;
	cmdBufTop = NULL;
	wholeScript = std::string ("");

	target_pos.ra = target_pos.dec = NAN;

	fullReadoutTime = 0;
	filterMovement = 0;
	telescopeSpeed = 0;
}

Script::Script (const char *script):Object ()
{
	defaultDevice[0] = '\0';
	master = NULL;
	loopCount = 0;
	executedCount = 0;
	lineOffset = 0;
	cmdBuf = new char[strlen (script) + 1];
	strcpy (cmdBuf, script);
	cmdBufTop = cmdBuf;
	wholeScript = std::string (script);
}

Script::~Script (void)
{
	notActive ();
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

void Script::parseScript (Rts2Target *target)
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
		element = parseBuf (target);
		if (!element)
			break;
		element->setLen (cmdBufTop - commandStart);
		lineOffset += cmdBufTop - commandStart;
		try
		{
			element->checkParameters ();
		}
		catch (ParsingError err)
		{
			delete element;
			throw err;
		}
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

	target->getPosition (&target_pos);

	strcpy (defaultDevice, cam_name);
	
	// set device specific values
	Configuration *config = Configuration::instance ();
	config->getFloat (cam_name, "readout_time", fullReadoutTime, fullReadoutTime);
	config->getFloat (cam_name, "filter_movement", filterMovement, filterMovement);
	config->getFloat ("observatory", "telescope_speed", telescopeSpeed, telescopeSpeed);
	
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
		parseScript (target);
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

void Script::postEvent (rts2core::Event * event)
{
	Script::iterator el_iter_sig;
	Element *el;
	int ret;
	switch (event->getType ())
	{
		case EVENT_SIGNAL:
			for (el_iter_sig = el_iter; el_iter_sig != end (); el_iter_sig++)
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
			for (el_iter_sig = el_iter; el_iter_sig != end (); el_iter_sig++)
			{
				el = *el_iter_sig;
				el->postEvent (new rts2core::Event (event));
			}
			break;
		default:
			if (el_iter != end ())
				(*el_iter)->postEvent (new rts2core::Event (event));
			break;
	}
	Object::postEvent (event);
}

void Script::errorReported (int current_state, int old_state)
{
	if (el_iter != end ())
		(*el_iter)->errorReported (current_state, old_state);
}

Element *Script::parseBuf (Rts2Target * target)
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
	char *opsep = index (commandStart, '=');
	if (devSep && (opsep == NULL || devSep < opsep))
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
	#ifdef RTS2_HAVE_PGSQL
	else if (!strcmp (commandStart, COMMAND_ACQUIRE))
	{
		double precision;
		float expTime;
		if (getNextParamDouble (&precision) || getNextParamFloat (&expTime))
			return NULL;
		// target is already acquired
		if (target && target->isAcquired ())
			return new ElementNone (this);
		return new ElementAcquire (this, precision, expTime, &target_pos);
	}
	#endif						 /* RTS2_HAVE_PGSQL */
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
		parseBlock (blockEl, target);
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
		parseBlock (acqIfEl, target);
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
				newElement = parseBuf (target);
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
		parseBlock (forEl, target);
		return forEl;
	}
	else if (!strcmp (commandStart, COMMAND_BLOCK_WHILE))
	{
		int max;
		rts2operands::Operand *condition = parseOperand (target);
		if (getNextParamInteger (&max))
			return NULL;
		el = nextElement ();
		if (*el != '{')
			return NULL;
		ElementWhile *whileEl = new ElementWhile (this, condition, max);
		parseBlock (whileEl, target);
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
		parseBlock (doEl, target);
		el = nextElement ();
		if (strcmp (el, COMMAND_BLOCK_WHILE))
			return NULL;
		rts2operands::Operand *condition = parseOperand (target);
		doEl->setCondition (condition);
		return doEl;
	}
	else if (!strcmp (commandStart, COMMAND_BLOCK_ONCE))
	{
		el = nextElement ();
		if (*el != '{')
			return NULL;
		ElementOnce *singleEl = new ElementOnce (this);
		parseBlock (singleEl, target);
		return singleEl;
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
		parseBlock (forEl, target);
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
		parseBlock (hexEl, target);
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
		parseBlock (ffEl, target);
		return ffEl;
	}
	else if (!strcmp (commandStart, COMMAND_EXE))
	{
		char *exe;
		if (getNextParamString (&exe))
			return NULL;
		return new Execute (this, getMaster (), exe, target);
	}
	else if (!strcmp (commandStart, COMMAND_COMMAND))
	{
		char *cmd;
		if (getNextParamString (&cmd))
			return NULL;
		return new ElementCommand (this, cmd);
	}

	// setValue fallback
	else if (strchr (commandStart, '='))
	{
		return new ElementChangeValue (this, new_device, commandStart);
	}
	return NULL;
}


void Script::parseBlock (ElementBlock *el, Rts2Target * target)
{
	while (1)
	{
		Element *newElement = parseBuf (target);
		// "}" will result in NULL, which we capture here
		if (!newElement)
			return;
		newElement->setLen (cmdBufTop - commandStart);
		el->addElement (newElement);
	}
}

rts2operands::Operand * Script::parseOperand (Rts2Target *target, rts2operands::Operand *op)
{
	while (isspace (*cmdBufTop))
		cmdBufTop++;

	if (*cmdBufTop == '(' )
	{
		do
			cmdBufTop++;
		while (isspace (*cmdBufTop));
		// find end of operand..
		return parseOperand (target);
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
			return parseOperand (target);
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
			rts2operands::Operand *op2 = parseOperand (target);
			return new rts2operands::OperandsLREquation (op, cmp, op2);
		}
		// find out what character represents..
		else if (isdigit (*cmdBufTop) || (*cmdBufTop == '.' && isdigit (cmdBufTop[1])))
		{
			while (*cmdBufTop != '\0' && !isspace (*cmdBufTop))
				cmdBufTop++;
			char *num = new char[cmdBufTop - start + 1];
			strncpy (num, start, cmdBufTop - start);
			num[cmdBufTop - start] = '\0';

			rts2operands::OperandsSet os;
			op = os.parseOperand (std::string (num), rts2operands::MUL_ANGLE);
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

void Script::exposureEnd (bool expectImage)
{
	if (executedCount < 0 || el_iter == end ())
		return;
	return (*el_iter)->exposureEnd (expectImage);
}

void Script::exposureFailed ()
{
	if (executedCount < 0 || el_iter == end ())
		return;
	return (*el_iter)->exposureFailed ();
}

void Script::notActive ()
{
	for (Script::iterator iter = begin (); iter != end (); iter++)
		(*iter)->notActive ();
}

int Script::processImage (Image * image)
{
	if (executedCount < 0 || el_iter == end ())
		return -1;
	return (*el_iter)->processImage (image);
}

bool Script::knowImage (Image * image)
{
	for (Script::iterator iter = begin (); iter != end (); iter++)
		if ((*iter)->knowImage (image))
			return true;
	return false;
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
		case PRINT_JSON:
			os << "[";
			for (el_iter = begin (); el_iter != end (); el_iter++)
			{
				if (el_iter != begin ())
					os << ",{";
				else
					os << "{";
				(*el_iter)->printJson (os);
				os << "}";
			}
			os << "]";
			break;
	}
}

Script::iterator Script::findElement (const char *name, Script::iterator start)
{
	Script::iterator iter;
	for (iter = start; iter != end (); iter++)
	{
		std::ostringstream os;
		(*iter)->printScript (os);
		if (os.str ().find (name) == 0)
			return iter;
	}
	return iter;
}

double Script::getExpectedDuration (struct ln_equ_posn *tel)
{
	double ret = 0;
	if (tel && !isnan (target_pos.ra) && !isnan (target_pos.dec))
	{
		double dist = ln_get_angular_separation (tel, &target_pos);
		ret += dist * getTelescopeSpeed ();
	}
	for (Script::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getExpectedDuration ();
	return ret;
}

double Script::getExpectedLightTime ()
{
	double ret = 0;
	for (Script::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getExpectedLightTime ();
	return ret;
}

int Script::getExpectedImages ()
{
	int ret = 0;
	for (Script::iterator iter = begin (); iter != end (); iter++)
		ret += (*iter)->getExpectedImages ();
	return ret;
}

double rts2script::getMaximalScriptDuration (Rts2Target *tar, rts2db::CamList &cameras, struct ln_equ_posn *tel)
{
  	double md = 0;
	for (rts2db::CamList::iterator cam = cameras.begin (); cam != cameras.end (); cam++)
	{
		std::string script_buf;
		rts2script::Script script;
		tar->getScript (cam->c_str(), script_buf);
		script.setTarget (cam->c_str (), tar);
		double d = script.getExpectedDuration (tel);
		if (d > md)
			md = d;  
	}
	return md;
}
