/* 
 * Device basic class.
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
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <config.h>
#include "status.h"
#include "rts2device.h"
#include "rts2command.h"

#define MINDATAPORT   5556
#define MAXDATAPORT   5656

int
Rts2DevConn::command ()
{
	int ret;
								 // authorized and running
	if ((getCentraldId () != -1 && isConnState (CONN_AUTH_OK)) || getType () == DEVICE_DEVICE)
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
		int auth_key;
		if (paramNextInteger (&auth_id)
			|| paramNextInteger (&auth_key) || !paramEnd ())
			return -2;
		setCentraldId (auth_id);
		setKey (auth_key);
		ret = master->authorize (this);
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
	return Rts2Conn::command ();
}


Rts2DevConn::Rts2DevConn (int in_sock, Rts2Device * in_master):Rts2Conn (in_sock,
in_master)
{
	address = NULL;
	master = in_master;
	if (in_sock > 0)
		setType (DEVICE_SERVER);
	else
		setType (DEVICE_DEVICE);
}


int
Rts2DevConn::init ()
{
	if (getType () != DEVICE_DEVICE)
		return Rts2Conn::init ();
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
	sock =
		socket (device_addr->ai_family, device_addr->ai_socktype,
		device_addr->ai_protocol);
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
			setConnState (CONN_CONNECTING);
			return 0;
		}
		return -1;
	}
	connAuth ();
	return 0;
}


int
Rts2DevConn::idle ()
{
	if (getType () != DEVICE_DEVICE)
		return Rts2Conn::idle ();

	if (isConnState (CONN_CONNECTING))
	{
		int err = 0;
		int ret;
		socklen_t len = sizeof (err);

		ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
		if (ret)
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_ERROR) << "Rts2Dev2DevConn::idle getsockopt " <<
				strerror (errno) << sendLog;
			#endif
			connectionError (-1);
		}
		else if (err)
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_ERROR) << "Rts2Dev2DevConn::idle getsockopt " <<
				strerror (err) << sendLog;
			#endif
			connectionError (-1);
		}
		else
		{
			connAuth ();
		}
	}
	return Rts2Conn::idle ();
}


int
Rts2DevConn::authorizationOK ()
{
	setConnState (CONN_AUTH_OK);
	sendPriorityInfo ();
	master->baseInfo ();
	master->sendBaseInfo (this);
	master->info ();
	master->sendInfo (this);
	master->sendFullStateInfo (this);
	sendCommandEnd (0, "OK authorized");
	return 0;
}


int
Rts2DevConn::authorizationFailed ()
{
	setCentraldId (-1);
	setConnState (CONN_DELETE);
	sendCommandEnd (DEVDEM_E_SYSTEM, "authorization failed");
	logStream (MESSAGE_DEBUG) << "authorization failed: " << getName () <<
		sendLog;
	return 0;
}


void
Rts2DevConn::setHavePriority (int in_have_priority)
{
	if (havePriority () != in_have_priority)
	{
		Rts2Conn::setHavePriority (in_have_priority);
	}
}


void
Rts2DevConn::setDeviceAddress (Rts2Address * in_addr)
{
	address = in_addr;
	setConnState (CONN_CONNECTING);
	setName (in_addr->getName ());
	init ();
	setOtherType (address->getType ());
}


void
Rts2DevConn::setDeviceName (char *in_name)
{
	setName (in_name);
	setConnState (CONN_RESOLVING_DEVICE);
}


void
Rts2DevConn::connAuth ()
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "auth: " << getName () << " state: " <<
		getConnState () << sendLog;
	#endif
	master->getCentraldConn ()->
		queCommand (new Rts2CommandAuthorize (master, getName ()));
	setConnState (CONN_AUTH_PENDING);
}


void
Rts2DevConn::setKey (int in_key)
{
	Rts2Conn::setKey (in_key);
	if (getType () == DEVICE_DEVICE)
	{
		if (isConnState (CONN_AUTH_PENDING))
		{
			// que to begining, send command
			// kill all runinng commands
			queSend (new Rts2CommandSendKey (master, in_key));
			setCommandInProgress (false);
		}
		else
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) <<
				"Rts2DevConn::setKey invalid connection state: " <<
				getConnState () << sendLog;
			#endif
		}
	}
}


void
Rts2DevConn::setConnState (conn_state_t new_conn_state)
{
	if (getType () != DEVICE_DEVICE)
		return Rts2Conn::setConnState (new_conn_state);
	if (new_conn_state == CONN_AUTH_OK)
	{
		char *msg;
		asprintf (&msg, "this_device %s %i", master->getDeviceName (),
			master->getDeviceType ());
		sendMsg (msg);
		free (msg);
		master->sendMetaInfo (this);
		master->baseInfo (this);
		master->info (this);
		sendPriorityInfo ();
		master->sendFullStateInfo (this);
	}
	Rts2Conn::setConnState (new_conn_state);
}


Rts2DevConnMaster::Rts2DevConnMaster (Rts2Block * in_master,
char *in_device_host,
int in_device_port,
char *in_device_name,
int in_device_type,
char *in_master_host,
int in_master_port):
Rts2Conn (-1, in_master)
{
	device_host = in_device_host;
	device_port = in_device_port;
	strncpy (device_name, in_device_name, DEVICE_NAME_SIZE);
	device_type = in_device_type;
	strncpy (master_host, in_master_host, HOST_NAME_MAX);
	master_port = in_master_port;

	setOtherType (DEVICE_TYPE_SERVERD);
}


Rts2DevConnMaster::~Rts2DevConnMaster (void)
{
}


int
Rts2DevConnMaster::registerDevice ()
{
	char *msg;
	int ret;
	if (!device_host)
	{
		device_host = new char[HOST_NAME_MAX];
		ret = gethostname (device_host, HOST_NAME_MAX);
		if (ret < 0)
			return -1;
	}
	asprintf (&msg, "register %s %i %s %i", device_name, device_type,
		device_host, device_port);
	ret = sendMsg (msg);
	free (msg);
	return ret;
}


void
Rts2DevConnMaster::connectionError (int last_data_size)
{
	if (sock > 0)
	{
		close (sock);
		sock = -1;
	}
	if (!isConnState (CONN_BROKEN))
	{
		setConnState (CONN_BROKEN);
		master->centraldConnBroken ();
		time (&nextTime);
		nextTime += 60;
	}
}


int
Rts2DevConnMaster::init ()
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
			"Rts2DevConnMaster::init cannot get host name " << master_host << " "
			<< strerror (errno) << sendLog;
		return -1;
	}
	// get hostname
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"Rts2DevConnMaster::init cannot create socket: " << strerror (errno)
			<< sendLog;
		return -1;
	}
	ret = connect (sock, (struct sockaddr *) &address, sizeof (address));
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Rts2DevConnMaster::init cannot connect: "
			<< strerror (errno) << sendLog;
		close (sock);
		sock = -1;
		return -1;
	}
	// have to wait for reply
	return registerDevice ();
}


int
Rts2DevConnMaster::idle ()
{
	time_t now;
	time (&now);
	switch (getConnState ())
	{
		case CONN_BROKEN:
			if (now > nextTime)
			{
				nextTime = now + 60;
				int ret;
				ret = init ();
				if (!ret)
					setConnState (CONN_AUTH_OK);
			}
			break;
		default:
			break;
	}
	return Rts2Conn::idle ();
}


int
Rts2DevConnMaster::command ()
{
	Rts2Conn *auth_conn;
	if (isCommand ("A"))
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
			master->centraldConnRunning ();
			setCommandInProgress (false);
			return -1;
		}
	}
	if (isCommand ("authorization_key"))
	{
		char *p_device_name;
		int p_key;
		Rts2Conn *conn;
		if (paramNextString (&p_device_name)
			|| paramNextInteger (&p_key) || !paramEnd ())
			return -2;
		conn = master->getOpenConnection (p_device_name);
		if (conn)
			conn->setKey (p_key);
		setCommandInProgress (false);
		return -1;
	}

	return Rts2Conn::command ();
}


int
Rts2DevConnMaster::priorityChange ()
{
	// change priority
	int priority_client;
	int timeout;
	if (paramNextInteger (&priority_client) || paramNextInteger (&timeout))
		return -2;
	master->setPriorityClient (priority_client, timeout);
	return -1;
}


void
Rts2DevConnMaster::setState (int in_value)
{
	Rts2Conn::setState (in_value);
	master->setMasterState (in_value);
}


void
Rts2DevConnMaster::setBopState (int in_value)
{
	Rts2Conn::setBopState (in_value);
	((Rts2Device *) master)->setFullBopState (in_value);
}


int
Rts2DevConnMaster::authorize (Rts2DevConn * conn)
{
	char *msg;
	int ret;
	asprintf (&msg, "authorize %i %i", conn->getCentraldId (), conn->getKey ());
	ret = sendMsg (msg);
	free (msg);
	return ret;
}


int
Rts2DevConnData::init ()
{
	// find empty port
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;

	struct sockaddr_in server;
	int test_port;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl (INADDR_ANY);

	// find empty port
	for (test_port = MINDATAPORT; test_port < MAXDATAPORT; test_port++)
	{
		server.sin_port = htons (test_port);
		if (bind (sock, (struct sockaddr *) &server, sizeof (server)) == 0)
			break;
	}
	if (test_port == MAXDATAPORT)
	{
		close (sock);
		sock = -1;
		return -1;
	}

	if (listen (sock, 1))
	{
		close (sock);
		sock = -1;
		return -1;
	}

	setPort (test_port);

	setConnState (CONN_CONNECTING);
	return 0;
}


int
Rts2DevConnData::sendMsg (const char *msg)
{
	return 0;
}


int
Rts2DevConnData::sendMsg (char *data, size_t data_size)
{
	if (!isConnState (CONN_CONNECTED))
		return -2;
	return write (sock, data, data_size);
}


int
Rts2DevConnData::sendHeader ()
{
	return 0;
}


Rts2CommandDeviceStatusInfo::Rts2CommandDeviceStatusInfo (Rts2Device * master, Rts2Conn * in_owner_conn):Rts2Command
(master)
{
	owner_conn = in_owner_conn;
	setCommand ("status_info");
}


int
Rts2CommandDeviceStatusInfo::commandReturnOK (Rts2Conn * conn)
{
	((Rts2Device *) owner)->endDeviceStatusCommand ();
	return Rts2Command::commandReturnOK (conn);
}


int
Rts2CommandDeviceStatusInfo::commandReturnFailed (int status, Rts2Conn * conn)
{
	((Rts2Device *) owner)->endDeviceStatusCommand ();
	return Rts2Command::commandReturnFailed (status, conn);
}


Rts2Device::Rts2Device (int in_argc, char **in_argv, int in_device_type, char *default_name):
Rts2Daemon (in_argc, in_argv)
{
	/* put defaults to variables.. */
	conn_master = NULL;

	device_name = default_name;
	centrald_host = "localhost";
	centrald_port = atoi (CENTRALD_PORT);

	fullBopState = 0;

	modefile = NULL;
	modeconf = NULL;
	modesel = NULL;

	log_option = 0;

	device_type = in_device_type;

	device_host = NULL;

	mailAddress = NULL;

	deviceStatusCommand = NULL;

	// now add options..
	addOption (OPT_LOCALHOST, "localhost", 1,
		"hostname, if it different from return of gethostname()");
	addOption (OPT_SERVER, "server", 1,
		"name of computer, on which central server runs");
	addOption (OPT_PORT, "port", 1, "port number of central host");
	addOption (OPT_MODEFILE, "modefile", 1, "file holding device modes");
	addOption ('M', NULL, 1, "send report mails to this adresses");
	addOption ('d', NULL, 1, "name of device");
}


