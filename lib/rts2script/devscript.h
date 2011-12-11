/*
 * Client for script devices.
 * Copyright (C) 2004-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DEVSCRIPT__
#define __RTS2_DEVSCRIPT__

#include "script.h"

#include "rts2object.h"
#include "block.h"
#include "../rts2/rts2target.h"

namespace rts2script
{

/**
 * That class provide scripting interface for devices, so they can
 * run scrips..
 */
class DevScript
{
	private:
		Rts2Conn * script_connection;
		ScriptPtr script;
		Rts2Target *nextTarget;
		int dont_execute_for;
		int dont_execute_for_obsid;
		int scriptLoopCount;
		int scriptCount;
		int lastTargetObsID;
		void setNextTarget (Rts2Target * in_target);

		void scriptBegin ();
	protected:
		Rts2Target * currentTarget;
		/**
		 * Reference to next command.
		 */
		Rts2Command *nextComd;

		/**
		 * Reference to next command connection, if it differs from scripting device.
		 */
		Rts2Conn *cmdConn;
		char cmd_device[DEVICE_NAME_SIZE];

		enum
		{
			NO_WAIT, WAIT_SLAVE, WAIT_MASTER, WAIT_SIGNAL, WAIT_MIRROR,
			WAIT_SEARCH
		} waitScript;

		/**
		 * Start new target.
		 *
		 * @param callScriptEnds  if true, script_ends command will be called before the script starts
		 */
		virtual void startTarget (bool callScriptEnds = true);

		virtual int getNextCommand () = 0;
		int nextPreparedCommand ();

		/**
		 * Check if script execution can be ended. Return true if device
		 * is in proper state to allow next script to take over.
		 *
		 * @return true if next script excecution can started, false if next script execution must wait for device to reach other state
		 */
		virtual bool canEndScript () { return true; }

		/**
		 * Entry point for script execution.
		 *
		 * That's entry point to script execution. It's called when device
		 * is free to do new job - e.g when camera finish exposure (as we
		 * can move filter wheel during chip readout).
		 *
		 * It can be called more then once for one command - hence we keep
		 * in nextComd prepared next command, and return it from
		 * nextPreparedCommand when it's wise to return it.
		 *
		 * @return 0 when there isn't any next command to execute, 1 when
		 * there is next command available.
		 */
		int haveNextCommand (Rts2DevClient *devClient);
		virtual void unblockWait () = 0;
		virtual void unsetWait () = 0;
		virtual void clearWait () = 0;
		virtual int isWaitMove () = 0;
		virtual void setWaitMove () = 0;
		virtual void queCommandFromScript (Rts2Command * comm) = 0;

		virtual int getFailedCount () = 0;
		virtual void clearFailedCount () = 0;
		void idle ();

		virtual void deleteScript ();

	public:
		DevScript (Rts2Conn * in_script_connection);
		virtual ~ DevScript (void);
		void postEvent (Rts2Event * event);
		virtual void nextCommand () = 0;

		void setScript (ScriptPtr _script) { script = _script; }

		ScriptPtr& getScript () { return script; }
};

}
#endif							 /* !__RTS2_DEVSCRIPT__ */
