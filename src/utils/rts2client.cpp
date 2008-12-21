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

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <fstream>

#include "rts2block.h"
#include "rts2centralstate.h"
#include "rts2client.h"
#include "rts2devclient.h"
#include "rts2logstream.h"

#include <config.h>

Rts2ConnClient::Rts2ConnClient (Rts2Block * in_master, int _centrald_num, char *_name):
Rts2Conn (in_master)
{
	setName (_centrald_num, _name);
	setConnState (CONN_RESOLVING_DEVICE);
	address = NULL;
}


Rts2ConnClient::~Rts2ConnClient (void)
{
}


int
Rts2ConnClient::init ()
{
	int ret;
	struct addrinfo *device_addr;
	if (!address)
		return -1;

	ret = address->getSockaddr (&device_addr);

	if (ret)
	{
		logStream (MESSAGE_ERROR) << "Rts2Address::getAddress getaddrinfor: " <<
			strerror (errno) << sendLog;
		return ret;
	}
	sock = socket (device_addr->ai_family, device_addr->ai_socktype,
		device_addr->ai_protocol);
	if (sock == -1)
	{
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret == -1)
		return -1;
	
	ret = connect (sock, device_addr->ai_addr, device_addr->ai_addrlen);
	freeaddrinfo (device_addr);
	if (ret == -1)
	{
		if (errno == EINPROGRESS)
		{
			setConnState (CONN_INPROGRESS);
			return 0;
		}
		return -1;
	}
	connConnected ();
	return 0;
}


void
Rts2ConnClient::setAddress (Rts2Address * in_addr)
{
	setConnState (CONN_CONNECTING);
	address = in_addr;
	setOtherType (address->getType ());
	init ();
}


void
Rts2ConnClient::connConnected ()
{
 	Rts2Conn::connConnected ();
	master->getSingleCentralConn ()->queCommand (new Rts2CommandKey (master, getName ()));
	setConnState (CONN_AUTH_PENDING);
}


void
Rts2ConnClient::setKey (int in_key)
{
	Rts2Conn::setKey (in_key);
	if (isConnState (CONN_AUTH_PENDING))
	{
		// que to begining, send command
		// kill all runinng commands
		queSend (new Rts2CommandSendKey (master, master->getSingleCentralConn ()->getCentraldId (), getCentraldNum (), in_key));
	}
}


Rts2ConnClient *
Rts2Client::createClientConnection (int _centrald_num, char *_deviceName)
{
	return new Rts2ConnClient (this, _centrald_num, _deviceName);
}


Rts2Conn *
Rts2Client::createClientConnection (Rts2Address * in_addr)
{
	Rts2ConnClient *conn;
	conn = createClientConnection (in_addr->getCentraldNum (), in_addr->getName ());
	conn->setAddress (in_addr);
	return conn;
}


int
Rts2Client::willConnect (Rts2Address * in_addr)
{
	return 1;
}


Rts2Client::Rts2Client (int in_argc, char **in_argv):Rts2Block (in_argc,
in_argv)
{
	central_host = "localhost";
	central_port = CENTRALD_PORT;

	login = "petr";
	password = "petr";

	addOption (OPT_SERVER, "server", 1,
		"hostname of central server; default to localhost");
	addOption (OPT_PORT, "port", 1, "port of centrald server");
}


Rts2Client::~Rts2Client (void)
{
}


int
Rts2Client::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_SERVER:
			central_host = optarg;
			break;
		case OPT_PORT:
			central_port = optarg;
			break;
		default:
			return Rts2Block::processOption (in_opt);
	}
	return 0;
}


Rts2ConnCentraldClient *
Rts2Client::createCentralConn ()
{
	return new Rts2ConnCentraldClient (this, getCentralLogin (),
		getCentralPassword (), getCentralHost (),
		getCentralPort ());
}


