#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include "rts2client.h"
#include "rts2devclient.h"
#include "rts2logstream.h"

Rts2ConnClient::Rts2ConnClient (Rts2Block * in_master, char *in_name):
Rts2Conn (in_master)
{
  setName (in_name);
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
    return ret;
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
  connLogin ();
  return 0;
}

int
Rts2ConnClient::idle ()
{
  if (isConnState (CONN_CONNECTING))
    {
      int err = 0;
      int ret;
      socklen_t len = sizeof (err);

      ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
      if (ret)
	{
	  syslog (LOG_ERR, "Rts2ConnClient::idle getsockopt %m");
	  connectionError (-1);
	}
      else if (err)
	{
	  syslog (LOG_ERR, "Rts2ConnClient::idle getsockopt %s",
		  strerror (err));
	  connectionError (-1);
	}
      else
	{
	  connLogin ();
	}
    }
  return Rts2Conn::idle ();
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
Rts2ConnClient::connLogin ()
{
  master->getCentraldConn ()->
    queCommand (new Rts2CommandAuthorize (master, getName ()));
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
      queSend (new Rts2CommandSendKey (master, in_key));
    }
}

/**************************************************************
 *
 * Rts2Client implementation.
 *
 *************************************************************/

Rts2ConnClient *
Rts2Client::createClientConnection (char *in_deviceName)
{
  return new Rts2ConnClient (this, in_deviceName);
}

Rts2Conn *
Rts2Client::createClientConnection (Rts2Address * in_addr)
{
  Rts2ConnClient *conn;
  conn = createClientConnection (in_addr->getName ());
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
  central_port = "617";

  login = "petr";
  password = "petr";

  addOption ('s', "centrald_server", 1,
	     "hostname of central server; default to localhost");
  addOption ('q', "centrald_port", 1, "port of centrald server");
}

Rts2Client::~Rts2Client (void)
{
}

int
Rts2Client::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 's':
      central_host = optarg;
      break;
    case 'q':
      central_port = optarg;
      break;
    default:
      return Rts2Block::processOption (in_opt);
    }
  return 0;
}

int
Rts2Client::init ()
{
  int ret;

  ret = Rts2Block::init ();
  if (ret)
    return ret;

  central_conn =
    new Rts2ConnCentraldClient (this, login, password, central_host,
				central_port);
  ret = 1;
  while (1)
    {
      ret = central_conn->init ();
      if (!ret)
	break;
      std::cerr << "Trying to contact centrald\n";
      sleep (10);
    }
  return addConnection (central_conn);
}

void
Rts2Client::getMasterState (char buf[20])
{
  int currMasterState;
  currMasterState = Rts2Block::getMasterState ();
  if (currMasterState == SERVERD_OFF)
    {
      strcpy (buf, "OFF");
      return;
    }
  if ((currMasterState & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
    {
      strcpy (buf, "standby ");
    }
  else
    {
      strcpy (buf, "ready ");
    }
  switch (currMasterState & SERVERD_STATUS_MASK)
    {
    case SERVERD_DAY:
      strcat (buf, "day");
      break;
    case SERVERD_EVENING:
      strcat (buf, "evening");
      break;
    case SERVERD_DUSK:
      strcat (buf, "dusk");
      break;
    case SERVERD_NIGHT:
      strcat (buf, "night");
      break;
    case SERVERD_DAWN:
      strcat (buf, "dawn");
      break;
    case SERVERD_MORNING:
      strcat (buf, "morning");
      break;
    default:
      strcat (buf, "unknow");
      break;
    }
}

/**
 * Centrald connection.
 *
 * Used for putting devices names query etc..
 */

Rts2ConnCentraldClient::Rts2ConnCentraldClient (Rts2Block * in_master,
						char *in_login,
						char *in_password,
						char *in_master_host,
						char *in_master_port):
Rts2Conn (in_master)
{
  master_host = in_master_host;
  master_port = in_master_port;

  login = in_login;
  password = in_password;
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
      return -1;
    }
  return Rts2Conn::command ();
}

int
Rts2ConnCentraldClient::informations ()
{
  char *i_name;
  int status_num;
  char *state_name;
  int state_value;
  if (paramNextString (&i_name) || paramNextInteger (&status_num)
      || paramNextString (&state_name) || paramNextInteger (&state_value)
      || !paramEnd ())
    return 0;
  return master->setMasterState (state_value);
}

int
Rts2ConnCentraldClient::status ()
{
  char *msg;
  int new_state;
  if (paramNextString (&msg) || paramNextInteger (&new_state) || !paramEnd ())
    return -1;
  return master->setMasterState (new_state);
}

Rts2CommandLogin::Rts2CommandLogin (Rts2Block * in_master, char *in_login, char *in_password):Rts2Command
  (in_master)
{
  char *
    command;
  login = in_login;
  password = in_password;
  asprintf (&command, "login %s", login);
  setCommand (command);
  free (command);
  state = LOGIN_SEND;
}

int
Rts2CommandLogin::commandReturnOK ()
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
