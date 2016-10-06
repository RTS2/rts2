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

#include "rts2script/script.h"

#include "object.h"
#include "block.h"
#include "rts2target.h"

namespace rts2script
{

/**
 * That class provide scripting interface for devices, so they can
 * run scrips..
 */
class DevScript
{

	public:
		DevScript (rts2core::Connection * in_script_connection);
		virtual ~ DevScript (void);
		void postEvent (rts2core::Event * event);
		virtual void nextCommand (rts2core::Command *triggerCommand = NULL) = 0;

		void setScript (ScriptPtr _script) { script = _script; }

		ScriptPtr& getScript () { return script; }

		/**
		 * Called when device state changes. Propagates error to the script.
		 */
		void stateChanged (rts2core::ServerState *state);

	protected:
		
		/**
		 ( Return true if there is still script to execute.
		 */
		bool haveScript () { return script.get (); }

		Rts2Target * currentTarget;
		Rts2Target * killTarget;

		/**
		 * Reference to next command.
		 */
		rts2core::Command *nextComd;

		/**
		 * Reference to next command connection(s), if it differs from scripting device.
		 */
		connections_t cmdConns;
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
		int haveNextCommand (rts2core::DevClient *devClient);
		virtual void unblockWait () = 0;
		virtual void unsetWait () = 0;
		virtual void clearWait () = 0;
		virtual int isWaitMove () = 0;
		virtual void setWaitMove () = 0;
		virtual int queCommandFromScript (rts2core::Command * comm) = 0;

		virtual int getFailedCount () = 0;
		virtual void clearFailedCount () = 0;
		void idle ();

		virtual void deleteScript ();

		rts2core::Command *scriptKillCommand;
		bool scriptKillcallScriptEnds;

	private:
		Rts2Target *nextTarget;

		rts2core::Connection * script_connection;
		ScriptPtr script;
		int dont_execute_for;
		int dont_execute_for_obsid;
		int scriptLoopCount;
		int scriptCount;
		int lastTargetObsID;
		void setNextTarget (Rts2Target * in_target);

		void scriptBegin ();
};

}
#endif							 /* !__RTS2_DEVSCRIPT__ */
