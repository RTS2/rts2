#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
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
#include "rts2dataconn.h"

#include "imghdr.h"

Rts2Block::Rts2Block (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
  idle_timeout = USEC_SEC * 10;
  priority_client = -1;
  for (int i = 0; i < MAX_CONN; i++)
    {
      connections[i] = NULL;
    }
  signal (SIGPIPE, SIG_IGN);

  masterState = 0;
  // allocate ports dynamically
  port = 0;
}

Rts2Block::~Rts2Block (void)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn;
      conn = connections[i];
      if (conn)
	delete conn;
      connections[i] = NULL;
    }
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

void
Rts2Block::postEvent (Rts2Event * event)
{
  // send to all connections
  for (int i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn;
      conn = connections[i];
      if (conn)
	{
	  conn->postEvent (new Rts2Event (event));
	}
    }
  return Rts2App::postEvent (event);
}

Rts2Conn *
Rts2Block::createConnection (int in_sock, int conn_num)
{
  return new Rts2Conn (in_sock, this);
}

Rts2Conn *
Rts2Block::addDataConnection (Rts2Conn * owner_conn, char *in_hostname,
			      int in_port, int in_size)
{
  Rts2Conn *conn;
  conn =
    new Rts2ClientTCPDataConn (this, owner_conn, in_hostname, in_port,
			       in_size);
  addConnection (conn);
  return conn;
}

int
Rts2Block::addConnection (Rts2Conn * conn)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
#ifdef DEBUG_EXTRA
	  logStream (MESSAGE_DEBUG) << "Rts2Block::addConnection add conn: "
	    << i << sendLog;
#endif
	  connections[i] = conn;
	  return 0;
	}
    }
  logStream (MESSAGE_ERROR) <<
    "Rts2Block::addConnection Cannot find empty connection!" << sendLog;
  return -1;
}

Rts2Conn *
Rts2Block::findName (const char *in_name)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
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
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	if (conn->getCentraldId () == in_id)
	  return conn;
    }
  return NULL;
}

int
Rts2Block::sendAll (char *msg)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	{
	  conn->send (msg);
	}
    }
  return 0;
}

int
Rts2Block::sendPriorityChange (int p_client, int timeout)
{
  char *msg;
  int ret;

  asprintf (&msg, PROTO_PRIORITY " %i %i", p_client, timeout);
  ret = sendAll (msg);
  free (msg);
  return ret;
}

void
Rts2Block::sendMessageAll (Rts2Message & msg)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	{
	  conn->sendMessage (msg);
	}
    }
}

int
Rts2Block::sendStatusMessage (char *state_name, int state)
{
  char *msg;
  int ret;

  asprintf (&msg, PROTO_STATUS " %s %i", state_name, state);
  ret = sendAll (msg);
  free (msg);
  return ret;
}

int
Rts2Block::idle ()
{
  int ret;
  Rts2Conn *conn;
  ret = waitpid (-1, NULL, WNOHANG);
  if (ret > 0)
    {
      childReturned (ret);
    }
  for (int i = 0; i < MAX_CONN; i++)
    {
      conn = connections[i];
      if (conn)
	conn->idle ();
    }
  return 0;
}

void
Rts2Block::addSelectSocks (fd_set * read_set)
{
  Rts2Conn *conn;
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      conn = connections[i];
      if (conn)
	{
	  conn->add (read_set);
	}
    }
}

void
Rts2Block::selectSuccess (fd_set * read_set)
{
  int i;
  Rts2Conn *conn;
  int ret;

  for (i = 0; i < MAX_CONN; i++)
    {
      conn = connections[i];
      if (conn)
	{
	  if (conn->receive (read_set) == -1)
	    {
#ifdef DEBUG_EXTRA
	      logStream (MESSAGE_ERROR) <<
		"Will delete connection " << i << ", name: " << conn->
		getName () << sendLog;
#endif
	      ret = deleteConnection (conn);
	      // delete connection only when it really requested to be deleted..
	      if (!ret)
		connections[i] = NULL;
	    }
	}
    }
}

void
Rts2Block::setMessageMask (int new_mask)
{
  Rts2Conn *conn = getCentraldConn ();
  if (!conn)
    return;
  conn->queCommand (new Rts2CommandMessageMask (this, new_mask));
}

int
Rts2Block::run ()
{
  int ret;
  struct timeval read_tout;
  fd_set read_set;

  end_loop = 0;

  while (!end_loop)
    {
      read_tout.tv_sec = idle_timeout / USEC_SEC;
      read_tout.tv_usec = idle_timeout % USEC_SEC;

      FD_ZERO (&read_set);
      addSelectSocks (&read_set);
      ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
      if (ret > 0)
	{
	  selectSuccess (&read_set);
	}
      ret = idle ();
      if (ret == -1)
	break;
    }
  return 0;
}