Rts2Device::~Rts2Device (void)
{
	delete deviceStatusCommand;
	delete modeconf;
}


Rts2DevConn *
Rts2Device::createConnection (int in_sock)
{
	return new Rts2DevConn (in_sock, this);
}


int
Rts2Device::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("ready"))
	{
		return ready (conn);
	}
	else if (conn->isCommand ("info"))
	{
		return info (conn);
	}
	else if (conn->isCommand ("base_info"))
	{
		return baseInfo (conn);
	}
	else if (conn->isCommand ("killall"))
	{
		CHECK_PRIORITY;
		return killAll ();
	}
	else if (conn->isCommand ("exit"))
	{
		conn->setConnState (CONN_DELETE);
		deleteConnection (conn);
		return -1;
	}
	else if (conn->isCommand ("script_ends"))
	{
		CHECK_PRIORITY;
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
		conn->setName (deviceName);
		conn->setOtherType (deviceType);
		conn->setCommandInProgress (false);
		return -1;
	}
	else if (conn->isCommand ("device_status"))
	{
		deviceStatusCommand = new Rts2CommandDeviceStatusInfo (this, conn);
		getCentraldConn ()->queCommand (deviceStatusCommand);
		// OK will be returned when command return from centrald
		return -1;
	}
	// we need to try that - due to other device commands
	return -5;
}


