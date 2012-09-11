/* 
 * Device basic class.
 * Copyright (C) 2003-2010 Petr Kubanek <petr@kubanek.net>
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

#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>

#include <rts2-config.h>
#include "status.h"
#include "device.h"
#include "command.h"

#define MINDATAPORT   5556
#define MAXDATAPORT   5656

using namespace rts2core;

int DevConnection::command ()
{
	int ret;
	// authorized and running
	if (master->requireAuthorization () == false || (getCentraldId () != -1 && isConnState (CONN_AUTH_OK)) || getType () == DEVICE_DEVICE)
	{
		ret = master->commandAuthorized (this);
		if (ret == DEVDEM_E_HW)
		{
			sendCommandEnd (DEVDEM_E_HW, "device error");
			return -1;
		}
		if (ret != -5)
			return ret;
	}
	if (isCommand ("auth"))
	{
		int auth_id;
		int centraldNum;
		int auth_key;
		if (paramNextInteger (&auth_id)
		  	|| paramNextInteger (&centraldNum)
			|| paramNextInteger (&auth_key)
			|| !paramEnd ())
			return -2;
		setCentraldId (auth_id);
		setKey (auth_key);
		ret = master->authorize (centraldNum, this);
		if (ret)
		{
			sendCommandEnd (DEVDEM_E_SYSTEM,
				"cannot authorize; try again later");
			setConnState (CONN_AUTH_FAILED);
			return -1;
		}
		setConnState (CONN_AUTH_PENDING);
		setCommandInProgress (false);
		return -1;
	}
	return Connection::command ();
}

DevConnection::DevConnection (int in_sock, Device * in_master):Connection (in_sock, in_master)
{
	address = NULL;
	master = in_master;
	if (in_sock > 0)
		setType (DEVICE_SERVER);
	else
		setType (DEVICE_DEVICE);
}

int DevConnection::init ()
{
	if (getType () != DEVICE_DEVICE)
		return Connection::init ();
	int ret;
	struct addrinfo *device_addr;
	if (!address)
		return -1;

	ret = address->getSockaddr (&device_addr);

	if (ret)
	{
		logStream (MESSAGE_ERROR) << "NetworkAddress::getAddress getaddrinfor for host " << address->getHost () << ":"
			<< gai_strerror (ret) << sendLog;
		return -1;
	}
	sock = socket (device_addr->ai_family,
		device_addr->ai_socktype, device_addr->ai_protocol);
	if (sock == -1)
	{
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret == -1)
	{
		return -1;
	}
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

int DevConnection::authorizationOK ()
{
	setConnState (CONN_AUTH_OK);
	master->baseInfo ();
	master->sendBaseInfo (this);
	try
	{
		master->info ();
	}
	catch (rts2core::Error &er)
	{
		// pass
	}
	master->sendInfo (this, true);
	master->sendFullStateInfo (this);
	master->resendProgress (this);
	sendCommandEnd (DEVDEM_OK, "OK authorized");
	return 0;
}

int DevConnection::authorizationFailed ()
{
	logStream (MESSAGE_DEBUG) << "authorization failed: " << getName ()
		<< getCentraldId () << " " << getCentraldNum ()
		<< sendLog;
	setCentraldId (-1);
	setConnState (CONN_DELETE);
	sendCommandEnd (DEVDEM_E_SYSTEM, "authorization failed");
	return 0;
}

void DevConnection::setDeviceAddress (NetworkAddress * in_addr)
{
	address = in_addr;
	setConnState (CONN_CONNECTING);
	setName (in_addr->getCentraldNum (), in_addr->getName ());
	init ();
	setOtherType (address->getType ());
}

void DevConnection::setDeviceName (int _centrald_num, char *_name)
{
	setName (_centrald_num, _name);
	// find centrald connection for us..
	setConnState (CONN_RESOLVING_DEVICE);
}

void DevConnection::connConnected ()
{
	Connection::connConnected ();
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "connConnected:  " << getName () << " state: " << getConnState () << sendLog;
	#endif
	if (address == NULL)
	{
		logStream (MESSAGE_ERROR) << "null address record in DevConnection::connConnected for device " << getName ()
			<< " centrald num " << getCentraldNum ()
			<< sendLog;
		return;
	}

	connections_t::iterator iter;
	for (iter = getMaster ()->getCentraldConns ()->begin (); iter != getMaster ()->getCentraldConns ()->end (); iter++)
	{
		if ((*iter)->getCentraldNum () == address->getHostNum ())
		{
			(*iter)-> queCommand (new CommandKey (getMaster (), getName ()));
			setConnState (CONN_AUTH_PENDING);
			return;
		}
	}
	logStream (MESSAGE_ERROR) << "Cannot find central server for authorization (name: " << getName ()
		<< " centrald num: " << getCentraldNum () << ")"
		<< " host num: " << address->getHostNum ()
		<< sendLog;
	return;
}

void DevConnection::setDeviceKey (int _centraldId, int _key)
{
	Connection::setKey (_key);
	if (getType () == DEVICE_DEVICE)
	{
		if (isConnState (CONN_AUTH_PENDING))
		{
			// que to begining, send command
			// kill all runinng commands

			queSend (new CommandSendKey (master, _centraldId, getCentraldNum (), _key));
			setCommandInProgress (false);
		}
		else
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) 
				<< "DevConnection::setKey invalid connection state: "
				<< getConnState () << sendLog;
			#endif
		}
	}
}

void DevConnection::setConnState (conn_state_t new_conn_state)
{
	if (getType () != DEVICE_DEVICE)
		return Connection::setConnState (new_conn_state);
	Connection::setConnState (new_conn_state);
	if (new_conn_state == CONN_AUTH_OK)
	{
	 	std::ostringstream _os;
		_os << "this_device " << master->getDeviceName ()
			<< " " << master->getDeviceType ();
		sendMsg (_os);
		master->sendMetaInfo (this);
		master->baseInfo (this);
		master->info (this);
		master->sendFullStateInfo (this);
	}
}

DevConnectionMaster::DevConnectionMaster (Block * _master, char *_device_host, int _device_port, const char *_device_name, int _device_type, const char *_master_host, int _master_port, int _serverNum):Connection (-1, _master)
{
	device_host = _device_host;
	device_port = _device_port;
	strncpy (device_name, _device_name, DEVICE_NAME_SIZE);
	device_type = _device_type;
	strncpy (master_host, _master_host, HOST_NAME_MAX);
	master_port = _master_port;
	setCentraldNum (_serverNum);

	setOtherType (DEVICE_TYPE_SERVERD);

	// set state to broken, so we can wait for server reconnecting
	setConnState (CONN_BROKEN);
	time (&nextTime);
	nextTime -= 1;
}

DevConnectionMaster::~DevConnectionMaster (void)
{
}

void DevConnectionMaster::connConnected ()
{
	int ret;
	if (!device_host)
	{
		device_host = new char[HOST_NAME_MAX];
		ret = gethostname (device_host, HOST_NAME_MAX);
		if (ret < 0)
		{
			logStream (MESSAGE_ERROR) << "Cannot get hostname : " << strerror (errno) << " (" << errno << "), exiting." << sendLog;
			connectionError (-1);
			delete device_host;
			device_host = NULL;
			return;
		}
	}
	setConnState (CONN_AUTH_PENDING);
	queSend (new CommandRegister (getMaster (), getCentraldNum (), device_name, device_type, device_host, device_port));
}

void DevConnectionMaster::connectionError (int last_data_size)
{
	if (sock > 0)
	{
		close (sock);
		sock = -1;
	}
	if (!isConnState (CONN_BROKEN))
	{
		setConnState (CONN_BROKEN);
		master->centraldConnBroken (this);
		time (&nextTime);
		nextTime += 60;
	}
}

int DevConnectionMaster::init ()
{
	struct hostent *master_info;
	struct sockaddr_in address;
	int ret;
	// init sock address
	address.sin_family = AF_INET;
	address.sin_port = htons (master_port);
	// get remote host
	if ((master_info = gethostbyname (master_host)))
	{
		address.sin_addr = *(struct in_addr *) master_info->h_addr;
	}
	else
	{
		logStream (MESSAGE_ERROR) <<
			"DevConnectionMaster::init cannot get host name " << master_host << " "
			<< strerror (errno) << sendLog;
		connectionError (-1);
		return -1;
	}
	// get hostname
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"DevConnectionMaster::init cannot create socket: " << strerror (errno)
			<< sendLog;
		connectionError (-1);
		return -1;
	}
	ret = fcntl (sock, F_SETFL, O_NONBLOCK);
	if (ret == -1)
	{
		connectionError (-1);
		return -1;
	}
	ret = connect (sock, (struct sockaddr *) &address, sizeof (address));
	if (ret == -1)
	{
		if (errno == EINPROGRESS)
		{
			setConnState (CONN_INPROGRESS);
			return 0;
		}
		connectionError (-1);
		return -1;
	}
	// just try to register us and hope for the best..
	connConnected ();
	return 0;
}

int DevConnectionMaster::idle ()
{
	time_t now;
	time (&now);
	switch (getConnState ())
	{
		case CONN_BROKEN:
			setConnTimeout (0);
			if (now > nextTime)
			{
				nextTime = now + 60;
				init ();
			}
			break;
		default:
			successfullRead ();
			setConnTimeout (300);
			break;
	}
	return Connection::idle ();
}

int DevConnectionMaster::command ()
{
	Connection *auth_conn;
	if (isCommand (PROTO_AUTH))
	{
		char *a_type;
		int auth_id;
		if (paramNextString (&a_type))
			return -1;
		if (!strcmp ("authorization_ok", a_type))
		{
			if (paramNextInteger (&auth_id) || !paramEnd ())
				return -1;
			// find connection with given ID
			auth_conn = master->findCentralId (auth_id);
			if (auth_conn)
				auth_conn->authorizationOK ();
			setCommandInProgress (false);
			return -1;
		}
		else if (!strcmp ("authorization_failed", a_type))
		{
			if (paramNextInteger (&auth_id) || !paramEnd ())
				return -1;
			// find connection with given ID
			auth_conn = master->findCentralId (auth_id);
			if (auth_conn)
				auth_conn->authorizationFailed ();
			setCommandInProgress (false);
			return -1;
		}
		if (!strcmp ("registered_as", a_type))
		{
			int clientId;
			if (paramNextInteger (&clientId) || !paramEnd ())
				return -2;
			setCentraldId (clientId);
			master->centraldConnRunning (this);
			setCommandInProgress (false);
			return -1;
		}
	}
	if (isCommand ("authorization_key"))
	{
		char *p_device_name;
		int p_key;
		Connection *conn;
		if (paramNextString (&p_device_name)
			|| paramNextInteger (&p_key) || !paramEnd ())
			return -2;
		conn = master->getOpenConnection (p_device_name);
		if (conn)
			((DevConnection *)conn)->setDeviceKey (getCentraldId (), p_key);
		setCommandInProgress (false);
		return -1;
	}

	return Connection::command ();
}

void DevConnectionMaster::setState (int in_value, char * msg)
{
	Connection::setState (in_value, msg);
	master->setMasterState (this, in_value);
}

void DevConnectionMaster::setBopState (int in_value)
{
	Connection::setBopState (in_value);
	((Device *) master)->setFullBopState (in_value);
}

int DevConnectionMaster::authorize (DevConnection * conn)
{
	queSend (new CommandAuthorize (getMaster (), conn->getCentraldId (), conn->getKey ()));
	return 0;
}

void DevConnectionMaster::setConnState (conn_state_t new_conn_state)
{
        Connection::setConnState (new_conn_state);
        if (isConnState (CONN_AUTH_OK))
        {
                master->statusInfo (this);
        }
}

CommandDeviceStatusInfo::CommandDeviceStatusInfo (Device * master, Connection * in_owner_conn):Command (master)
{
	owner_conn = in_owner_conn;
	setCommand ("status_info");
}

int CommandDeviceStatusInfo::commandReturnOK (Connection * conn)
{
	((Device *) owner)->endDeviceStatusCommand ();
	return Command::commandReturnOK (conn);
}

int CommandDeviceStatusInfo::commandReturnFailed (int status, Connection * conn)
{
	((Device *) owner)->endDeviceStatusCommand ();
	return Command::commandReturnFailed (status, conn);
}

void CommandDeviceStatusInfo::deleteConnection (Connection * conn)
{
	if (conn == owner_conn)
		owner_conn = NULL;
}

Device::Device (int in_argc, char **in_argv, int in_device_type, const char *default_name):Daemon (in_argc, in_argv)
{
	/* put defaults to variables.. */
	device_name = default_name;

	fullBopState = 0;

	log_option = 0;

	device_type = in_device_type;

	device_host = NULL;

	deviceStatusCommand = NULL;

	doCheck = true;
	doAuth = true;

	// now add options..
	addOption (OPT_NOAUTH, "noauth", 0, "allow unauthorized connections");
	addOption (OPT_NOTCHECKNULL, "notcheck", 0, "ignore if some recomended values are not set");
	addOption (OPT_LOCALHOST, "localhost", 1, "hostname, if it different from return of gethostname()");
	addOption (OPT_SERVER, "server", 1, "hostname (and possibly port number, separated by :) of central server");
	addOption ('d', NULL, 1, "name of device");
}

