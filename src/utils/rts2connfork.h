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

#include "rts2connnosend.h"

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
class Rts2ConnFork:public Rts2ConnNoSend
{
	private:
		pid_t childPid;
		time_t forkedTimeout;
		// error socket
		int sockerr;

	protected:
		char *exePath;
		virtual void connectionError (int last_data_size);
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
	public:
		Rts2ConnFork (Rts2Block * in_master, const char *in_exe, int in_timeout =
			0);
		virtual ~ Rts2ConnFork (void);

		virtual int add (fd_set * readset, fd_set * writeset, fd_set * expset);

		virtual int receive (fd_set * readset);

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
		void term ();

		virtual int idle ();

		virtual void childReturned (pid_t in_child_pid);

		virtual void childEnd ()
		{
			// insert here some post-processing
		}
};
#endif							 /* !__RTS2_CONN_FORK__ */