int
Rts2Device::setMode (int new_mode)
{
	if (modesel == NULL)
	{
		logStream (MESSAGE_ERROR) << "Called setMode without modesel."
			<< sendLog;
		return -1;
	}
	if (new_mode < 0 || new_mode >= modesel->selSize ())
	{
		logStream (MESSAGE_ERROR) << "Invalid new mode " << new_mode
			<< "." << sendLog;
		return -1;
	}
	Rts2ConfigSection *sect = (*modeconf)[new_mode];
	// setup values
	for (Rts2ConfigSection::iterator iter = sect->begin ();
		iter != sect->end (); iter++)
	{
		Rts2Value *val = getValue ((*iter).getValueName ().c_str ());
		if (val == NULL)
		{
			logStream (MESSAGE_ERROR)
				<< "Cannot find value with name '"
				<< (*iter).getValueName ()
				<< "'." << sendLog;
			return -1;
		}
		// test for suffix
		std::string suffix = (*iter).getSuffix ();
		if (suffix.length () > 0)
		{
			if (val->getValueExtType () == RTS2_VALUE_MMAX)
			{
				if (!strcasecmp (suffix.c_str (), "min"))
				{
					((Rts2ValueDoubleMinMax *) val)->setMin ((*iter).getValueDouble ());
					sendValueAll (val);
					continue;
				}
				else if (!strcasecmp (suffix.c_str (), "max"))
				{
					((Rts2ValueDoubleMinMax *) val)->setMax ((*iter).getValueDouble ());
					sendValueAll (val);
					continue;
				}
			}
			logStream (MESSAGE_ERROR)
				<< "Do not know what to do with suffix " << suffix << "."
				<< sendLog;
			return -1;
		}
		// create and set new value
		Rts2Value *new_value = duplicateValue (val);
		int ret;
		ret = new_value->setValueString ((*iter).getValue ().c_str ());
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot load value " << val->getName ()
				<< " for mode " << new_mode
				<< " with value '" << (*iter).getValue ()
				<< "'." << sendLog;
			return -1;
		}
		ret = setCondValue (getCondValue (val->getName ().c_str ()), '=', new_value);
		if (ret == -2)
		{
			logStream (MESSAGE_ERROR) << "Cannot load value from mode file " << val->getName ()
				<< " mode " << new_mode
				<< " value '" << new_value->getValue ()
				<< sendLog;
			return -1;
		}
	}
	return 0;
}