Device::~Device (void)
{
	delete device_host;
}

DevConnection * Device::createConnection (int in_sock)
{
	return new DevConnection (in_sock, this);
}

int Device::commandAuthorized (Connection * conn)
{
	if (conn->isCommand (COMMAND_INFO))
	{
		return info (conn);
	}
	else if (conn->isCommand ("base_info"))
	{
		return baseInfo (conn);
	}
	else if (conn->isCommand ("killall"))
	{
		return killAll (true);
	}
	else if (conn->isCommand ("killall_wse"))
	{
		return killAll (false);
	}
	else if (conn->isCommand ("exit"))
	{
		conn->setConnState (CONN_DELETE);
		deleteConnection (conn);
		return -1;
	}
	else if (conn->isCommand ("script_ends"))
	{
		return scriptEnds ();
	}
	// pseudo-command; will not be answered with ok ect..
	// as it can occur inside command block
	else if (conn->isCommand ("this_device"))
	{
		char *deviceName;
		int deviceType;
		if (conn->paramNextString (&deviceName)
			|| conn->paramNextInteger (&deviceType)
			|| !conn->paramEnd ())
			return -2;
		conn->setName (-1, deviceName);
		conn->setOtherType (deviceType);
		conn->setCommandInProgress (false);
		return -1;
	}
	else if (conn->isCommand ("device_status"))
	{
		queDeviceStatusCommand (conn);
		// OK will be returned when command return from centrald
		return -1;
	}
	else if (conn->isCommand ("autosave"))
	{
		return autosaveValues ();
	}
	// we need to try that - due to other device commands
	return -5;
}

