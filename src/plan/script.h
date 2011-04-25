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

#ifndef __RTS2_SCRIPT__
#define __RTS2_SCRIPT__

#include "element.h"
#include "elementblock.h"
#include <config.h>

#include "../utils/counted_ptr.h"
#include "../utils/block.h"
#include "../utils/rts2command.h"
#include "../utils/rts2devclient.h"
#include "../utils/rts2target.h"
#include "../writers/image.h"

#include <list>

#define NEXT_COMMAND_NEXT              -2
#define NEXT_COMMAND_END_SCRIPT        -1
#define NEXT_COMMAND_KEEP               1
#define NEXT_COMMAND_WAITING            2
#define NEXT_COMMAND_RESYNC             3
#define NEXT_COMMAND_CHECK_WAIT         4
#define NEXT_COMMAND_PRECISION_FAILED   5
// you should not return NEXT_COMMAND_PRECISION_OK on first nextCommand call
#define NEXT_COMMAND_PRECISION_OK       6
#define NEXT_COMMAND_WAIT_ACQUSITION    7
#define NEXT_COMMAND_ACQUSITION_IMAGE   8

#define NEXT_COMMAND_WAIT_SIGNAL        9
#define NEXT_COMMAND_WAIT_MIRROR        10

#define NEXT_COMMAND_WAIT_SEARCH        11

// when return value contains that mask, we will keep command,
// as we get it from block
#define NEXT_COMMAND_MASK_BLOCK         0x100000

#define EVENT_OK_ASTROMETRY             RTS2_LOCAL_EVENT + 200
#define EVENT_NOT_ASTROMETRY            RTS2_LOCAL_EVENT + 201
#define EVENT_ALL_PROCESSED             RTS2_LOCAL_EVENT + 202
#define EVENT_AFTER_COMMAND_FINISHED    RTS2_LOCAL_EVENT + 203

namespace rts2script
{

class Element;

/**
 * Thrown by parsing algorithm whenever error during parsing is encountered.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ParsingError:public rts2core::Error
{
	public:
		ParsingError (std::string _msg):rts2core::Error (_msg) {}
};

class UnknowOperantMultiplier:public ParsingError
{
	public:
		UnknowOperantMultiplier (char multiplier):ParsingError (std::string ("unknow multiplier '") + multiplier + "'") {} 
};

/**
 * Types of script output.
 */
typedef enum { PRINT_TEXT, PRINT_XML, PRINT_SCRIPT } printType;

/**
 * Holds script to execute on given device.
 * Script might include commands to other devices; in such case device
 * name must be given in script, separated from command with .
 * (point).
 *
 * It should be extended to enable actions depending on results of
 * observations etc..now we have only small core to build on.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Script:public Rts2Object, public std::list <Element *>
{
	public:
		Script (int scriptLoopCount = 0, rts2core::Block * _master = NULL);
		Script (const char *script);
		virtual ~ Script (void);

		/**
		 * Parse target script.
		 */
		void parseScript (Rts2Target *target, struct ln_equ_posn *target_pos);
		/**
		 * Get script from target, create vector of script elements and
		 * script string, which will be put to FITS header.
		 *
		 * @params cam_name Name of the camera.
		 * @params target  Script target. It is used to suply informations to script.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int setTarget (const char *cam_name, Rts2Target *target);

		virtual void postEvent (Rts2Event * event);
		template < typename T > int nextCommand (T & device, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		// returns -1 when there wasn't any error, otherwise index of element that wasn't parsed
		int getFaultLocation ()
		{
			if (*cmdBufTop == '\0')
				return -1;
			return (cmdBufTop - cmdBuf);
		}
		int isLastCommand (void) { return (el_iter == end ()); }

		void getDefaultDevice (char new_device[DEVICE_NAME_SIZE]) { strncpy (new_device, defaultDevice, DEVICE_NAME_SIZE); }

		char *getDefaultDevice () { return defaultDevice; }
		
		void exposureEnd ();

		int processImage (rts2image::Image * image);

		rts2core::Block *getMaster ()	{ return master; }

		int getExecutedCount () { return executedCount; }

		const char *getScriptBuf () { return cmdBuf; }
		/**
		 * Returns script for FITS header.
		 *
		 * @return Script text for FITS header file.
		 */
		const std::string getWholeScript () { return wholeScript; }

