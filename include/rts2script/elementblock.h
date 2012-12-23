/*
 * Wait for some event or for a number of seconds.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_SCRIPT_BLOCK__
#define __RTS2_SCRIPT_BLOCK__

#include "rts2script/operands.h"
#include "rts2script/element.h"

namespace rts2script
{

class Script;

/**
 * Block - text surrounded by {}.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementBlock:public Element
{
	public:
		ElementBlock (Script * _script);
		virtual ~ ElementBlock (void);

		virtual void addElement (Element * element);

		virtual void postEvent (rts2core::Event * event);

		virtual int defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (rts2core::DevClientCamera * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (rts2core::DevClientPhot * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void errorReported (int current_state, int old_state);

		virtual void exposureEnd (bool expectImage);
		virtual int processImage (rts2image::Image * image);
		virtual int waitForSignal (int _sig);
		virtual void cancelCommands ();
		virtual void beforeExecuting ();

		virtual int getStartPos ();
		virtual int getLen ();

		virtual int idleCall ();

		virtual void prettyPrint (std::ostream &os);
		virtual void printXml (std::ostream &os);

		virtual void printScript (std::ostream &os);

		virtual double getExpectedDuration (int runnum);
		virtual double getExpectedLightTime ();
		virtual int getExpectedImages ();

	protected:
		std::list < Element * > blockElements;
		std::list < Element * >::iterator curr_element;
		int blockScriptRet (int ret);
		int loopCount;

		virtual bool endLoop () { return true; }

		virtual bool getNextLoop ()
		{
			bool ret = endLoop ();
			if (ret)
				afterBlockEnd ();
			return ret;
		}

		int getLoopCount () { return loopCount; }

		// called after one loop ends
		// prepare us for next execution
		virtual void afterBlockEnd ();
};

/**
 * Will wait for signal, after signal we will end execution of that block.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementSignalEnd:public ElementBlock
{
	public:
		ElementSignalEnd (Script * _script, int end_sig_num);
		virtual ~ ElementSignalEnd (void);

		virtual int waitForSignal (int _sig);

		virtual void printScript (std::ostream &os) { os << COMMAND_BLOCK_WAITSIG " " << sig_num; ElementBlock::printScript (os); }
	protected:
		virtual bool endLoop ()
		{
			return (sig_num == -1);
		}
	private:
		// sig_num will become -1, when we get signal..
		int sig_num;
};

/**
 * Will test if target is acquired. Can be acompained with else block, that
 * will be executed when
 */
class ElementAcquired:public ElementBlock
{
	public:
		ElementAcquired (Script * _script, int _tar_id);
		virtual ~ ElementAcquired (void);

		virtual void postEvent (rts2core::Event * event);

		virtual int defnextCommand (rts2core::DevClient * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (rts2core::DevClientCamera * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (rts2core::DevClientPhot * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		virtual int processImage (rts2image::Image * image);
		virtual int waitForSignal (int _sig);
		virtual void cancelCommands ();

		virtual void addElseElement (Element * element);

		virtual void printScript (std::ostream &os);

	protected:
		virtual bool endLoop ();
	private:
		ElementBlock * elseBlock;
		int tar_id;
		enum { NOT_CALLED, ACQ_OK, ACQ_FAILED } state;
		void checkState ();
};

/**
 * Else block for storing else path..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementElse:public ElementBlock
{
	public:
		ElementElse (Script * _script):ElementBlock (_script) {}
	protected:
		virtual bool endLoop ();
};

/**
 * For loop.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementFor:public ElementBlock
{
	public:
		ElementFor (Script * _script, int _max):ElementBlock (_script) { max = _max; }
		virtual void prettyPrint (std::ostream &os);
		virtual void printXml (std::ostream &os);
		virtual void printScript (std::ostream &os);
		virtual void printJson (std::ostream &os);

		virtual double getExpectedDuration (int runnum);
		virtual double getExpectedLightTime ();
		virtual int getExpectedImages ();

	protected:
		virtual bool endLoop () { return getLoopCount () >= max; }
	private:
		int max;
};

/**
 * Do while current second of day is lower then requested second of day
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementWhileSod:public ElementBlock
{
	public:
		ElementWhileSod (Script * _script, int _endSod):ElementBlock (_script) { endSod = _endSod; }

		virtual ~ElementWhileSod (void) {}

		virtual int nextCommand (rts2core::DevClientCamera * client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual void printScript (std::ostream &os);
	protected:
		virtual bool endLoop ()
		{
			time_t now;
			time (&now);
			struct tm *tm_sod = gmtime (&now);
			int currSod = tm_sod->tm_sec + tm_sod->tm_min * 60 + tm_sod->tm_hour * 3600;
			return endSod <= currSod;
		}
	private:
		// end second of day (UT)
		int endSod;
};

/**
 * Repeat commands while condition hold true.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementWhile:public ElementBlock
{
	public:
		ElementWhile (Script * _script, rts2operands::Operand *_condition, int _max_cycles):ElementBlock (_script) { condition = _condition; max_cycles = _max_cycles; }
		virtual ~ElementWhile () { delete condition; }

		virtual void printScript (std::ostream &os);
	protected:
		virtual bool endLoop ();
	private:
		rts2operands::Operand *condition;
		int max_cycles;
};

/**
 * Repeat command while condition hold true. Test condition after first execution.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementDo:public ElementBlock
{
	public:
		ElementDo (Script *_script, int _max_cycles):ElementBlock (_script) { max_cycles = _max_cycles; condition = NULL; }
		virtual ~ElementDo () { delete condition; }
		void setCondition (rts2operands::Operand *_condition) { condition = _condition; }

		virtual void printScript (std::ostream &os);
	protected:
		virtual bool endLoop ();
	private:
		rts2operands::Operand *condition;
		int max_cycles;
};

/**
 * Run command only if it is the first script execution.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ElementOnce:public ElementBlock
{
	public:
		ElementOnce (Script *_script):ElementBlock (_script) {}
		virtual ~ElementOnce () {}

		virtual void printScript (std::ostream &os);
	protected:
		virtual bool endLoop ();
};

}
#endif							 /* !__RTS2_SCRIPT_BLOCK__ */