int Device::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_LOCALHOST:
			device_host = optarg;
			break;
		case OPT_SERVER:
			centraldHosts.push_back (HostString (optarg));
			break;
		case OPT_NOTCHECKNULL:
			doCheck = false;
			break;
		case OPT_NOAUTH:
			doAuth = false;
			break;
		case 'd':
			device_name = optarg;
			break;
		default:
			return Daemon::processOption (in_opt);
	}
	return 0;
}

Connection *Device::getCentraldConn (const char *server)
{
	std::list <HostString>::iterator iter_h;
	connections_t::iterator iter_c;
	for (iter_h = centraldHosts.begin (), iter_c = getCentraldConns ()->begin ();
		iter_h != centraldHosts.end () && iter_c != getCentraldConns ()->end ();
		iter_h++, iter_c++)
	{
		if (strcasecmp (iter_h->getHostname (), server) == 0)
			return *iter_c;
	}
	return NULL;
}

void Device::queDeviceStatusCommand (Connection *in_owner_conn)
{
	deviceStatusCommand = new CommandDeviceStatusInfo (this, in_owner_conn);
	(*getCentraldConns ()->begin ())->queCommand (deviceStatusCommand);
}

int Device::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	return Daemon::setValue (old_value, new_value);
}

Connection * Device::createClientConnection (NetworkAddress * in_addres)
{
	DevConnection *conn;
	if (in_addres->isAddress (device_name))
		return NULL;
	conn = createConnection (-1);
	conn->setDeviceAddress (in_addres);
	return conn;
}

