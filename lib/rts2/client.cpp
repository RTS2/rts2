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

#include "block.h"
#include "client.h"
#include "devclient.h"
#include "logstream.h"

#include <rts2-config.h>

using namespace rts2core;

ConnClient::ConnClient (Block * in_master, int _centrald_num, char *_name):Connection (in_master)
{
	setName (_centrald_num, _name);
	setConnState (CONN_RESOLVING_DEVICE);
	address = NULL;
}

ConnClient::~ConnClient (void)
{
}

int ConnClient::init ()
{
	int ret;
	struct addrinfo *device_addr;
	if (!address)
		return -1;

	ret = address->getSockaddr (&device_addr);

	if (ret)
	{
		logStream (MESSAGE_ERROR) << "NetworkAddress::getAddress getaddrinfor for host " << address->getHost ()
			<< ": " << gai_strerror (ret) << sendLog;
		return -1;
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

void ConnClient::setAddress (NetworkAddress * in_addr)
{
	setConnState (CONN_CONNECTING);
	address = in_addr;
	setOtherType (address->getType ());
	init ();
}

void ConnClient::connConnected ()
{
 	Connection::connConnected ();
	master->getSingleCentralConn ()->queCommand (new CommandKey (master, getName ()));
	setConnState (CONN_AUTH_PENDING);
}

void ConnClient::setKey (int in_key)
{
	Connection::setKey (in_key);
	if (isConnState (CONN_AUTH_PENDING))
	{
		// que to begining, send command
		// kill all runinng commands
		queSend (new CommandSendKey (master, master->getSingleCentralConn ()->getCentraldId (), getCentraldNum (), in_key));
	}
}

ConnClient * Client::createClientConnection (int _centrald_num, char *_deviceName)
{
	return new ConnClient (this, _centrald_num, _deviceName);
}

Connection * Client::createClientConnection (NetworkAddress * in_addr)
{
	ConnClient *conn;
	conn = createClientConnection (in_addr->getCentraldNum (), in_addr->getName ());
	conn->setAddress (in_addr);
	return conn;
}

int Client::willConnect (NetworkAddress * in_addr)
{
	return 1;
}

Client::Client (int in_argc, char **in_argv, const char *_name):Block (in_argc, in_argv)
{
	central_host = "localhost";
	central_port = RTS2_CENTRALD_PORT;

	//login = "petr";
	//password = "petr";
	login = getlogin();
	password = login;

	name = _name;

	addOption (OPT_PORT, "port", 1, "port of centrald server");
	addOption (OPT_SERVER, "server", 1, "hostname of central server; default to localhost");
}

Client::~Client (void)
{
}

int Client::processOption (int in_opt)
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
			return Block::processOption (in_opt);
	}
	return 0;
}

ConnCentraldClient * Client::createCentralConn ()
{
	return new ConnCentraldClient (this, getCentralLogin (), name, getCentralPassword (), getCentralHost (), getCentralPort ());
}

int Client::init ()
{
	int ret;

	ret = Block::init ();
	if (ret)
	{
		std::cerr << "Cannot init application, exiting." << std::endl;
		return ret;
	}

	ConnCentraldClient *central_conn = createCentralConn ();

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

int Client::run ()
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

ConnCentraldClient::ConnCentraldClient (Block * in_master, const char *in_login, const char *in_name, const char *in_password, const char *in_master_host, const char *in_master_port):Connection (in_master)
{
	master_host = in_master_host;
	master_port = in_master_port;

	login = in_login;
	setName (-1, in_name);
	password = in_password;

	setOtherType (DEVICE_TYPE_SERVERD);
}

int ConnCentraldClient::init ()
{
	int ret;
	struct addrinfo hints = {0};
	struct addrinfo *master_addr;

	hints.ai_flags = 0;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	ret = getaddrinfo (master_host, master_port, &hints, &master_addr);
	if (ret)
	{
		std::cerr << gai_strerror (ret) << " for " << master_host << ":" << master_port << std::endl;
		return ret;
	}
	sock = socket (master_addr->ai_family, master_addr->ai_socktype, master_addr->ai_protocol);
	if (sock == -1)
	{
		return -1;
	}
	ret = connect (sock, master_addr->ai_addr, master_addr->ai_addrlen);
	freeaddrinfo (master_addr);
	if (ret == -1)
		return -1;
	setConnState (CONN_CONNECTED);

	queCommand (new CommandLogin (master, login, getName (), password));
	return 0;
}

int ConnCentraldClient::command ()
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
		Connection *conn;
		if (paramNextString (&p_device_name)
			|| paramNextInteger (&p_key) || !paramEnd ())
			return -2;
		conn = master->getConnection (p_device_name);
		if (conn)
			conn->setKey (p_key);
		setCommandInProgress (false);
		return -1;
	}
	return Connection::command ();
}

void ConnCentraldClient::setState (rts2_status_t in_value, char * msg)
{
	Connection::setState (in_value, msg);
	master->setMasterState (this, in_value);
}

CommandLogin::CommandLogin (Block * in_master, const char *in_login, const char *name, const char *in_password):Command (in_master)
{
	std::ostringstream _os;
	login = in_login;
	password = in_password;
	_os << "login " << login << " " << name;
	setCommand (_os);
	state = LOGIN_SEND;
}

int CommandLogin::commandReturnOK (Connection * conn)
{
	std::ostringstream _os;
	switch (state)
	{
		case LOGIN_SEND:
			_os << "password " <<  password;
			setCommand (_os);
			state = PASSWORD_SEND;
			// reque..
			return RTS2_COMMAND_REQUE;
		case PASSWORD_SEND:
			setCommand (COMMAND_INFO);
			state = INFO_SEND;
			return RTS2_COMMAND_REQUE;
		case INFO_SEND:
			return -1;
	}
	return 0;
}
