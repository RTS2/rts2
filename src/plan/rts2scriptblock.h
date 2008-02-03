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

#include "rts2script.h"

/**
 * Block - text surrounded by {}.
 *
 * @author petr
 */
class Rts2ScriptElementBlock:public Rts2ScriptElement
{
	private:
		std::list < Rts2ScriptElement * >blockElements;
		std::list < Rts2ScriptElement * >::iterator curr_element;
		int blockScriptRet (int ret);
		int loopCount;
	protected:
		virtual bool endLoop ()
		{
			return true;
		}
		virtual bool getNextLoop ()
		{
			bool ret = endLoop ();
			if (ret)
				afterBlockEnd ();
			return ret;
		}
		int getLoopCount ()
		{
			return loopCount;
		}
		// called after one loop ends
		// prepare us for next execution
		virtual void afterBlockEnd ();
	public:
		Rts2ScriptElementBlock (Rts2Script * in_script);
		virtual ~ Rts2ScriptElementBlock (void);

		virtual void addElement (Rts2ScriptElement * element);

		virtual void postEvent (Rts2Event * event);

		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientCamera * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientPhot * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int processImage (Rts2Image * image);
		virtual int waitForSignal (int in_sig);
		virtual void cancelCommands ();
		virtual void beforeExecuting ();

		virtual int getStartPos ();
		virtual int getLen ();

		virtual int idleCall ();
};

/**
 * Will wait for signal, after signal we will end execution of that block.
 */
class Rts2SEBSignalEnd:public Rts2ScriptElementBlock
{
	private:
		// sig_num will become -1, when we get signal..
		int sig_num;
	protected:
		virtual bool endLoop ()
		{
			return (sig_num == -1);
		}
	public:
		Rts2SEBSignalEnd (Rts2Script * in_script, int end_sig_num);
		virtual ~ Rts2SEBSignalEnd (void);

		virtual int waitForSignal (int in_sig);
};

/**
 * Will test if target is acquired. Can be acompained with else block, that
 * will be executed when
 */
class Rts2SEBAcquired:public Rts2ScriptElementBlock
{
	private:
		Rts2ScriptElementBlock * elseBlock;
		int tar_id;
		enum
		{ NOT_CALLED, ACQ_OK, ACQ_FAILED }
		state;
		void checkState ();
	protected:
		virtual bool endLoop ();
	public:
		Rts2SEBAcquired (Rts2Script * in_script, int in_tar_id);
		virtual ~ Rts2SEBAcquired (void);

		virtual void postEvent (Rts2Event * event);

		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientCamera * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int nextCommand (Rts2DevClientPhot * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);

		virtual int processImage (Rts2Image * image);
		virtual int waitForSignal (int in_sig);
		virtual void cancelCommands ();

		virtual void addElseElement (Rts2ScriptElement * element);
};

/**
 * Else block for storing else path..
 */
class Rts2SEBElse:public Rts2ScriptElementBlock
{
	protected:
		virtual bool endLoop ();
	public:
		Rts2SEBElse (Rts2Script * in_script):Rts2ScriptElementBlock (in_script)
		{
		}
};

class Rts2SEBFor:public Rts2ScriptElementBlock
{
	private:
		int max;
	protected:
		virtual bool endLoop ()
		{
			return getLoopCount () >= max;
		}
	public:
		Rts2SEBFor (Rts2Script * in_script,
			int in_max):Rts2ScriptElementBlock (in_script)
		{
			max = in_max;
		}
};
#endif							 /* !__RTS2_SCRIPT_BLOCK__ */
