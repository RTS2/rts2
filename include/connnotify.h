/* 
 * Copnnection for inotify events.
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

#ifndef __RTS2_CONNNOTIFY__
#define __RTS2_CONNNOTIFY__

#include "rts2-config.h"

#include "connnosend.h"

#ifdef RTS2_HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif

namespace rts2core
{

/**
 * Class for inotify file descriptors.
 *
 * Read events from descriptor created by inotify, and send them to master.
 * If ConnNotify is added to watched connections by calling addConnection,
 * fileModified will be called every time modification to file is noticed.
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ConnNotify:public ConnNoSend
{
	public:
		/**
		 * Create new notify connection.
		 *
		 * @param _master   Reference to master holding this connection.
		 */
		ConnNotify (rts2core::Block *_master);

		/**
		 * Init TCP/IP connection to host given at constructor.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int init ();

		/**
		 * Set debugging flag. When debugging is enabled, all data send
		 * to and from the socket will be logged using standard RTS2
		 * logging with MESSAGE_DEBUG type.
		 */
		void setDebug (bool _debug = true) { debug = _debug; }

		/**
		 * Called when select call indicates that socket holds new
		 * data for reading.
		 *
		 * @param readset  Read FD_SET, connection must test if socket is among this set.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int receive (fd_set * readset);

		bool getDebug () { return debug; }

#ifdef RTS2_HAVE_SYS_INOTIFY_H
		int addWatch (const char *filename, uint32_t mask = IN_MODIFY);
#else
		int addWatch (const char *filename, uint32_t mask = 0);
#endif

	private:
		const char *hostname;
		bool debug;
};

};

#endif // !__RTS2_CONNNOTIFY__