void Device::checkQueChanges (int fakeState)
{
	int ret;
	bool changed = false;
	for (rts2core::ValueQueVector::iterator iter = queValues.begin (); iter != queValues.end ();)
	{
		rts2core::ValueQue *queVal = *iter;
		// free qued values
		if (!queValueChange (queVal->getCondValue (), fakeState))
		{
			std::string newValStr = std::string (queVal->getNewValue ()->getDisplayValue ());
			ret = doSetValue (
				queVal->getCondValue (),
				queVal->getOperation (),
				queVal->getNewValue ()
				);
			if (ret)
				logStream (MESSAGE_ERROR)
					<< "cannot set queued value "
					<< queVal->getOldValue ()->getName ()
					<< " with operator " << queVal->getOperation ()
					<< " and operand " << newValStr
					<< sendLog;
			else
				logStream (MESSAGE_DEBUG)
					<< "change value of '"
					<< queVal->getOldValue ()->getName ()
					<< "' from que with operator " << queVal->getOperation ()
					<< " and operand " << newValStr
					<< sendLog;
			delete queVal;
			iter = queValues.erase (iter);
			changed = true;
		}
		else
		{
			iter++;
		}
	}
	if (changed)
	{
		setFullBopState (fakeState);
	}
}

void Device::stateChanged (int new_state, int old_state, const char *description, Connection *commandedConn)
{
	Daemon::stateChanged (new_state, old_state, description, commandedConn);
	// try to wake-up queued changes..
	checkQueChanges (new_state);
	sendStatusMessage (getState (), description, commandedConn);
}

