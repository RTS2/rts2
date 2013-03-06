/* 
 * Basic RTS2 devices and clients building block.
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
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

#include <algorithm>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "block.h"
#include "command.h"
#include "client.h"

#include "imghdr.h"
#include "centralstate.h"

//* Null terminated list of names for different device types.
const char *type_names[] = 
{
  "UNKNOWN", "SERVERD", "MOUNT", "CCD", "DOME", "WEATHER", "ARCH", "PHOT", "PLAN", "GRB", "FOCUS", // 10
  "MIRROR", "CUPOLA", "FILTER", "AUGERSH", "SENSOR", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN" // 20
  "EXEC", "IMGP", "SELECTOR", "XMLRPC", "INDI", "LOGD", "SCRIPTOR", NULL
};

using namespace rts2core;

Block::Block (int in_argc, char **in_argv):App (in_argc, in_argv)
{
	idle_timeout = USEC_SEC * 10;

	signal (SIGPIPE, SIG_IGN);

	masterState = SERVERD_HARD_OFF;
	stateMasterConn = NULL;
	// allocate ports dynamically
	port = 0;
}


Block::~Block (void)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end ();)
	{
		Connection *conn = *iter;
		iter = connections.erase (iter);
		delete conn;
	}
	for (iter = centraldConns.begin (); iter != centraldConns.end ();)
	{
		Connection *conn = *iter;
		iter = centraldConns.erase (iter);
		delete conn;
	}
	for (std::list <NetworkAddress *>::iterator ia = blockAddress.begin (); ia != blockAddress.end (); ia++)
		delete *ia;
	blockAddress.clear ();
	for (std::list <ConnUser *>::iterator iu = blockUsers.begin (); iu != blockUsers.end (); iu++)
		delete *iu;
	blockUsers.clear ();
}

void Block::setPort (int in_port)
{
	port = in_port;
}

int Block::getPort (void)
{
	return port;
}

void Block::addSelectSocks (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->add (&read_set, &write_set, &exp_set);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->add (&read_set, &write_set, &exp_set);
}

bool Block::commandQueEmpty ()
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		if (!(*iter)->queEmpty ())
			return false;
	}
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
	{
		if (!(*iter)->queEmpty ())
			return false;
	}
	return true;
}

void Block::postEvent (Event * event)
{
	// send to all connections
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->postEvent (new Event (event));
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->postEvent (new Event (event));
	return App::postEvent (event);
}

Connection * Block::createConnection (int in_sock)
{
	return new Connection (in_sock, this);
}

void Block::addConnection (Connection *_conn)
{
	connections_added.push_back (_conn);
}

void Block::removeConnection (Connection *_conn)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end ();)
	{
		if (*iter == _conn)
			iter = connections.erase (iter);
		else
			iter++;
	}

	for (iter = connections_added.begin (); iter != connections_added.end ();)
	{
		if (*iter == _conn)
			iter = connections_added.erase (iter);
		else
			iter++;
	}
}

void Block::addCentraldConnection (Connection *_conn, bool added)
{
	if (added)
	  	centraldConns.push_back (_conn);
	else
		centraldConns_added.push_back (_conn);
}

Connection * Block::findName (const char *in_name)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Connection *conn = *iter;
		if (!strcmp (conn->getName (), in_name))
			return conn;
	}
	// if connection not found, try to look to list of available
	// connections
	return NULL;
}

Connection * Block::findCentralId (int in_id)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Connection *conn = *iter;
		if (conn->getCentraldId () == in_id)
			return conn;
	}
	return NULL;
}

int Block::sendAll (const char *msg)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->sendMsg (msg);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->sendMsg (msg);
	return 0;
}

int Block::sendAllExcept (const char *msg, Connection *exceptConn)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		if ((*iter) != exceptConn)
			(*iter)->sendMsg (msg);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
	  	if ((*iter) != exceptConn)
			(*iter)->sendMsg (msg);
	return 0;
}

void Block::sendValueAll (char *val_name, char *value)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->sendValue (val_name, value);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->sendValue (val_name, value);
}

void Block::sendMessageAll (Message & msg)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->sendMessage (msg);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->sendMessage (msg);
}

void Block::sendStatusMessage (rts2_status_t state, const char * msg, Connection *commandedConn)
{
	std::ostringstream _os;
	_os << PROTO_STATUS << " " << state;
	if (msg != NULL)
		_os << " \"" << msg << "\"";
	if (commandedConn)
	{
		sendStatusMessageConn (state | DEVICE_SC_CURR, commandedConn);
		sendAllExcept (_os, commandedConn);
	}
	else
	{
		sendAll (_os);
	}
}

void Block::sendStatusMessageConn (rts2_status_t state, Connection * conn)
{
 	std::ostringstream _os;
	_os << PROTO_STATUS << " " << state;
	conn->sendMsg (_os);
}

void Block::sendBopMessage (rts2_status_t state, rts2_status_t bopState)
{
	std::ostringstream _os;
	_os << PROTO_BOP_STATE << " " << state << " " << bopState;
	sendAll (_os);
}

void Block::sendBopMessage (rts2_status_t state, rts2_status_t bopState, Connection * conn)
{
	std::ostringstream _os;
	_os << PROTO_BOP_STATE << " " << state << " " << bopState;
	conn->sendMsg (_os);
}

int Block::idle ()
{
	int ret;
	ret = waitpid (-1, NULL, WNOHANG);
	if (ret > 0)
	{
		childReturned (ret);
	}
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->idle ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->idle ();

	// add from connection queue..
	for (iter = connections_added.begin (); iter != connections_added.end (); iter = connections_added.erase (iter))
	{
		connections.push_back (*iter);
	}

	for (iter = centraldConns_added.begin (); iter != centraldConns_added.end (); iter = centraldConns_added.erase (iter))
	{
		centraldConns.push_back (*iter);
	}

	// test for any pending timers..
	std::map <double, Event *>::iterator iter_t = timers.begin ();
	while (iter_t != timers.end () && iter_t->first < getNow ())
	{
		if (pushToDelete (iter_t))
		{
			Event *sec = iter_t->second;
		 	if (sec->getArg () != NULL)
			  	((Object *)sec->getArg ())->postEvent (sec);
			else
				postEvent (sec);
		}
		iter_t++;
	}

	// delete timers queue for delete
	for (std::vector <std::map <double, Event *>::iterator>::iterator iter_d = toDelete.begin (); iter_d != toDelete.end (); iter_d = toDelete.erase (iter_d))
	{
		timers.erase (*iter_d);
	}

	return 0;
}

void Block::selectSuccess (fd_set &read_set, fd_set &write_set, fd_set &exp_set)
{
	Connection *conn;
	int ret;

	connections_t::iterator iter;

	for (iter = connections.begin (); iter != connections.end ();)
	{
		conn = *iter;
		if (conn->receive (&read_set) == -1 || conn->writable (&write_set) == -1)
		{
			ret = deleteConnection (conn);
			// delete connection only when it really requested to be deleted..
			if (!ret)
			{
				iter = connections.erase (iter);
				connectionRemoved (conn);
				delete conn;
			}
			else
			{
				iter++;
			}
		}
		else
		{
			iter++;
		}
	}
	for (iter = centraldConns.begin (); iter != centraldConns.end ();)
	{
		conn = *iter;
		if (conn->receive (&read_set) == -1 || conn->writable (&write_set) == -1)
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "Will delete connection " << " name: " << conn->getName () << sendLog;
			#endif
			ret = deleteConnection (conn);
			// delete connection only when it really requested to be deleted..
			if (!ret)
			{
				iter = centraldConns.erase (iter);
				connectionRemoved (conn);
				delete conn;
			}
			else
			{
				iter++;
			}
		}
		else
		{
			iter++;
		}
	}
}

void Block::setMessageMask (int new_mask)
{
	connections_t::iterator iter;
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->queCommand (new CommandMessageMask (this, new_mask));
}

void Block::oneRunLoop ()
{
	int ret;
	struct timeval read_tout;
	double t_diff;

	fd_set read_set;
	fd_set write_set;
	fd_set exp_set;

	if (timers.begin () != timers.end () && (USEC_SEC * (t_diff = (timers.begin ()->first - getNow ()))) < idle_timeout)
	{
		read_tout.tv_sec = t_diff;
		read_tout.tv_usec = (t_diff - floor (t_diff)) * USEC_SEC;
	}
	else
	{
		read_tout.tv_sec = idle_timeout / USEC_SEC;
		read_tout.tv_usec = idle_timeout % USEC_SEC;
	}

	FD_ZERO (&read_set);
	FD_ZERO (&write_set);
	FD_ZERO (&exp_set);

	addSelectSocks (read_set, write_set, exp_set);
	if (select (FD_SETSIZE, &read_set, &write_set, &exp_set, &read_tout) > 0)
		selectSuccess (read_set, write_set, exp_set);
	ret = idle ();
	if (ret == -1)
		endRunLoop ();
}

int Block::deleteConnection (Connection * conn)
{
	if (conn->isConnState (CONN_DELETE))
	{
		connections_t::iterator iter;
		// try to look if there are any references to connection in other connections
		for (iter = connections.begin (); iter != connections.end (); iter++)
		{
			if (conn != *iter)
				(*iter)->deleteConnection (conn);
		}
		for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		{
			if (conn != *iter)
				(*iter)->deleteConnection (conn);
		}
		return 0;
	}
	// don't delete us when we are in incorrect state
	return -1;
}

void Block::connectionRemoved (Connection * conn)
{
}

void Block::deviceReady (Connection * conn)
{
}

void Block::deviceIdle (Connection * conn)
{
}

void Block::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	connections_t::iterator iter;
	// send message to all connections that they can possibly continue executing..
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->masterStateChanged ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->masterStateChanged ();
}

void Block::bopStateChanged ()
{
	connections_t::iterator iter;
	// send message to all connections that they can possibly continue executing..
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->masterStateChanged ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->masterStateChanged ();
}

void Block::updateMetaInformations (Value *value)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		value->sendMetaInfo (*iter);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		value->sendMetaInfo (*iter);
}

std::map <Connection *, std::vector <Value *> > Block::failedValues ()
{
	std::map <Connection *, std::vector <Value *> > ret;
	ValueVector::iterator viter;
	for (connections_t::iterator iter = connections.begin (); iter != connections.end (); iter++)
	{
		viter = (*iter)->valueBegin ();
		for (viter = (*iter)->getFailedValue (viter); viter != (*iter)->valueEnd (); viter = (*iter)->getFailedValue (viter))
		{
			ret[*iter].push_back (*viter);
			viter++;
		}
	}
	for (connections_t::iterator iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
	{
		viter = (*iter)->valueBegin ();
		for (viter = (*iter)->getFailedValue (viter); viter != (*iter)->valueEnd (); viter = (*iter)->getFailedValue (viter))
		{
			ret[*iter].push_back (*viter);
			viter++;
		}
	}
	return ret;
}

bool Block::centralServerInState (rts2_status_t state)
{
	for (connections_t::iterator iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
	{
		if ((*iter)->getState () & state)
			return true;
	}
	return false;
}

int Block::setMasterState (Connection *_conn, rts2_status_t new_state)
{
	rts2_status_t old_state = masterState;
	// ignore connections from wrong master..
	if (stateMasterConn != NULL && _conn != stateMasterConn)
	{
		if ((new_state & SERVERD_ONOFF_MASK) != SERVERD_HARD_OFF && (new_state & WEATHER_MASK) != BAD_WEATHER && (new_state & SERVERD_ONOFF_MASK) != SERVERD_STANDBY)
		{
			if ((old_state & ~BOP_MASK) != (new_state & ~BOP_MASK))
				logStream (MESSAGE_DEBUG) << "ignoring state change, as it does not arrive from master connection" << sendLog;
			return 0;
		}
		// ignore request from non-master server asking us to switch to standby, when we are in hard off
		if ((masterState & SERVERD_ONOFF_MASK) == SERVERD_HARD_OFF && (new_state & SERVERD_ONOFF_MASK) == SERVERD_STANDBY)
			return 0;
	}
	// change state NOW, before it will mess in processing routines
	masterState = new_state;

	if ((old_state & ~BOP_MASK) != (new_state & ~BOP_MASK))
	{
		// call changeMasterState only if something except BOP_MASK changed
		changeMasterState (old_state, new_state);
	}
	if ((old_state & BOP_MASK) != (new_state & BOP_MASK))
	{
		bopStateChanged ();
	}
	return 0;
}

std::string Block::getMasterStateString ()
{
	return CentralState (getMasterStateFull ()).getString ();
}

void Block::childReturned (pid_t child_pid)
{
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "child returned: " << child_pid << sendLog;
	#endif
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->childReturned (child_pid);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->childReturned (child_pid);
}

int Block::willConnect (NetworkAddress * in_addr)
{
	return 0;
}

bool Block::isGoodWeather ()
{
	if (getCentraldConns ()->size () <= 0)
		return false;
	// check if all masters are up and running and think it is good idea to open
	// the roof
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if (!((*iter)->isConnState (CONN_CONNECTED) || (*iter)->isConnState (CONN_AUTH_OK)))
			return false;
		if ((*iter)->isGoodWeather () == false)
			return false;
	}
	return true;
}

bool Block::canMove ()
{
	if (getCentraldConns ()->size () <= 0)
		return false;
	// check if all masters are up and running and think it is good idea to open
	// the roof
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if (!((*iter)->isConnState (CONN_CONNECTED) || (*iter)->isConnState (CONN_AUTH_OK)))
			return false;
		if ((*iter)->canMove () == false)
			return false;
	}
	return true;
}

bool Block::allCentraldRunning ()
{
	if (getCentraldConns ()->size () <= 0)
		return false;
	// check if all masters are up and running and think it is good idea to open
	// the roof
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if (!((*iter)->isConnState (CONN_CONNECTED) || (*iter)->isConnState (CONN_AUTH_OK)))
			return false;
	}
	return true;
}

bool Block::someCentraldRunning ()
{
	if (getCentraldConns ()->size () <= 0)
		return true;
	// check if all masters are up and running and think it is good idea to open
	// the roof
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if ((*iter)->isConnState (CONN_CONNECTED) || (*iter)->isConnState (CONN_AUTH_OK))
			return true;
	}
	return false;
}

NetworkAddress * Block::findAddress (const char *blockName)
{
	std::list < NetworkAddress * >::iterator addr_iter;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end (); addr_iter++)
	{
		NetworkAddress *addr = (*addr_iter);
		if (addr->isAddress (blockName))
		{
			return addr;
		}
	}
	return NULL;
}

NetworkAddress * Block::findAddress (int centraldNum, const char *blockName)
{
	std::list < NetworkAddress * >::iterator addr_iter;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end (); addr_iter++)
	{
		NetworkAddress *addr = (*addr_iter);
		if (addr->isAddress (centraldNum, blockName))
		{
			return addr;
		}
	}
	return NULL;
}

void Block::addAddress (int p_host_num, int p_centrald_num, int p_centrald_id, const char *p_name, const char *p_host, int p_port, int p_device_type)
{
	int ret;
	NetworkAddress *an_addr;
	an_addr = findAddress (p_centrald_num, p_name);
	if (an_addr)
	{
		ret = an_addr->update (p_centrald_num, p_name, p_host, p_port, p_device_type);
		if (!ret)
		{
			addAddress (an_addr);
			return;
		}
	}
	an_addr = new NetworkAddress (p_host_num, p_centrald_num, p_centrald_id, p_name, p_host, p_port, p_device_type);
	blockAddress.push_back (an_addr);
	addAddress (an_addr);
}

int Block::addAddress (NetworkAddress * in_addr)
{
	Connection *conn;
	// recheck all connections waiting for our address
	conn = getOpenConnection (in_addr->getName ());
	if (conn)
		conn->addressUpdated (in_addr);
	else if (willConnect (in_addr))
	{
		conn = createClientConnection (in_addr);
		if (conn)
			addConnection (conn);
	}
	return 0;
}

void Block::deleteAddress (int p_centrald_num, const char *p_name)
{
	std::list < NetworkAddress * >::iterator addr_iter;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end (); addr_iter++)
	{
		NetworkAddress *addr = (*addr_iter);
		if (addr->isAddress (p_centrald_num, p_name))
		{
			blockAddress.erase (addr_iter);
			delete addr;
			return;
		}
	}
}

DevClient * Block::createOtherType (Connection * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new DevClientTelescope (conn);
		case DEVICE_TYPE_CCD:
			return new DevClientCamera (conn);
		case DEVICE_TYPE_DOME:
			return new DevClientDome (conn);
		case DEVICE_TYPE_CUPOLA:
			return new DevClientCupola (conn);
		case DEVICE_TYPE_PHOT:
			return new DevClientPhot (conn);
		case DEVICE_TYPE_FW:
			return new DevClientFilter (conn);
		case DEVICE_TYPE_EXECUTOR:
			return new DevClientExecutor (conn);
		case DEVICE_TYPE_IMGPROC:
			return new DevClientImgproc (conn);
		case DEVICE_TYPE_SELECTOR:
			return new DevClientSelector (conn);
		case DEVICE_TYPE_GRB:
			return new DevClientGrb (conn);
		case DEVICE_TYPE_BB:
			return new DevClientBB (conn);
		default:
			return new DevClient (conn);
	}
}

void Block::addClient (int p_centraldId, const char *p_login, const char *p_name)
{
	int ret;
	std::list < ConnUser * >::iterator user_iter;
	for (user_iter = blockUsers.begin (); user_iter != blockUsers.end (); user_iter++)
	{
		ret = (*user_iter)->update (p_centraldId, p_login, p_name);
		if (!ret)
			return;
	}
	addClient (new ConnUser (p_centraldId, p_login, p_name));
}

int Block::addClient (ConnUser * in_user)
{
	blockUsers.push_back (in_user);
	return 0;
}

void Block::deleteClient (int p_centraldId)
{
	std::list < ConnUser * >::iterator user_iter;
	for (user_iter = blockUsers.begin (); user_iter != blockUsers.end (); user_iter++)
	{
		if ((*user_iter)->getCentraldId () == p_centraldId)
		{
			blockUsers.erase (user_iter);
			return;
		}
	}
}

Connection * Block::getOpenConnection (const char *deviceName)
{
	connections_t::iterator iter;

	// try to find active connection..
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Connection *conn = *iter;
		if (conn->isName (deviceName))
			return conn;
	}
	return NULL;
}

void Block::getOpenConnectionType (int deviceType, connections_t::iterator &current)
{
	for (; current != connections.end (); current++)
	{
		if ((*current)->getOtherType () == deviceType)
			return;
	}
}

Connection * Block::getOpenConnection (int device_type)
{
	connections_t::iterator iter;

	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Connection *conn = *iter;
		if (conn->getOtherType () == device_type)
			return conn;
	}
	return NULL;
}

Connection * Block::getConnection (char *deviceName)
{
	Connection *conn;
	NetworkAddress *devAddr;

	conn = getOpenConnection (deviceName);
	if (conn)
		return conn;

	devAddr = findAddress (deviceName);

	if (!devAddr)
	{
		logStream (MESSAGE_ERROR)
			<< "Cannot find device with name " << deviceName
			<< sendLog;
		return NULL;
	}

	// open connection to given address..
	conn = createClientConnection (devAddr);
	if (conn)
		addConnection (conn);
	return conn;
}

void Block::message (Message & msg)
{
}

void Block::clearAll ()
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->queClear ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->queClear ();
}

int Block::queAll (Command * command)
{
	// go throught all adresses
	std::list < NetworkAddress * >::iterator addr_iter;
	Connection *conn;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end ();
		addr_iter++)
	{
		conn = getConnection ((*addr_iter)->getName ());
		if (conn)
		{
			Command *newCommand = new Command (command);
			conn->queCommand (newCommand);
		}
		else
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "Block::queAll no connection for "
				<< (*addr_iter)->getName () << sendLog;
			#endif
		}
	}
	delete command;
	return 0;
}

int Block::queAll (const char *text)
{
	return queAll (new Command (this, text));
}

void Block::queAllCentralds (const char *command)
{
	for (connections_t::iterator iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->queCommand (new Command (this, command));
}

Connection * Block::getMinConn (const char *valueName)
{
	int lovestValue = INT_MAX;
	Connection *minConn = NULL;
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Value *que_size;
		Connection *conn = *iter;
		que_size = conn->getValue (valueName);
		if (que_size)
		{
			if (que_size->getValueInteger () >= 0
				&& que_size->getValueInteger () < lovestValue)
			{
				minConn = conn;
				lovestValue = que_size->getValueInteger ();
			}
		}
	}
	return minConn;
}

Value * Block::getValue (const char *device_name, const char *value_name)
{
	Connection *conn = getOpenConnection (device_name);
	if (!conn)
		return NULL;
	return conn->getValue (value_name);
}

Value * Block::getValueExpression (std::string expression, const char *defaultDevice)
{
	size_t sep = expression.find ('.');
	if (sep != std::string::npos)
	{
		std::string devname = expression.substr (0, sep);
		if (getCentraldConns ()->size () > 0 && (devname.length () == 0 || devname == "centrald"))
			return getCentraldConns ()->front ()->getValue (expression.substr (sep + 1).c_str ());
		return getValue (expression.substr (0, sep).c_str (), expression.substr (sep + 1).c_str ());
	}
	// else
	return getValue (defaultDevice, expression.c_str ());
}

int Block::statusInfo (Connection * conn)
{
	return 0;
}

bool Block::commandOriginatorPending (Object * object, Connection * exclude_conn)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		if ((*iter) != exclude_conn && (*iter)->commandOriginatorPending (object))
		{
			#ifdef DEBUG_EXTRA
			std::cout << "command originator pending on " << (*iter)->getName () << std::endl;
			#endif				 /* DEBUG_EXTRA */
			return true;
		}
	}
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
	{
		if ((*iter) != exclude_conn && (*iter)->commandOriginatorPending (object))
		{
			#ifdef DEBUG_EXTRA
			std::cout << "command originator pending on " << (*iter)->getName () << std::endl;
			#endif				 /* DEBUG_EXTRA */
			return true;
		}
	}
	return false;
}

void Block::deleteTimers (int event_type)
{
	for (std::map <double, Event *>::iterator iter = timers.begin (); iter != timers.end (); )
	{
		if (iter->second->getType () == event_type)
		{
			if (pushToDelete (iter))
				delete (iter->second);
		}
		iter++;
	}
}

void Block::valueMaskError (Value *val, int32_t err)
{
  	if ((val->getFlags () & RTS2_VALUE_ERRORMASK) != err)
	{
		val->maskError (err);
		updateMetaInformations (val);
	}
}

bool Block::pushToDelete (const std::map <double, Event *>::iterator &iter)
{
	// only push unique iterators
	if (std::find (toDelete.begin (), toDelete.end (), iter) == toDelete.end ())
	{
		toDelete.push_back (iter);
		return true;
	}
	return false;
}

bool isCentraldName (const char *_name)
{
	return !strcmp (_name, "..") || !strcmp (_name, "centrald");
}