int
Rts2Device::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_LOCALHOST:
			device_host = optarg;
			break;
		case OPT_SERVER:
			centrald_host = optarg;
			break;
		case OPT_PORT:
			centrald_port = atoi (optarg);
			break;
		case OPT_MODEFILE:
			modefile = optarg;
			break;
		case 'M':
			mailAddress = optarg;
			break;
		case 'd':
			device_name = optarg;
			break;
		default:
			return Rts2Daemon::processOption (in_opt);
	}
	return 0;
}


void
Rts2Device::cancelPriorityOperations ()
{
	scriptEnds ();
}


int
Rts2Device::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == modesel)
	{
		return setMode (new_value->getValueInteger ())? -2 : 0;
	}
	return Rts2Daemon::setValue (old_value, new_value);
}


void
Rts2Device::clearStatesPriority ()
{
	maskState (0xffffff | DEVICE_ERROR_MASK, DEVICE_ERROR_KILL,
		"all operations canceled by priority");
}


int
Rts2Device::deleteConnection (Rts2Conn * conn)
{
	if (deviceStatusCommand && deviceStatusCommand->getOwnerConn ())
	{
		deviceStatusCommand->nullOwnerConn ();
		deviceStatusCommand = NULL;
	}
	return Rts2Daemon::deleteConnection (conn);
}


Rts2Conn *
Rts2Device::createClientConnection (char *in_device_name)
{
	Rts2DevConn *conn;
	conn = createConnection (-1);
	conn->setDeviceName (in_device_name);
	return conn;
}


Rts2Conn *
Rts2Device::createClientConnection (Rts2Address * in_addres)
{
	Rts2DevConn *conn;
	if (in_addres->isAddress (device_name))
		return NULL;
	conn = createConnection (-1);
	conn->setDeviceAddress (in_addres);
	return conn;
}


