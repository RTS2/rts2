/* 
 * Basic RTS2 devices and clients building block.
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

#include "rts2block.h"
#include "rts2command.h"
#include "rts2client.h"

#include "imghdr.h"

Rts2Block::Rts2Block (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
	idle_timeout = USEC_SEC * 10;

	signal (SIGPIPE, SIG_IGN);

	masterState = SERVERD_HARD_OFF;
	// allocate ports dynamically
	port = 0;
}


Rts2Block::~Rts2Block (void)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end ();)
	{
		Rts2Conn *conn = *iter;
		iter = connections.erase (iter);
		delete conn;
	}
	for (iter = centraldConns.begin (); iter != centraldConns.end ();)
	{
		Rts2Conn *conn = *iter;
		iter = centraldConns.erase (iter);
		delete conn;
	}
	blockAddress.clear ();
	blockUsers.clear ();
}


void
Rts2Block::setPort (int in_port)
{
	port = in_port;
}


int
Rts2Block::getPort (void)
{
	return port;
}


bool Rts2Block::commandQueEmpty ()
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


void
Rts2Block::postEvent (Rts2Event * event)
{
	// send to all connections
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->postEvent (new Rts2Event (event));
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->postEvent (new Rts2Event (event));
	return Rts2App::postEvent (event);
}


Rts2Conn *
Rts2Block::createConnection (int in_sock)
{
	return new Rts2Conn (in_sock, this);
}


void
Rts2Block::addConnection (Rts2Conn *_conn)
{
	connections_added.push_back (_conn);
}


void
Rts2Block::addCentraldConnection (Rts2Conn *_conn, bool added)
{
	if (added)
	  	centraldConns.push_back (_conn);
	else
		centraldConns_added.push_back (_conn);
}


Rts2Conn *
Rts2Block::findName (const char *in_name)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Rts2Conn *conn = *iter;
		if (!strcmp (conn->getName (), in_name))
			return conn;
	}
	// if connection not found, try to look to list of available
	// connections
	return NULL;
}


Rts2Conn *
Rts2Block::findCentralId (int in_id)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Rts2Conn *conn = *iter;
		if (conn->getCentraldId () == in_id)
			return conn;
	}
	return NULL;
}


int
Rts2Block::sendAll (const char *msg)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->sendMsg (msg);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->sendMsg (msg);
	return 0;
}


void
Rts2Block::sendValueAll (char *val_name, char *value)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->sendValue (val_name, value);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->sendValue (val_name, value);
}


void
Rts2Block::sendMessageAll (Rts2Message & msg)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->sendMessage (msg);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->sendMessage (msg);
}


void
Rts2Block::sendStatusMessage (int state)
{
	std::ostringstream _os;
	_os << PROTO_STATUS << " " << state;
	sendAll (_os);
}


void
Rts2Block::sendStatusMessage (int state, Rts2Conn * conn)
{
 	std::ostringstream _os;
	_os << PROTO_STATUS << " " << state;
	conn->sendMsg (_os);
}


void
Rts2Block::sendBopMessage (int state)
{
	std::ostringstream _os;
	_os << PROTO_BOP_STATE << " " << state;
	sendAll (_os);
}


void
Rts2Block::sendBopMessage (int state, Rts2Conn * conn)
{
	std::ostringstream _os;
	_os << PROTO_BOP_STATE << " " << state;
	conn->sendMsg (_os);
}


int
Rts2Block::idle ()
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
	std::map <double, Rts2Event *>::iterator iter_t = timers.begin ();
	while (!timers.empty () && iter_t->first < getNow ())
	{
		Rts2Event *sec = iter_t->second;
		timers.erase (iter_t++);
	 	if (sec->getArg () != NULL)
		  	((Rts2Object *)sec->getArg ())->postEvent (sec);
		else
			postEvent (sec);
	}

	return 0;
}


void
Rts2Block::addSelectSocks ()
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->add (&read_set, &write_set, &exp_set);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->add (&read_set, &write_set, &exp_set);
}


void
Rts2Block::selectSuccess ()
{
	Rts2Conn *conn;
	int ret;

	connections_t::iterator iter;

	for (iter = connections.begin (); iter != connections.end ();)
	{
		conn = *iter;
		if (conn->receive (&read_set) == -1 || conn->writable (&write_set) == -1)
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) <<
				"Will delete connection " << " name: " << conn->
				getName () << sendLog;
			#endif
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
			logStream (MESSAGE_DEBUG) <<
				"Will delete connection " << " name: " << conn->
				getName () << sendLog;
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


void
Rts2Block::setMessageMask (int new_mask)
{
	connections_t::iterator iter;
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->queCommand (new Rts2CommandMessageMask (this, new_mask));
}


void
Rts2Block::oneRunLoop ()
{
	int ret;
	struct timeval read_tout;
	double t_diff;

	if (timers.begin () != timers.end () && (t_diff = timers.begin ()->first - getNow ()) < idle_timeout)
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

	addSelectSocks ();
	ret = select (FD_SETSIZE, &read_set, &write_set, &exp_set, &read_tout);
	if (ret > 0)
		selectSuccess ();
	ret = idle ();
	if (ret == -1)
		endRunLoop ();
}


int
Rts2Block::deleteConnection (Rts2Conn * conn)
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


void
Rts2Block::connectionRemoved (Rts2Conn * conn)
{
}


void
Rts2Block::deviceReady (Rts2Conn * conn)
{
}


void
Rts2Block::deviceIdle (Rts2Conn * conn)
{
}


int
Rts2Block::changeMasterState (int new_state)
{
	connections_t::iterator iter;
	// send message to all connections that they can possibly continue executing..
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->masterStateChanged ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->masterStateChanged ();
	return 0;
}


void
Rts2Block::bopStateChanged ()
{
	connections_t::iterator iter;
	// send message to all connections that they can possibly continue executing..
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->masterStateChanged ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->masterStateChanged ();
}


void
Rts2Block::updateMetaInformations (Rts2Value *value)
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		value->sendMetaInfo (*iter);
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		value->sendMetaInfo (*iter);
}


int
Rts2Block::setMasterState (int new_state)
{
	int old_state = masterState;
	// change state NOW, before it will mess in processing routines
	masterState = new_state;
	if ((old_state & ~BOP_MASK) != (new_state & ~BOP_MASK))
	{
		// call changeMasterState only if something except BOP_MASK changed
		changeMasterState (new_state);
	}
	if ((old_state & BOP_MASK) != (new_state & BOP_MASK))
	{
		bopStateChanged ();
	}
	return 0;
}


void
Rts2Block::childReturned (pid_t child_pid)
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


int
Rts2Block::willConnect (Rts2Address * in_addr)
{
	return 0;
}


bool
Rts2Block::isGoodWeather ()
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


bool
Rts2Block::allCentraldRunning ()
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


bool
Rts2Block::someCentraldRunning ()
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



Rts2Address *
Rts2Block::findAddress (const char *blockName)
{
	std::list < Rts2Address * >::iterator addr_iter;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end (); addr_iter++)
	{
		Rts2Address *addr = (*addr_iter);
		if (addr->isAddress (blockName))
		{
			return addr;
		}
	}
	return NULL;
}


Rts2Address *
Rts2Block::findAddress (int centraldNum, const char *blockName)
{
	std::list < Rts2Address * >::iterator addr_iter;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end (); addr_iter++)
	{
		Rts2Address *addr = (*addr_iter);
		if (addr->isAddress (centraldNum, blockName))
		{
			return addr;
		}
	}
	return NULL;
}


void
Rts2Block::addAddress (int p_host_num, int p_centrald_num, int p_centrald_id, const char *p_name, const char *p_host, int p_port,
int p_device_type)
{
	int ret;
	Rts2Address *an_addr;
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
	an_addr = new Rts2Address (p_host_num, p_centrald_num, p_centrald_id, p_name, p_host, p_port, p_device_type);
	blockAddress.push_back (an_addr);
	addAddress (an_addr);
}


int
Rts2Block::addAddress (Rts2Address * in_addr)
{
	Rts2Conn *conn;
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


void
Rts2Block::deleteAddress (int p_centrald_num, const char *p_name)
{
	std::list < Rts2Address * >::iterator addr_iter;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end (); addr_iter++)
	{
		Rts2Address *addr = (*addr_iter);
		if (addr->isAddress (p_centrald_num, p_name))
		{
			blockAddress.erase (addr_iter);
			delete addr;
			return;
		}
	}
}


Rts2DevClient *
Rts2Block::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			return new Rts2DevClientTelescope (conn);
		case DEVICE_TYPE_CCD:
			return new Rts2DevClientCamera (conn);
		case DEVICE_TYPE_DOME:
			return new Rts2DevClientDome (conn);
		case DEVICE_TYPE_COPULA:
			return new Rts2DevClientCupola (conn);
		case DEVICE_TYPE_PHOT:
			return new Rts2DevClientPhot (conn);
		case DEVICE_TYPE_FW:
			return new Rts2DevClientFilter (conn);
		case DEVICE_TYPE_EXECUTOR:
			return new Rts2DevClientExecutor (conn);
		case DEVICE_TYPE_IMGPROC:
			return new Rts2DevClientImgproc (conn);
		case DEVICE_TYPE_SELECTOR:
			return new Rts2DevClientSelector (conn);
		case DEVICE_TYPE_GRB:
			return new Rts2DevClientGrb (conn);

		default:
			return new Rts2DevClient (conn);
	}
}


void
Rts2Block::addUser (int p_centraldId, const char *p_login)
{
	int ret;
	std::list < Rts2ConnUser * >::iterator user_iter;
	for (user_iter = blockUsers.begin (); user_iter != blockUsers.end (); user_iter++)
	{
		ret =
			(*user_iter)->update (p_centraldId, p_login);
		if (!ret)
			return;
	}
	addUser (new Rts2ConnUser (p_centraldId, p_login));
}


int
Rts2Block::addUser (Rts2ConnUser * in_user)
{
	blockUsers.push_back (in_user);
	return 0;
}


Rts2Conn *
Rts2Block::getOpenConnection (const char *deviceName)
{
	connections_t::iterator iter;

	// try to find active connection..
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Rts2Conn *conn = *iter;
		if (conn->isName (deviceName))
			return conn;
	}
	return NULL;
}


Rts2Conn *
Rts2Block::getConnection (char *deviceName)
{
	Rts2Conn *conn;
	Rts2Address *devAddr;

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


int
Rts2Block::getCentraldIdAtNum (int centrald_num)
{
	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
	{
		if ((*iter)->getCentraldNum () == centrald_num)
			return (*iter)->getCentraldId ();
	}
	return -1;
}


void
Rts2Block::message (Rts2Message & msg)
{
}


void
Rts2Block::clearAll ()
{
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
		(*iter)->queClear ();
	for (iter = centraldConns.begin (); iter != centraldConns.end (); iter++)
		(*iter)->queClear ();
}


int
Rts2Block::queAll (Rts2Command * command)
{
	// go throught all adresses
	std::list < Rts2Address * >::iterator addr_iter;
	Rts2Conn *conn;

	for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end ();
		addr_iter++)
	{
		conn = getConnection ((*addr_iter)->getName ());
		if (conn)
		{
			Rts2Command *newCommand = new Rts2Command (command);
			conn->queCommand (newCommand);
		}
		else
		{
			#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) << "Rts2Block::queAll no connection for "
				<< (*addr_iter)->getName () << sendLog;
			#endif
		}
	}
	delete command;
	return 0;
}


int
Rts2Block::queAll (const char *text)
{
	return queAll (new Rts2Command (this, text));
}


Rts2Conn *
Rts2Block::getMinConn (const char *valueName)
{
	int lovestValue = INT_MAX;
	Rts2Conn *minConn = NULL;
	connections_t::iterator iter;
	for (iter = connections.begin (); iter != connections.end (); iter++)
	{
		Rts2Value *que_size;
		Rts2Conn *conn = *iter;
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


Rts2Value *
Rts2Block::getValue (const char *device_name, const char *value_name)
{
	Rts2Conn *conn = getOpenConnection (device_name);
	if (!conn)
		return NULL;
	return conn->getValue (value_name);
}


int
Rts2Block::statusInfo (Rts2Conn * conn)
{
	return 0;
}


bool
Rts2Block::commandOriginatorPending (Rts2Object * object, Rts2Conn * exclude_conn)
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

void Rts2Block::deleteTimers (int event_type)
{
	for (std::map <double, Rts2Event *>::iterator iter = timers.begin (); iter != timers.end (); )
	{
		if (iter->second->getType () == event_type)
		{
			delete (iter->second);
			timers.erase (iter++);
		}
		else
		{
			iter++;
		}
	}
}
