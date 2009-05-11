/* 
 * Device basic class.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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
		logStream (MESSAGE_ERROR) << "Rts2Address::getAddress getaddrinfor for host " << address->getHost () << ":"
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


int
Rts2DevConn::authorizationOK ()
{
	setConnState (CONN_AUTH_OK);
	master->baseInfo ();
	master->sendBaseInfo (this);
	master->info ();
	master->sendInfo (this);
	master->sendFullStateInfo (this);
	sendCommandEnd (DEVDEM_OK, "OK authorized");
	return 0;
}


int
Rts2DevConn::authorizationFailed ()
{
	logStream (MESSAGE_DEBUG) << "authorization failed: " << getName ()
		<< getCentraldId () << " " << getCentraldNum ()
		<< sendLog;
	setCentraldId (-1);
	setConnState (CONN_DELETE);
	sendCommandEnd (DEVDEM_E_SYSTEM, "authorization failed");
	return 0;
}


void
Rts2DevConn::setDeviceAddress (Rts2Address * in_addr)
{
	address = in_addr;
	setConnState (CONN_CONNECTING);
	setName (in_addr->getCentraldNum (), in_addr->getName ());
	init ();
	setOtherType (address->getType ());
}


void
Rts2DevConn::setDeviceName (int _centrald_num, char *_name)
{
	setName (_centrald_num, _name);
	// find centrald connection for us..
	setConnState (CONN_RESOLVING_DEVICE);
}


void
Rts2DevConn::connConnected ()
{
	Rts2Conn::connConnected ();
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "connConnected:  " << getName () << " state: " <<
		getConnState () << sendLog;
	#endif
	if (address == NULL)
	{
		logStream (MESSAGE_ERROR) << "null address record in Rts2DevConn::connConnected for device " << getName ()
			<< " centrald num " << getCentraldNum ()
			<< sendLog;
		return;
	}

	connections_t::iterator iter;
	for (iter = getMaster ()->getCentraldConns ()->begin (); iter != getMaster ()->getCentraldConns ()->end (); iter++)
	{
		if ((*iter)->getCentraldNum () == address->getHostNum ())
		{
			(*iter)-> queCommand (new Rts2CommandKey (getMaster (), getName ()));
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


void
Rts2DevConn::setDeviceKey (int _centraldId, int _key)
{
	Rts2Conn::setKey (_key);
	if (getType () == DEVICE_DEVICE)
	{
		if (isConnState (CONN_AUTH_PENDING))
		{
			// que to begining, send command
			// kill all runinng commands

			queSend (new Rts2CommandSendKey (master, _centraldId, getCentraldNum (), _key));
			setCommandInProgress (false);
		}
		else
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) 
				<< "Rts2DevConn::setKey invalid connection state: "
				<< getConnState () << sendLog;
			#endif
		}
	}
}


void
Rts2DevConn::setConnState (conn_state_t new_conn_state)
{
	if (getType () != DEVICE_DEVICE)
		return Rts2Conn::setConnState (new_conn_state);
	Rts2Conn::setConnState (new_conn_state);
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


Rts2DevConnMaster::Rts2DevConnMaster (Rts2Block * _master, char *_device_host, int _device_port,
	const char *_device_name, int _device_type,
	const char *_master_host, int _master_port, int _serverNum)
:Rts2Conn (-1, _master)
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


Rts2DevConnMaster::~Rts2DevConnMaster (void)
{
}


void
Rts2DevConnMaster::connConnected ()
{
	int ret;
	if (!device_host)
	{
		device_host = new char[HOST_NAME_MAX];
		ret = gethostname (device_host, HOST_NAME_MAX);
		if (ret < 0)
		{
			logStream (MESSAGE_ERROR)
				<< "Cannot get hostname : "
				<< strerror (errno)
				<< " (" << errno << "), exiting."
				<< sendLog;
			connectionError (-1);
			return;
		}
	}
	setConnState (CONN_AUTH_PENDING);
	queSend (new Rts2CommandRegister (getMaster (), getCentraldNum (), device_name,
		device_type, device_host, device_port));
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
		master->centraldConnBroken (this);
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
		connectionError (-1);
		return -1;
	}
	// get hostname
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		logStream (MESSAGE_ERROR) <<
			"Rts2DevConnMaster::init cannot create socket: " << strerror (errno)
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


int
Rts2DevConnMaster::idle ()
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
				int ret;
				ret = init ();
			}
			break;
		default:
			successfullRead ();
			setConnTimeout (300);
			break;
	}
	return Rts2Conn::idle ();
}


int
Rts2DevConnMaster::command ()
{
	Rts2Conn *auth_conn;
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
		Rts2Conn *conn;
		if (paramNextString (&p_device_name)
			|| paramNextInteger (&p_key) || !paramEnd ())
			return -2;
		conn = master->getOpenConnection (p_device_name);
		if (conn)
			((Rts2DevConn *)conn)->setDeviceKey (getCentraldId (), p_key);
		setCommandInProgress (false);
		return -1;
	}

	return Rts2Conn::command ();
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
	queSend (new Rts2CommandAuthorize (getMaster (), conn->getCentraldId (), conn->getKey ()));
	return 0;
}


void
Rts2DevConnMaster::setConnState (conn_state_t new_conn_state)
{
        Rts2Conn::setConnState (new_conn_state);
        if (isConnState (CONN_AUTH_OK))
        {
                master->statusInfo (this);
        }
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


void
Rts2CommandDeviceStatusInfo::deleteConnection (Rts2Conn * conn)
{
	if (conn == owner_conn)
		owner_conn = NULL;
}


Rts2Device::Rts2Device (int in_argc, char **in_argv, int in_device_type, const char *default_name):
Rts2Daemon (in_argc, in_argv)
{
	/* put defaults to variables.. */
	device_name = default_name;

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
		"hostname (and possibly port number, separated by :) of central server");
	addOption (OPT_MODEFILE, "modefile", 1, "file holding device modes");
	addOption ('M', NULL, 1, "send report mails to this adresses");
	addOption ('d', NULL, 1, "name of device");
}


