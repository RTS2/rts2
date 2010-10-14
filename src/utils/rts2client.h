/* 
 * Supporting classes for clients programs.
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

#ifndef __RTS2_CLIENT__
#define __RTS2_CLIENT__

#include "rts2block.h"
#include "rts2command.h"
#include "rts2devclient.h"

/**
 * @defgroup RTS2Client
 *
 * This module includes infrastructure needed for client applications.
 */

/**
 * Represents connection between device and client.
 *
 * This class is used in Rts2Client to represent connections
 * which client establish to other RTS2 components.
 *
 * @ingroup RTS2Client
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConnClient:public Rts2Conn
{
	private:
		Rts2Address * address;

	protected:
		virtual void connConnected ();
	public:
		Rts2ConnClient (Rts2Block *_master, int _centrald_num, char *_name);
		virtual ~ Rts2ConnClient (void);

		virtual int init ();

		virtual void setAddress (Rts2Address * in_addr);

		/**
		 * Set client key.
		 *
		 * Keys are distributed from Rts2Centrald to client connection.
		 *
		 * @param in_key New client key.
		 */
		virtual void setKey (int in_key);
};

/**
 * Represents connection between centrald and client.
 *
 * This class is used to represent conection between central
 * server and client. It is used to handle key management etc.
 *
 * @ingroup RTS2Client
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConnCentraldClient:public Rts2Conn
{
	private:
		const char *master_host;
		const char *master_port;

		const char *login;
		const char *password;
	protected:
		virtual void setState (int in_value, char * msg);

	public:
		Rts2ConnCentraldClient (Rts2Block * in_master, const char *in_login, const char *in_password, const char *in_master_host, const char *in_master_port);
		virtual int init ();

		virtual int command ();
};

namespace rts2core
{

class Rts2CommandLogin:public Rts2Command
{
	private:
		const char *login;
		const char *password;
	private:
		enum
		{ LOGIN_SEND, PASSWORD_SEND, INFO_SEND }
		state;
	public:
		Rts2CommandLogin (Rts2Block * master, const char *in_login,
			const char *in_password);
		virtual int commandReturnOK (Rts2Conn * conn);
};

}

/**
 * Common class for client aplication.
 *
 * Connect to centrald, get names of all devices.
 * Works similary to Rts2Device.
 *
 * @ingroup RTS2Block RTS2Client
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Client:public Rts2Block
{
	public:
		Rts2Client (int in_argc, char **in_argv);
		virtual ~ Rts2Client (void);

		virtual int run ();

	protected:
		virtual Rts2ConnClient * createClientConnection (int _centrald_num, char *_deviceName);
		virtual Rts2Conn *createClientConnection (Rts2Address * in_addr);
		virtual int willConnect (Rts2Address * in_addr);

		virtual int processOption (int in_opt);
		virtual int init ();

		virtual Rts2ConnCentraldClient *createCentralConn ();

		const char *getCentralLogin ()
		{
			return login;
		}
		const char *getCentralPassword ()
		{
			return password;
		}
		const char *getCentralHost ()
		{
			return central_host;
		}
		const char *getCentralPort ()
		{
			return central_port;
		}

	private:
		const char *central_host;
		const char *central_port;
		const char *login;
		const char *password;
};
#endif							 /* ! __RTS2_CLIENT__ */
