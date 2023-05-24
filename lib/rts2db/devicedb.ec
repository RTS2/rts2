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

#include "rts2db/devicedb.h"
#include "configuration.h"

#include <pwd.h>

#define OPT_DEBUGDB    OPT_LOCAL + 201

using namespace rts2db;

EXEC SQL include sqlca;

int DeviceDb::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () < getDeviceType () || (in_addr->getType () == getDeviceType () && strcmp (in_addr->getName (), getDeviceName ()) < 0))
		return 1;
	return 0;
}

DeviceDb::DeviceDb (int argc, char **argv, int in_device_type, const char *default_name):rts2core::Device (argc, argv, in_device_type, default_name)
{
	connectString = NULL;		 // default DB
	configFile = NULL;
	config = NULL;

	addOption (OPT_DATABASE, "database", 1, "connect string to PSQL database (default to stars)");
	addOption (OPT_CONFIG, "config", 1, "configuration file");
	addOption (OPT_DEBUGDB, "debugdb", 0, "print database debugging messages");
}

DeviceDb::~DeviceDb (void)
{
	EXEC SQL DISCONNECT;
	if (connectString)
		delete[] connectString;
}

void DeviceDb::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
	}
	rts2core::Device::postEvent (event);
}

int DeviceDb::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_DATABASE:
			connectString = new char[strlen (optarg) + 1];
			strcpy (connectString, optarg);
			break;
		case OPT_CONFIG:
			configFile = optarg;
			break;
		case OPT_DEBUGDB:
			ECPGdebug (1, stderr);
			break;
		default:
			return rts2core::Device::processOption (in_opt);
	}
	return 0;
}

int DeviceDb::reloadConfig ()
{
	// load config..
	return config->loadFile (configFile);
}

int DeviceDb::initDB (const char *conn_name)
{
	if (config == NULL)
	{
		config = rts2core::Configuration::instance ();
		int ret = reloadConfig ();

		if (ret)
			return ret;
	}

	if (connectString && strlen(connectString) == 0)
	{
		logStream (MESSAGE_WARNING) << "starting without DB" << sendLog;
		return 0;
	}

	if (rts2db::connectToDb (connectString, conn_name))
		return -1;

	cameras.load ();

	return 0;
}

// Static thread-local variables to keep DB connection parameters
// They will be used if connectToDb() is called with NULL arguments
__thread char *pConnectString = NULL;
__thread char *pConnName = NULL;

int rts2db::connectToDb (char *connectString, const char *conn_name)
{
	if (!connectString)
		connectString = pConnectString;
	else
		pConnectString = strdup (connectString);

	if (!conn_name)
		conn_name = pConnName;
	else
		pConnName = strdup (conn_name);

	std::string cs;
	EXEC SQL BEGIN DECLARE SECTION;
	const char *c_db;
	const char *c_username;
	const char *c_password;
	const char *c_connection = conn_name;
	EXEC SQL END DECLARE SECTION;

	rts2core::Configuration *config = rts2core::Configuration::instance ();

	if (connectString)
	{
		c_db = connectString;
	}
	else
	{
		config->getString ("database", "name", cs);
		c_db = cs.c_str ();
	}

	std::string db_username;
	std::string db_password;

	if (config->getString ("database", "username", db_username, "") == 0)
	{
		c_username = db_username.c_str ();
		if (config->getString ("database", "password", db_password) == 0)
		{
			c_password = db_password.c_str ();
			EXEC SQL CONNECT TO :c_db AS :c_connection USER  :c_username USING :c_password;
			if (sqlca.sqlcode != 0)
			{
				logStream (MESSAGE_ERROR) << "cannot connect to DB '" << c_db
					<< "' with user '" << c_username
					<< "' and password xxxx (see rts2.ini) :"
					<< sqlca.sqlerrm.sqlerrmc << sendLog;
				return -1;
			}
		}
		else
		{
			EXEC SQL CONNECT TO :c_db AS :c_connection USER  :c_username;
			if (sqlca.sqlcode != 0)
			{
				logStream (MESSAGE_ERROR) << "cannot connect to DB '" << c_db
					<< "' with user '" << c_username
					<< "': " << sqlca.sqlerrm.sqlerrmc << sendLog;
				return -1;
			}
		}
	}
	else
	{
		EXEC SQL CONNECT TO :c_db AS :c_connection;
		if (sqlca.sqlcode != 0)
		{
			struct passwd *up = getpwuid (geteuid ());
			logStream (MESSAGE_ERROR) << "cannot connect to DB '" << c_db << "'. Please check if the database server is running (on specified port, or on port 5432, which is the default one; please be aware that RTS2 does not parse PostgreSQL configuration, so if the database is running on the non-default port, it will not be accessible unless you specify the port). Also please make sure that the current user, " << up->pw_name << "(" << up->pw_uid << ") can log into database: " << sqlca.sqlerrm.sqlerrmc << sendLog;
			return -1;
		}
	}

	return 0;
}

int rts2db::checkDbConnection ()
{
	// Execute simplest SQL query to check whether we are connected to DB
	EXEC SQL BEGIN DECLARE SECTION;
	int tmp;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL SELECT 0 INTO :tmp;

	// Check for error, and re-connect if any
	if (sqlca.sqlcode)
	{
		logStream (MESSAGE_ERROR) << "DB error: " << sqlca.sqlerrm.sqlerrmc << " (" << sqlca.sqlcode << ")" << sendLog;

		if (sqlca.sqlcode == ECPG_PGSQL || sqlca.sqlcode == ECPG_NO_CONN)
		{
			EXEC SQL DISCONNECT;

			if (connectToDb (NULL, NULL))
				return -1;
			else
				logStream (MESSAGE_INFO) << "Successfully re-connected to DB" << sendLog;
		}
	}

	return 0;
}

int DeviceDb::init ()
{
	int ret;

	ret = rts2core::Device::init ();
	if (ret)
		return ret;

	// load config.
	return initDB ("master");
}

void DeviceDb::forkedInstance ()
{
	// dosn't work??
	//  EXEC SQL DISCONNECT;
	rts2core::Device::forkedInstance ();
}

void DeviceDb::signaledHUP ()
{
	reloadConfig();
}
