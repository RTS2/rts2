/* 
 * Conteiner holding multiple daemon.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics AS CR  <kubanek@fzu.cz>
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

#ifndef __RTS2_MULTIDEV__
#define __RTS2_MULTIDEV__

#include "device.h"

#include <list>

namespace rts2core
{

class MultiDev: public std::list < Device* >
{
	public:
		void initMultidev (int debug = 0);
		virtual int run (int debug = 0);
		void multiLoop ();
		void runLoop (float tmout);
};

/**
 * Master class for multidevs. This is used if two or more device shares single
 * communication channel (serial link,..). Device classes are added with
 * Daemon::addDevice method. Methods in added devices are then called as need
 * to allow devices to control devices and gets events and data from network
 * sockets.
 */
class MultiBase:public Daemon
{
	public:
		MultiBase (int argc, char **argv, const char *default_name);

		/**
		 * Adds device to controlled devices.
		 *
		 * @param dev device instance
		 */
		void addDevice (Device *dev);
		virtual int init ();
		virtual int run ();
    		virtual void initLockFile () { setLockFile (std::string (getLockPrefix ()) + std::string (multi_name)); }

	protected:
		virtual int processOption (int opt);
		virtual bool isRunning (rts2core::Connection *conn) { return false; }
		virtual rts2core::Connection *createClientConnection (rts2core::NetworkAddress * in_addr) { return NULL; }

	private:
		rts2core::MultiDev md;
		const char *multi_name;
};

}

#endif							 /* !__RTS2_MULTIDEV__ */
