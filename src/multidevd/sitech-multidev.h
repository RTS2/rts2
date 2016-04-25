/**
 * Abstract class for Sitech multidev
 * Copyright (C) 2016 Petr Kubanek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RTS2_SITECH_MULTIDEV__
#define __RTS2_SITECH_MULTIDEV__

#include "multidev.h"
#include "connection/sitech.h"

/**
 * Abstract class for all Sitech multidev devices. Provides abstract method to assing Sitech connection.
 */
class SitechMultidev
{
	public:
		SitechMultidev ()
		{
			sitech = NULL;
		}

		void setSitechConnection (rts2core::ConnSitech *conn)
		{
			sitech = conn;
		}

		rts2core::ConnSitech *sitech;
		rts2core::SitechAxisStatus axisStatus;
		rts2core::SitechXAxisRequest requestX;
		rts2core::SitechYAxisRequest requestY;
};

class SitechMultiBase:public rts2core::Daemon
{
	public:
		SitechMultiBase (int argc, char **argv);
		virtual ~SitechMultiBase ();

		virtual int run ();

	protected:
		virtual int processOption (int opt);

		virtual bool isRunning (rts2core::Connection *conn) { return false; }
		virtual rts2core::Connection *createClientConnection (rts2core::NetworkAddress * in_addr) { return NULL; }

		virtual int initHardware ();
	private:
		const char *focName;
		const char *mirrorName;

		const char *sitechDev;
		rts2core::ConnSitech *sitechConn;

		rts2core::MultiDev md;
};

#endif //!__RTS2_SITECH_MULTIDEV__