int
Rts2Block::deleteConnection (Rts2Conn * conn)
{
  if (conn->havePriority ())
    {
      cancelPriorityOperations ();
    }
  if (conn->isConnState (CONN_DELETE))
    {
      delete conn;
      return 0;
    }
  // don't delete us when we are in incorrect state
  return -1;
}

int
Rts2Block::setPriorityClient (int in_priority_client, int timeout)
{
  int discard_priority = 1;
  Rts2Conn *priConn;
  priConn = findCentralId (in_priority_client);
  if (priConn && priConn->getHavePriority ())
    discard_priority = 0;

  for (int i = 0; i < MAX_CONN; i++)
    {
      if (connections[i]
	  && connections[i]->getCentraldId () == in_priority_client)
	{
	  if (discard_priority)
	    {
	      cancelPriorityOperations ();
	      if (priConn)
		priConn->setHavePriority (0);
	    }
	  connections[i]->setHavePriority (1);
	  break;
	}
    }
  priority_client = in_priority_client;
  return 0;
}

void
Rts2Block::cancelPriorityOperations ()
{
}

void
Rts2Block::childReturned (pid_t child_pid)
{
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "child returned: " << child_pid << sendLog;
#endif
  for (int i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn;
      conn = connections[i];
      if (conn)
	{
	  conn->childReturned (child_pid);
	}
    }
}

int
Rts2Block::willConnect (Rts2Address * in_addr)
{
  return 0;
}

Rts2Address *
Rts2Block::findAddress (const char *blockName)
{
  std::list < Rts2Address * >::iterator addr_iter;

  for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end ();
       addr_iter++)
    {
      Rts2Address *addr = (*addr_iter);
      if (addr->isAddress (blockName))
	{
	  return addr;
	}
    }
  return NULL;
}

void
Rts2Block::addAddress (const char *p_name, const char *p_host, int p_port,
		       int p_device_type)
{
  int ret;
  Rts2Address *an_addr;
  an_addr = findAddress (p_name);
  if (an_addr)
    {
      ret = an_addr->update (p_name, p_host, p_port, p_device_type);
      if (!ret)
	{
	  addAddress (an_addr);
	  return;
	}
    }
  an_addr = new Rts2Address (p_name, p_host, p_port, p_device_type);
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
Rts2Block::deleteAddress (const char *p_name)
{
  std::list < Rts2Address * >::iterator addr_iter;

  for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end ();
       addr_iter++)
    {
      Rts2Address *addr = (*addr_iter);
      if (addr->isAddress (p_name))
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
      return new Rts2DevClientCopula (conn);
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
Rts2Block::addUser (int p_centraldId, int p_priority, char p_priority_have,
		    const char *p_login, const char *p_status_txt)
{
  int ret;
  std::list < Rts2User * >::iterator user_iter;
  for (user_iter = blockUsers.begin (); user_iter != blockUsers.end ();
       user_iter++)
    {
      ret =
	(*user_iter)->update (p_centraldId, p_priority, p_priority_have,
			      p_login, p_status_txt);
      if (!ret)
	return;
    }
  addUser (new
	   Rts2User (p_centraldId, p_priority, p_priority_have, p_login,
		     p_status_txt));
}

int
Rts2Block::addUser (Rts2User * in_user)
{
  blockUsers.push_back (in_user);
  return 0;
}

Rts2Conn *
Rts2Block::getOpenConnection (char *deviceName)
{
  Rts2Conn *conn;

  // try to find active connection..
  for (int i = 0; i < MAX_CONN; i++)
    {
      conn = connections[i];
      if (!conn)
	continue;
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
#ifdef DEBUG_EXTRA
      logStream (MESSAGE_ERROR) <<
	"Cannot find device with name " << deviceName <<
	", creating new connection" << sendLog;
#endif
      conn = createClientConnection (deviceName);
      addConnection (conn);
      return conn;
    }

  // open connection to given address..
  conn = createClientConnection (devAddr);
  addConnection (conn);
  return conn;
}

void
Rts2Block::message (Rts2Message & msg)
{
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
Rts2Block::queAll (char *text)
{
  return queAll (new Rts2Command (this, text));
}

int
Rts2Block::allQuesEmpty ()
{
  int ret;
  for (int i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn;
      conn = connections[i];
      if (conn)
	{
	  ret = conn->queEmpty ();
	  if (!ret)
	    return ret;
	}
    }
  return ret;
}

Rts2Conn *
Rts2Block::getMinConn (const char *valueName)
{
  int lovestValue = INT_MAX;
  Rts2Conn *minConn = NULL;
  for (int i = 0; i < MAX_CONN; i++)
    {
      Rts2Value *que_size;
      Rts2Conn *conn;
      conn = connections[i];
      if (conn)
	{
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
    }
  return minConn;
}