Rts2Device::~Rts2Device (void)
{
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
	if (conn->isCommand ("info"))
	{
		return info (conn);
	}
	else if (conn->isCommand ("base_info"))
	{
		return baseInfo (conn);
	}
	else if (conn->isCommand ("killall"))
	{
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
	// we need to try that - due to other device commands
	return -5;
}


int
Rts2Device::loadModefile ()
{
	if (!modefile)
		return 0;
	
	int ret;
	
	delete modeconf;
	modeconf = new Rts2ConfigRaw ();
	ret = modeconf->loadFile (modefile);
	if (ret)
		return ret;

	createValue (modesel, "MODE", "mode name", true, RTS2_VALUE_DEVPREFIX, 0, true);

	for (Rts2ConfigRaw::iterator iter = modeconf->begin ();
		iter != modeconf->end (); iter++)
	{
		modesel->addSelVal ((*iter)->getName ());
	}
	return setMode (0, true);
}


int
Rts2Device::setMode (int new_mode, bool defaultValues)
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
	for (Rts2ConfigSection::iterator iter = sect->begin ();	iter != sect->end (); iter++)
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
		ret = new_value->setValueCharArr ((*iter).getValue ().c_str ());
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot load value " << val->getName ()
				<< " for mode " << new_mode
				<< " with value '" << (*iter).getValue ()
				<< "'." << sendLog;
			return -1;
		}
		Rts2CondValue *cond_val = getCondValue (val->getName ().c_str ());
		if (defaultValues)
		{
			cond_val->setIgnoreSave ();
			deleteSaveValue (cond_val);
		}
		ret = setCondValue (cond_val, '=', new_value);
		if (ret == -2)
		{
			logStream (MESSAGE_ERROR) << "Cannot load value from mode file " << val->getName ()
				<< " mode " << new_mode
				<< " value '" << (*iter).getValue ().c_str ()
				<< "'." << sendLog;
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
			centraldHosts.push_back (HostString (optarg));
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
Rts2Device::queDeviceStatusCommand (Rts2Conn *in_owner_conn)
{
	deviceStatusCommand = new Rts2CommandDeviceStatusInfo (this, in_owner_conn);
	(*getCentraldConns ()->begin ())->queCommand (deviceStatusCommand);
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
			if (queVal->getCondValue ()->needClearValueSaveAfterLoad ())
			{
				queVal->getCondValue ()->clearValueSave ();
				queVal->getCondValue ()->clearValueSaveAfterLoad ();
			}
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
	// try to open log file..

	ret = Rts2Daemon::init ();
	if (ret)
		return ret;

	std::string s = std::string (getLockPrefix ()) + std::string (device_name);
	ret = checkLockFile (s.c_str ());
	if (ret < 0)
		return ret;
	ret = doDeamonize ();
	if (ret)
		return ret;
	ret = lockFile ();
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
		Rts2Conn * conn_master = new Rts2DevConnMaster (this, device_host, getPort (),
			device_name, device_type, (*iter).getHostname (), (*iter).getPort (), i);
		conn_master->init ();
		addCentraldConnection (conn_master, true);
	}

	return 0;
}


int
Rts2Device::initValues ()
{
	return loadModefile ();
}


int
Rts2Device::authorize (int centrald_num, Rts2DevConn * conn)
{
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if ((*iter)->getCentraldNum () == centrald_num)
			return ((Rts2DevConnMaster*) (*iter))->authorize (conn);
	}
	return -1;
}


int
Rts2Device::sendMasters (const char *msg)
{
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		(*iter)->sendMsg (msg);
	}
	return 0;
}


void
Rts2Device::centraldConnRunning (Rts2Conn *conn)
{
	Rts2Daemon::centraldConnRunning (conn);
	sendMetaInfo (conn);
}


void
Rts2Device::sendMessage (messageType_t in_messageType, const char *in_messageString)
{
	Rts2Daemon::sendMessage (in_messageType, in_messageString);
	for (connections_t::iterator iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		Rts2Message msg = Rts2Message (getDeviceName (), in_messageType, in_messageString);
		(*iter)->sendMessage (msg);
	}
}


int
Rts2Device::sendMail (const char *subject, const char *text)
{
	// no mail will be send
	if (!mailAddress)
		return 0;

	return sendMailTo (subject, text, mailAddress);
}


int
Rts2Device::killAll ()
{
	return 0;
}


int
Rts2Device::scriptEnds ()
{
	loadValues ();
	return 0;
}


int
Rts2Device::statusInfo (Rts2Conn * conn)
{
	sendStatusMessage (getState (), conn);
	return 0;
}


void
Rts2Device::setFullBopState (int new_state)
{
	for (Rts2ValueQueVector::iterator iter = queValues.begin (); iter != queValues.end (); iter++)
	{
		new_state = maskQueValueBopState (new_state, (*iter)->getStateCondition ());
	}
	// send new BOP state to all connected clients
	sendBopMessage (new_state);
	if (deviceStatusCommand)
	{
		// we get what we waited for..
		endDeviceStatusCommand ();
	}
	fullBopState = new_state;
}


int
Rts2Device::maskQueValueBopState (int new_state, int valueQueCondition)
{
	return new_state;
}


void
Rts2Device::signaledHUP ()
{
	loadModefile ();
}