		int getParsedStartPos () { return commandStart - cmdBuf + lineOffset; }

		int idle ();

		void prettyPrint (std::ostream &os, printType pt);

		std::list <Element *>::iterator findElement (const char *name, std::list <Element *>::iterator start);

		/**
		 * Return expected script duration in seconds.
		 */
		double getExpectedDuration ();

		int getLoopCount () { return loopCount; }

	private:
		char *cmdBuf;
		std::string wholeScript;
		char *cmdBufTop;
		char *commandStart;
		int loopCount;

		char defaultDevice[DEVICE_NAME_SIZE];

		Element *currElement;

		// test whenewer next element is one that is given..
		bool isNext (const char *element);
		char *nextElement ();
		int getNextParamString (char **val);
		int getNextParamFloat (float *val);
		int getNextParamDouble (double *val);
		int getNextParamInteger (int *val);
		// we should not save reference to target, as it can be changed|deleted without our knowledge
		Element *parseBuf (Rts2Target * target, struct ln_equ_posn *target_pos);

		/**
		 * Parse block surrounded with {}
		 *
		 * @param el          ElementBlock where elements from block will be stored.
		 * @param target      Passed from parseBuf. Target for observation.
		 * @param target_pos  Passed from parseBuf. Target position.
		 */
		void parseBlock (ElementBlock *el, Rts2Target * target, struct ln_equ_posn *target_pos);

		/**
		 * Parse string as operand, returns reference to newly created operand.
		 */
		rts2operands::Operand *parseOperand (Rts2Target * target, struct ln_equ_posn *target_pos, rts2operands::Operand *op = NULL);

		std::list < Element * >::iterator el_iter;
		rts2core::Block *master;

		// counts comments
		int commentNumber;
		// is >= 0 when script runs, will become -1 when script is deleted (in beging of script destructor
		int executedCount;

		// offset for scripts spanning more than one line
		int lineOffset;
};

template < typename T > int Script::nextCommand (T & device, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	int ret;

	*new_command = NULL;

	if (executedCount < 0)
	{
		return NEXT_COMMAND_END_SCRIPT;
	}

	while (1)
	{
		if (el_iter == end ())
			// command not found, end of script,..
			return NEXT_COMMAND_END_SCRIPT;
		currElement = *el_iter;
		ret = currElement->nextCommand (&device, new_command, new_device);
		// send info about currently executed script element..
		device.queCommand (new Rts2CommandChangeValue (&device, "scriptPosition", '=', currElement->getStartPos ()));
		
		device.queCommand (new Rts2CommandChangeValue (&device, "scriptLen", '=', currElement->getLen ()));

		if (ret != NEXT_COMMAND_NEXT)
		{
			break;
		}
		// move to next command
		el_iter++;
	}
	if (ret & NEXT_COMMAND_MASK_BLOCK)
	{
		ret &= ~NEXT_COMMAND_MASK_BLOCK;
	}
	else
	{
		switch (ret)
		{
			case 0:
			case NEXT_COMMAND_CHECK_WAIT:
			case NEXT_COMMAND_PRECISION_FAILED:
			case NEXT_COMMAND_PRECISION_OK:
			case NEXT_COMMAND_WAIT_ACQUSITION:
				el_iter++;
				currElement = NULL;
				break;
			case NEXT_COMMAND_WAITING:
				*new_command = NULL;
				break;
			case NEXT_COMMAND_KEEP:
			case NEXT_COMMAND_RESYNC:
			case NEXT_COMMAND_ACQUSITION_IMAGE:
			case NEXT_COMMAND_WAIT_SIGNAL:
			case NEXT_COMMAND_WAIT_MIRROR:
			case NEXT_COMMAND_WAIT_SEARCH:
				// keep us
				break;
		}
	}
	if (ret != NEXT_COMMAND_NEXT)
		executedCount++;
	return ret;
};

typedef counted_ptr <Script> ScriptPtr;

}
#endif							 /* ! __RTS2_SCRIPT__ */