void
Rts2Device::checkQueChanges (int fakeState)
{
	int ret;
	for (Rts2ValueQueVector::iterator iter = queValues.begin (); iter != queValues.end ();)
	{
		Rts2ValueQue *queVal = *iter;
		// free qued values
		if (!queValueChange (queVal->getCondValue (), fakeState))
		{
			std::string newValStr = std::string (queVal->getNewValue ()->getValue ());
			ret = doSetValue (
				queVal->getCondValue (),
				queVal->getOperation (),
				queVal->getNewValue ()
				);
			if (ret)
				logStream (MESSAGE_ERROR)
					<< "cannot set qued value "
					<< queVal->getOldValue ()->getName ()
					<< " with operation " << queVal->getOperation ()
					<< " and new value value " << newValStr
					<< sendLog;
			else
				logStream (MESSAGE_DEBUG)
					<< "change value of' "
					<< queVal->getOldValue ()->getName ()
					<< "' from que to " << newValStr
					<< sendLog;
			delete queVal;
			iter = queValues.erase (iter);
		}
		else
		{
			iter++;
		}
	}
}


void
Rts2Device::stateChanged (int new_state, int old_state, const char *description)
{
	Rts2Daemon::stateChanged (new_state, old_state, description);
	// try to wake-up qued changes..
	checkQueChanges (new_state);
	sendStatusMessage (getState ());
}


void
Rts2Device::sendStateInfo (Rts2Conn * conn)
{
	sendStatusMessage (getState (), conn);
}


void
Rts2Device::sendFullStateInfo (Rts2Conn * conn)
{
	sendStateInfo (conn);
	sendBopMessage (fullBopState, conn);
}


int
Rts2Device::init ()
{
	int ret;
	char *lock_fname;

	// try to open log file..

	ret = Rts2Daemon::init ();
	if (ret)
		return ret;

	asprintf (&lock_fname, LOCK_PREFIX "%s", device_name);
	ret = checkLockFile (lock_fname);
	if (ret < 0)
		return ret;
	ret = doDeamonize ();
	if (ret)
		return ret;
	ret = lockFile ();
	if (ret)
		return ret;

	free (lock_fname);

	// check for modefile
	if (modefile != NULL)
	{
		modeconf = new Rts2ConfigRaw ();
		ret = modeconf->loadFile (modefile);
		if (ret)
			return ret;

		createValue (modesel, "MODE", "mode name", true, RTS2_VALUE_DEVPREFIX);

		for (Rts2ConfigRaw::iterator iter = modeconf->begin ();
			iter != modeconf->end (); iter++)
		{
			modesel->addSelVal ((*iter)->getName ());
		}
		setMode (0);
	}

	conn_master =
		new Rts2DevConnMaster (this, device_host, getPort (), device_name,
		device_type, centrald_host, centrald_port);
	addConnection (conn_master);

	while (conn_master->init () < 0)
	{
		logStream (MESSAGE_WARNING) << "Rts2Device::init waiting for master" <<
			sendLog;
		sleep (60);
		if (getEndLoop ())
			return -1;
	}
	return ret;
}


int
Rts2Device::authorize (Rts2DevConn * conn)
{
	return conn_master->authorize (conn);
}


int
Rts2Device::ready ()
{
	return -1;
}


void
Rts2Device::sendMessage (messageType_t in_messageType,
const char *in_messageString)
{
	Rts2Daemon::sendMessage (in_messageType, in_messageString);
	Rts2Conn *centraldConn = getCentraldConn ();
	if (!centraldConn)
		return;
	Rts2Message msg =
		Rts2Message (getDeviceName (), in_messageType, in_messageString);
	centraldConn->sendMessage (msg);
}


int
Rts2Device::sendMail (char *subject, char *text)
{
	// no mail will be send
	if (!mailAddress)
		return 0;

	return sendMailTo (subject, text, mailAddress);
}


int
Rts2Device::killAll ()
{
	cancelPriorityOperations ();
	return 0;
}


int
Rts2Device::scriptEnds ()
{
	loadValues ();
	return 0;
}


int
Rts2Device::ready (Rts2Conn * conn)
{
	int ret;
	ret = ready ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
		return -1;
	}
	return 0;
}


int
Rts2Device::statusInfo (Rts2Conn * conn)
{
	sendStatusMessage (getState (), conn);
	return 0;
}


int
Rts2Device::setFullBopState (int new_state)
{
	// send new BOP state to all connected clients
	sendBopMessage (new_state);
	if (deviceStatusCommand)
	{
		// we get what we wait for..
		endDeviceStatusCommand ();
	}
	fullBopState = new_state;
	return 0;
}
