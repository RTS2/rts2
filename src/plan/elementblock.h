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

#ifndef __RTS2_SCRIPT_BLOCK__
#define __RTS2_SCRIPT_BLOCK__

#include "script.h"

namespace rts2script
{

/**
 * Block - text surrounded by {}.
 *
 * @author petr
 */
class ElementBlock:public Element
{
	public:
		ElementBlock (Script * in_script);
		virtual ~ ElementBlock (void);

		virtual void addElement (Element * element);

		virtual void postEvent (Rts2Event * event);

		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientCamera * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientPhot * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int processImage (Rts2Image * image);
		virtual int waitForSignal (int in_sig);
		virtual void cancelCommands ();
		virtual void beforeExecuting ();

		virtual int getStartPos ();
		virtual int getLen ();

		virtual int idleCall ();

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
 */
class ElementSignalEnd:public ElementBlock
{
	public:
		ElementSignalEnd (Script * in_script, int end_sig_num);
		virtual ~ ElementSignalEnd (void);

		virtual int waitForSignal (int in_sig);
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
		ElementAcquired (Script * in_script, int in_tar_id);
		virtual ~ ElementAcquired (void);

		virtual void postEvent (Rts2Event * event);

		virtual int defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientCamera * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientPhot * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);

		virtual int processImage (Rts2Image * image);
		virtual int waitForSignal (int in_sig);
		virtual void cancelCommands ();

		virtual void addElseElement (Element * element);
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
 */
class ElementElse:public ElementBlock
{
	public:
		ElementElse (Script * in_script):ElementBlock (in_script) {}
	protected:
		virtual bool endLoop ();
};

class ElementFor:public ElementBlock
{
	public:
		ElementFor (Script * in_script, int in_max):ElementBlock (in_script) { max = in_max; }
	protected:
		virtual bool endLoop () { return getLoopCount () >= max; }
	private:
		int max;
};

/**
 * Do while current second of day is lower then requested second of day
 */
class ElementWhileSod:public ElementBlock
{
	public:
		ElementWhileSod (Script * in_script, int in_endSod):ElementBlock (in_script) { endSod = in_endSod; }

		virtual ~ElementWhileSod (void) {}

		virtual int nextCommand (Rts2DevClientCamera * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
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

}
#endif							 /* !__RTS2_SCRIPT_BLOCK__ */
