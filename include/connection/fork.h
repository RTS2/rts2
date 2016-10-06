/* 
 * Forking connection handling.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONN_FORK__
#define __RTS2_CONN_FORK__

#include "connnosend.h"

namespace rts2core
{

/**
 * Class representing forked connection, used to controll process which runs in
 * paraller with current execution line.
 *
 * This object is ussually created and added to connections list. It's standard
 * write and read methods are called when some data become available from the
 * pipe to which new process writes standard output.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2Block
 */
class ConnFork:public ConnNoSend
{
	public:
		ConnFork (rts2core::Block * _master, const char *_exe, bool _fillConnEnvVars, bool _openin, int _timeout = 0);
		virtual ~ConnFork ();


		/**
		 * Add argument to command which will be executed.
		 *
		 * @param arg Argument which will be added.
		 */
		template < typename T > void addArg (T arg)
		{
			std::ostringstream _os;
			_os << arg;
			argv.push_back (_os.str ());
		}

		int writeToProcess (const char *msg);
		int writeToProcessInt (int msg);

		/**
		 * This method will log new lines arriving if debug is turned on.
		 */
		virtual void processLine ();

		void setInput (std::string _input) { input = _input; }

		virtual int add (Block *block);

		virtual int receive (Block *block);

		virtual int writable (Block *block);

		/**
		 * Create and execute processing.
		 *
		 * BIG FAT WARNINIG: This method is called after sucessfull
		 * fork call. Due to the fact that fork keeps all sockets from
		 * parent process opened, you may not use any DB call in
		 * newProcess or call any call which operates with DB (executes
		 * ECPG statements).
		 */
		virtual int newProcess ();

		virtual int init ();

		virtual void stop ();
		void terminate ();

		virtual int idle ();

		virtual void childReturned (pid_t in_child_pid);

		/**
		 * Called after script execution ends.
		 */
		virtual void childEnd () {}

		const char *getExePath () { return exePath; }

		/**
		 * Set debug flag. If set to true, all communication with executable will be logged.
		 */
		void setConnectionDebug (bool _connDebug = true) { connDebug = _connDebug; }

	protected:
		char *exePath;
		virtual void connectionError (int last_data_size);

		/**
		 * Called before fork command, This is the spot to possibly check
		 * and fill in execution filename.
		 */
		virtual void beforeFork ();

		/**
		 * Called when initialization of the connection fails at some point.
		 */
		virtual void initFailed ();

		/**
		 * Called with error line input. Default processing is to log it as MESSAGE_ERROR.
		 *
		 * @param errbuf  Buffer containing null terminated error line string.
		 */
		virtual void processErrorLine (char *errbuf);

	private:
		pid_t childPid;
		std::vector <std::string> argv;
		int forkedTimeout;

		// for statistics, how much time was consumed
		time_t startTime;
		time_t endTime;

		// holds pipe with stderr. Stdout is stored in sock
		int sockerr;
		// holds write end - we can send input to this socket
		int sockwrite;
		bool fillConnEnvVars;

		bool connDebug;

		std::string input;

		void fillConnectionEnv (Connection *conn, const char *name);
};

}
#endif							 /* !__RTS2_CONN_FORK__ */
