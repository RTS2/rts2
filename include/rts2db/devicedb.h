/*
 * Device with database connection.
 * Copyright (C) 2004-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DEVICEDB__
#define __RTS2_DEVICEDB__

#include "device.h"
#include "configuration.h"

#include "camlist.h"

namespace rts2db
{

/**
 * Adds database connectivity to device class.
 * Provides parameters to specify database location,
 * and routines to parse database location from .ini file.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class DeviceDb:public rts2core::Device
{
	public:
		DeviceDb (int argc, char **argv, int in_device_type, const char *default_name);
		virtual ~ DeviceDb (void);

		virtual void postEvent (rts2core::Event *event);

		CamList cameras;

		/**
		 * Create database connection.
		 *
		 * @param conn_name   connection name
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		int initDB (const char *conn_name);

	protected:
		virtual int willConnect (rts2core::NetworkAddress * in_addr);
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();

		virtual int init ();
		virtual void forkedInstance ();

		virtual void signaledHUP ();

		rts2core::Configuration *config;

		bool emptyConnectString () { return connectString != NULL && strlen(connectString) == 0; }

	private:
		char *connectString;
		char *configFile;
};

/**
 * Global function to connect to database that keeps the parameters (in a
 * thread-local way) between invocations so that they may be used outside of
 * DeviceDb sub-classes for re-establishing a connection with exactly the same
 * parameters as initially.
 */
int connectToDb (char *connectionString, const char *connectionName);

/**
 * Global function to test whether current connection to the database is alive,
 * and re-establish it if possible.
 *
 * @return 0 on success (either alive connection or successfully re-established
 * one), and -1 if the connection can't be re-established.
 */
int checkDbConnection ();

}
#endif							 /* !__RTS2_DEVICEDB__ */