void Device::sendFullStateInfo (Connection * conn)
{
	sendBopMessage (getState (), fullBopState, conn);
}

int Device::init ()
{
	int ret;
	// try to open log file..

	ret = Daemon::init ();
	if (ret)
		return ret;

	// add localhost and server..
	if (centraldHosts.size () == 0)
	{
		centraldHosts.push_back (HostString ("localhost", CENTRALD_PORT));
	}

	std::list <HostString>::iterator iter = centraldHosts.begin ();
	for (int i = 0; iter != centraldHosts.end (); iter++, i++)
	{
		Connection * conn_master = new DevConnectionMaster (this, device_host, getPort (), device_name, device_type, (*iter).getHostname (), (*iter).getPort (), i);
		conn_master->init ();
		addCentraldConnection (conn_master, true);
	}

	return initHardware ();
}

void Device::beforeRun ()
{
	if (doCheck && checkNotNulls ())
	{
		logStream (MESSAGE_ERROR) << "some values are not set. Please see above errors and fix them, or run driver with --nocheck option" << sendLog;
		exit (1);
	}
	std::string s = std::string (getLockPrefix ()) + std::string (device_name);
	int ret = checkLockFile (s.c_str ());
	if (ret < 0)
		exit (ret);
	ret = doDaemonize ();
	if (ret)
		exit (ret);
#ifndef RTS2_HAVE_FLOCK
	// reopen..
	ret = checkLockFile (s.c_str ());
	if (ret < 0)
		exit (ret);
#endif
	ret = lockFile ();
	if (ret)
		exit (ret);
}

int Device::authorize (int centrald_num, DevConnection * conn)
{
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if ((*iter)->getCentraldNum () == centrald_num)
			return ((DevConnectionMaster*) (*iter))->authorize (conn);
	}
	return -1;
}

int Device::sendMasters (const char *msg)
{
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		(*iter)->sendMsg (msg);
	}
	return 0;
}

void Device::centraldConnRunning (Connection *conn)
{
	Daemon::centraldConnRunning (conn);
	sendMetaInfo (conn);
}

void Device::sendMessage (messageType_t in_messageType, const char *in_messageString)
{
	Daemon::sendMessage (in_messageType, in_messageString);
	for (connections_t::iterator iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		Message msg = Message (getDeviceName (), in_messageType, in_messageString);
		(*iter)->sendMessage (msg);
	}
}

int Device::killAll (bool callScriptEnd)
{
	// remove all queued changes - do not perform them
	for (rts2core::ValueQueVector::iterator iter = queValues.begin (); iter != queValues.end (); )
	{
		delete *iter;
		iter = queValues.erase (iter);
	}
	int ret;
	if (callScriptEnd)
		ret = scriptEnds ();
	else
		ret = 0;
	// reset all errors
	maskState (DEVICE_ERROR_KILL | DEVICE_ERROR_HW | DEVICE_NOT_READY, DEVICE_ERROR_KILL, "reseting all errors");
	return ret;
}

int Device::scriptEnds ()
{
	if (modesel)
	{
		setMode (0);
		sendValueAll (modesel);
	}
	logStream (MESSAGE_DEBUG) << "executed script ends, all values changed to defaults" << sendLog;
	return 0;
}

int Device::statusInfo (Connection * conn)
{
	sendStatusMessageConn (getState (), conn);
	resendProgress (conn);
	return 0;
}

void Device::setFullBopState (int new_state)
{
	for (rts2core::ValueQueVector::iterator iter = queValues.begin (); iter != queValues.end (); iter++)
	{
		new_state = maskQueValueBopState (new_state, (*iter)->getStateCondition ());
	}
	// send new BOP state to all connected clients
	sendBopMessage (getState (), new_state);
	if (deviceStatusCommand)
	{
		// we get what we waited for..
		endDeviceStatusCommand ();
	}
	fullBopState = new_state;
}

rts2core::Value * Device::getValue (const char *_device_name, const char *value_name)
{
	if (!strcmp (_device_name, getDeviceName ()) || !strcmp (_device_name, "."))
		return getOwnValue (value_name);
	return Daemon::getValue (_device_name, value_name);
}

int Device::maskQueValueBopState (int new_state, int valueQueCondition)
{
	return new_state;
}