int
Rts2Client::init ()
{
	int ret;

	ret = Rts2Block::init ();
	if (ret)
	{
		std::cerr << "Cannot init application, exiting." << std::endl;
		return ret;
	}

	Rts2ConnCentraldClient *central_conn = createCentralConn ();

	ret = 1;
	while (!getEndLoop ())
	{
		ret = central_conn->init ();
		if (!ret || getEndLoop ())
			break;
		std::cerr << "Trying to contact centrald\n";
		sleep (10);
	}
	addCentraldConnection (central_conn, true);
	return 0;
}


int
Rts2Client::run ()
{
	int ret;
	ret = init ();
	if (ret)
	{
		std::cerr << "Cannot init client, exiting." << std::endl;
		return ret;
	}
	while (!getEndLoop ())
	{
		oneRunLoop ();
	}
	return 0;
}


std::string Rts2Client::getMasterStateString ()
{
	return Rts2CentralState (getMasterStateFull ()).getString ();
}


Rts2ConnCentraldClient::Rts2ConnCentraldClient (Rts2Block * in_master, const char *in_login, const char *in_password, const char *in_master_host, const char *in_master_port):
Rts2Conn (in_master)
{
	master_host = in_master_host;
	master_port = in_master_port;

	login = in_login;
	password = in_password;

	setOtherType (DEVICE_TYPE_SERVERD);
}


int
Rts2ConnCentraldClient::init ()
{
	int ret;
	struct addrinfo hints;
	struct addrinfo *master_addr;

	hints.ai_flags = 0;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	ret = getaddrinfo (master_host, master_port, &hints, &master_addr);
	if (ret)
	{
		std::cerr << gai_strerror (ret) << "\n";
		return ret;
	}
	sock =
		socket (master_addr->ai_family, master_addr->ai_socktype,
		master_addr->ai_protocol);
	if (sock == -1)
	{
		return -1;
	}
	ret = connect (sock, master_addr->ai_addr, master_addr->ai_addrlen);
	freeaddrinfo (master_addr);
	if (ret == -1)
		return -1;
	setConnState (CONN_CONNECTED);

	queCommand (new Rts2CommandLogin (master, login, password));
	return 0;
}


int
Rts2ConnCentraldClient::command ()
{
	// centrald-specific command processing
	if (isCommand ("logged_as"))
	{
		int p_centrald_id;
		if (paramNextInteger (&p_centrald_id) || !paramEnd ())
			return -2;
		setCentraldId (p_centrald_id);
		master->centraldConnRunning (this);
		setCommandInProgress (false);
		return -1;
	}
	if (isCommand ("authorization_key"))
	{
		char *p_device_name;
		int p_key;
		Rts2Conn *conn;
		if (paramNextString (&p_device_name)
			|| paramNextInteger (&p_key) || !paramEnd ())
			return -2;
		conn = master->getConnection (p_device_name);
		if (conn)
			conn->setKey (p_key);
		setCommandInProgress (false);
		return -1;
	}
	return Rts2Conn::command ();
}


void
Rts2ConnCentraldClient::setState (int in_value)
{
	Rts2Conn::setState (in_value);
	master->setMasterState (in_value);
}


Rts2CommandLogin::Rts2CommandLogin (Rts2Block * in_master, const char *in_login, const char *in_password):Rts2Command
(in_master)
{
	char * command;
	login = in_login;
	password = in_password;
	asprintf (&command, "login %s", login);
	setCommand (command);
	free (command);
	state = LOGIN_SEND;
}


int
Rts2CommandLogin::commandReturnOK (Rts2Conn * conn)
{
	char *command;
	switch (state)
	{
		case LOGIN_SEND:
			asprintf (&command, "password %s", password);
			setCommand (command);
			free (command);
			state = PASSWORD_SEND;
			// reque..
			return RTS2_COMMAND_REQUE;
		case PASSWORD_SEND:
			setCommand ("info");
			state = INFO_SEND;
			return RTS2_COMMAND_REQUE;
		case INFO_SEND:
			return -1;
	}
	return 0;
}
