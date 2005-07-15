#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "rts2block.h"
#include "rts2command.h"
#include "rts2client.h"
#include "rts2dataconn.h"

#include "imghdr.h"

Rts2Conn::Rts2Conn (Rts2Block * in_master):Rts2Object ()
{
  sock = -1;
  master = in_master;
  buf_top = buf;
  *name = '\0';
  key = 0;
  priority = -1;
  have_priority = 0;
  centrald_id = -1;
  conn_state = CONN_UNKNOW;
  type = NOT_DEFINED_SERVER;
  runningCommand = NULL;
  for (int i = 0; i < MAX_STATE; i++)
    serverState[i] = NULL;
  otherDevice = NULL;
  otherType = -1;

  time (&lastGoodSend);
  time (&lastData);

  connectionTimeout = 300;	// 5 minutes timeout
}

Rts2Conn::Rts2Conn (int in_sock, Rts2Block * in_master):
Rts2Object ()
{
  sock = in_sock;
  master = in_master;
  buf_top = buf;
  *name = '\0';
  key = 0;
  priority = -1;
  have_priority = 0;
  centrald_id = -1;
  conn_state = CONN_CONNECTED;
  type = NOT_DEFINED_SERVER;
  runningCommand = NULL;
  for (int i = 0; i < MAX_STATE; i++)
    serverState[i] = NULL;
  otherDevice = NULL;
  otherType = -1;

  time (&lastGoodSend);
  time (&lastData);

  connectionTimeout = 150;	// 5 minutes timeout (150 + 150)
}

Rts2Conn::~Rts2Conn (void)
{
  if (sock >= 0)
    close (sock);
  for (int i = 0; i < MAX_STATE; i++)
    if (serverState[i])
      delete serverState[i];
  delete otherDevice;
  queClear ();
}

int
Rts2Conn::add (fd_set * set)
{
  if (sock >= 0)
    {
      FD_SET (sock, set);
    }
}

void
Rts2Conn::postEvent (Rts2Event * event)
{
  if (otherDevice)
    otherDevice->postEvent (new Rts2Event (event));
  Rts2Object::postEvent (event);
}

void
Rts2Conn::postMaster (Rts2Event * event)
{
  master->postEvent (event);
}

int
Rts2Conn::idle ()
{
  time_t now;
  time (&now);
  if (now > lastData + connectionTimeout
      || now > lastGoodSend + connectionTimeout)
    send ("T ready");
  if (now > lastData + connectionTimeout * 2)
    endConnection ();
}

int
Rts2Conn::acceptConn ()
{
  int new_sock;
  struct sockaddr_in other_side;
  socklen_t addr_size = sizeof (struct sockaddr_in);
  new_sock = accept (sock, (struct sockaddr *) &other_side, &addr_size);
  if (new_sock == -1)
    {
      syslog (LOG_DEBUG, "Rts2Conn::acceptConn data accept");
      return -1;
    }
  else
    {
      close (sock);
      sock = new_sock;
      syslog (LOG_DEBUG, "Rts2Conn::acceptConn connection accepted");
      setConnState (CONN_CONNECTED);
      return 0;
    }
}

int
Rts2Conn::setState (int in_state_num, char *in_state_name, int in_value)
{
  if (!serverState[in_state_num])
    serverState[in_state_num] = new Rts2ServerState (in_state_name);
  return setState (in_state_name, in_value);
}

int
Rts2Conn::setState (char *in_state_name, int in_value)
{
  for (int i = 0; i < MAX_STATE; i++)
    {
      Rts2ServerState *state;
      state = serverState[i];
      if (state && !strcmp (state->name, in_state_name))
	{
	  state->value = in_value;
	  if (otherDevice)
	    otherDevice->stateChanged (state);
	  return 0;
	}
    }
  return -1;
}

void
Rts2Conn::setOtherType (int other_device_type)
{
  if (otherDevice)
    delete otherDevice;
  otherDevice = master->createOtherType (this, other_device_type);
  otherType = other_device_type;
}

int
Rts2Conn::getOtherType ()
{
  if (otherDevice)
    return otherType;
  return -1;
}

int
Rts2Conn::processLine ()
{
  // starting at command_start, we have complete line, which was
  // received
  int ret;

  // find command parameters end
  command_buf_top = command_start;

  while (*command_buf_top && !isspace (*command_buf_top))
    command_buf_top++;
  // mark command end..
  if (*command_buf_top)
    {
      *command_buf_top = '\0';
      command_buf_top++;
    }
  // messages..
  if (isCommand ("M"))
    {
      ret = message ();
    }
  // informations..
  else if (isCommand ("I"))
    {
      ret = informations ();
    }
  // status
  else if (isCommand ("S"))
    {
      ret = status ();
    }
  // technical - to keep connection working
  else if (isCommand ("T"))
    {
      char *msg;
      if (paramNextString (&msg) || !paramEnd ())
	return -1;
      if (!strcmp (msg, "ready"))
	{
	  send ("T OK");
	  return -1;
	}
      if (!strcmp (msg, "OK"))
	{
	  return -1;
	}
      return -2;
    }
  else if (isCommandReturn ())
    {
      ret = commandReturn ();
    }
  else
    {
      ret = command ();
    }
  syslog (LOG_DEBUG, "Rts2Conn::processLine [%i] command: %s ret: %i",
	  getCentraldId (), getCommand (), ret);
  if (!ret)
    sendCommandEnd (0, "OK");
  else if (ret == -2)
    sendCommandEnd (DEVDEM_E_COMMAND,
		    "invalid parameters/invalid number of parameters");
  return ret;
}

int
Rts2Conn::receive (fd_set * set)
{
  int data_size = 0;
  // connections market for deletion
  if (isConnState (CONN_DELETE))
    return -1;
  if ((sock >= 0) && FD_ISSET (sock, set))
    {
      if (isConnState (CONN_CONNECTING))
	{
	  return acceptConn ();
	}
      int ret;
      data_size = read (sock, buf_top, MAX_DATA - (buf_top - buf));
      if (data_size <= 0)
	return connectionError ();
      buf_top[data_size] = '\0';
      successfullRead ();
      syslog (LOG_DEBUG, "Rts2Conn::receive reas: %s full_buf: %s size: %i",
	      buf_top, buf, data_size);
      // put old data size into account..
      data_size += buf_top - buf;
      buf_top = buf;
      command_start = buf;
      while (*buf_top)
	{
	  while (isspace (*buf_top) || (*buf_top && *buf_top == '\n'))
	    buf_top++;
	  command_start = buf_top;
	  // find command end..
	  while (*buf_top && *buf_top != '\n' && *buf_top != '\r')
	    buf_top++;

	  // tr "\r\n" "\n\n", so we can deal more easily with it..
	  if (*buf_top == '\r' && *(buf_top + 1) == '\n')
	    *buf_top = '\n';

	  if (*buf_top == '\n')
	    {
	      // mark end of line..
	      *buf_top = '\0';
	      buf_top++;
	      processLine ();
	      command_start = buf_top;
	    }
	}
      if (buf != command_start)
	{
	  memmove (buf, command_start, (buf_top - command_start + 1));
	  // move buffer to the end..
	  buf_top -= command_start - buf;
	}
    }
  return data_size;
}

int
Rts2Conn::getOurAddress (struct sockaddr_in *addr)
{
  // get our address and pass it to data conn
  socklen_t size;
  size = sizeof (struct sockaddr_in);

  return getsockname (sock, (struct sockaddr *) addr, &size);
}

void
Rts2Conn::setAddress (struct in_addr *in_address)
{
  addr = *in_address;
}

void
Rts2Conn::getAddress (char *addrBuf, int buf_size)
{
  char *addr_s;
  int ret;
  addr_s = inet_ntoa (addr);
  strncpy (addrBuf, addr_s, buf_size);
  addrBuf[buf_size - 1] = '0';
}

int
Rts2Conn::havePriority ()
{
  return have_priority || master->grantPriority (this);
}

void
Rts2Conn::setCentraldId (int in_centrald_id)
{
  centrald_id = in_centrald_id;
  master->checkPriority (this);
}

int
Rts2Conn::sendPriorityInfo (int number)
{
  char *msg;
  int ret;
  asprintf (&msg, "I status %i priority %i", number, havePriority ());
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::queCommand (Rts2Command * command)
{
  command->setConnection (this);
  if (runningCommand
      || isConnState (CONN_CONNECTING)
      || isConnState (CONN_AUTH_PENDING) || isConnState (CONN_UNKNOW))
    {
      commandQue.push_back (command);
      return 0;
    }
  runningCommand = command;
  return command->send ();
}

int
Rts2Conn::queSend (Rts2Command * command)
{
  command->setConnection (this);
  if (runningCommand)
    commandQue.push_front (runningCommand);
  runningCommand = command;
  return runningCommand->send ();
}

void
Rts2Conn::queClear ()
{
  std::list < Rts2Command * >::iterator que_iter;
  for (que_iter = commandQue.begin (); que_iter != commandQue.end ();
       que_iter++)
    {
      Rts2Command *command;
      command = (*que_iter);
      delete command;
    }
  commandQue.clear ();
}

// high-level commands, used to pass variables etc..
int
Rts2Conn::command ()
{
  if (isCommand ("device"))
    {
      int p_centraldId;
      char *p_name;
      char *p_host;
      int p_port;
      int p_device_type;
      if (paramNextInteger (&p_centraldId)
	  || paramNextString (&p_name)
	  || paramNextString (&p_host)
	  || paramNextInteger (&p_port)
	  || paramNextInteger (&p_device_type) || !paramEnd ())
	return -2;
      master->addAddress (p_name, p_host, p_port, p_device_type);
      return -1;
    }
  if (isCommand ("user"))
    {
      int p_centraldId;
      int p_priority;
      char *p_priority_have;
      char *p_login;
      char *p_status_txt = NULL;
      if (paramNextInteger (&p_centraldId)
	  || paramNextInteger (&p_priority)
	  || paramNextString (&p_priority_have)
	  || paramNextString (&p_login)
	  || paramNextStringNull (&p_status_txt))
	return -2;
      master->addUser (p_centraldId, p_priority, (*p_priority_have == '*'),
		       p_login, p_status_txt);
      return -1;
    }
  if (isCommand ("D"))
    {
      char *p_command;
      if (paramNextString (&p_command))
	return -2;
      if (!strcmp (p_command, "connect"))
	{
	  int p_chip_id;
	  char *p_hostname;
	  int p_port;
	  int p_size;
	  if (paramNextInteger (&p_chip_id)
	      || paramNextString (&p_hostname)
	      || paramNextInteger (&p_port)
	      || paramNextInteger (&p_size) || !paramEnd ())
	    return -2;
	  master->addDataConnection (this, p_hostname, p_port, p_size);
	  return -1;
	}
      return -2;
    }
  // try otherDevice
  if (otherDevice)
    {
      int ret;
      ret = otherDevice->command ();
      if (ret == 0)
	return -1;
    }
  sendCommandEnd (-4, "Unknow command");
  return -4;
}

int
Rts2Conn::informations ()
{
  char *sub_command;
  int stat_num;
  char *state_name;
  int value;
  if (paramNextString (&sub_command)
      || paramNextInteger (&stat_num)
      || paramNextString (&state_name)
      || paramNextInteger (&value) || !paramEnd ())
    return -2;
  // set initial state & name
  setState (stat_num, state_name, value);
  return -1;
}

int
Rts2Conn::status ()
{
  char *stat_name;
  int value;
  char *stat_text;
  if (paramNextString (&stat_name)
      || paramNextInteger (&value) || paramNextStringNull (&stat_text))
    return -2;
  setState (stat_name, value);
  return -1;
}

int
Rts2Conn::sendNextCommand ()
{
  // pop next command
  if (commandQue.size () > 0)
    {
      runningCommand = *(commandQue.begin ());
      commandQue.erase (commandQue.begin ());
      runningCommand->send ();
      return 0;
    }
  runningCommand = NULL;
  return -1;
}

int
Rts2Conn::commandReturn ()
{
  int ret;
  // ignore (for the moment) retuns recieved without command
  if (!runningCommand)
    {
      return -1;
    }
  ret = runningCommand->commandReturn (atoi (getCommand ()));
  switch (ret)
    {
    case RTS2_COMMAND_REQUE:
      if (runningCommand)
	runningCommand->send ();
      break;
    case -1:
      delete runningCommand;
      sendNextCommand ();
      break;
    }
  return -1;
}

int
Rts2Conn::message ()
{
  // we don't want any messages yet..
  return -1;
}

int
Rts2Conn::send (char *message)
{
  int len;
  int ret;

  if (sock == -1)
    return -1;

  len = strlen (message);

  ret = write (sock, message, len);

  if (ret != len)
    {
      syslog (LOG_ERR, "Rts2Conn::send [%i:%i] error %i sending '%s':%m",
	      getCentraldId (), sock, ret, message);
      connectionsBreak ();
      return -1;
    }
  syslog (LOG_DEBUG, "Rts2Conn::send [%i:%i] send %i: '%s'", getCentraldId (),
	  sock, ret, message);
  write (sock, "\r\n", 2);
  successfullSend ();
  return 0;
}

void
Rts2Conn::successfullSend ()
{
  time (&lastGoodSend);
}

void
Rts2Conn::successfullRead ()
{
  time (&lastData);
}

int
Rts2Conn::sendValue (char *name, int value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i", name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, int val1, int val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i %i", name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, int val1, double val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i %f", name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, char *value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %s", name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, double value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %f", name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, char *val1, int val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %s %i", name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, int val1, int val2, double val3, double val4,
		     double val5, double val6)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i %i %f %f %f %f", name, val1, val2, val3, val4,
	    val5, val6);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValueTime (char *name, time_t * value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i", name, *value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendCommandEnd (int num, char *message)
{
  char *msg;

  asprintf (&msg, "%+04i %s", num, message);
  send (msg);
  free (msg);

  return 0;
}

void
Rts2Conn::setConnState (conn_state_t new_conn_state)
{
  if (new_conn_state == CONN_AUTH_OK || new_conn_state == CONN_CONNECTED)
    {
      if (!runningCommand)
	sendNextCommand ();
      // otherwise command will be send after command which trigerred
      // state change finished..
    }
  conn_state = new_conn_state;
}

int
Rts2Conn::isConnState (conn_state_t in_conn_state)
{
  return (conn_state == in_conn_state);
}


int
Rts2Conn::paramEnd ()
{
  return !*command_buf_top;
}

int
Rts2Conn::paramNextString (char **str)
{
  while (isspace (*command_buf_top))
    command_buf_top++;
  *str = command_buf_top;
  if (!*command_buf_top)
    return -1;
  while (!isspace (*command_buf_top) && *command_buf_top)
    command_buf_top++;
  if (*command_buf_top)
    {
      *command_buf_top = '\0';
      command_buf_top++;
    }
  return 0;
}

int
Rts2Conn::paramNextStringNull (char **str)
{
  int ret;
  ret = paramNextString (str);
  if (ret)
    return !paramEnd ();
  return ret;
}

int
Rts2Conn::paramNextInteger (int *num)
{
  char *str_num;
  char *num_end;
  if (paramNextString (&str_num))
    return -1;
  *num = strtol (str_num, &num_end, 10);
  if (*num_end)
    return -1;
  return 0;
}

int
Rts2Conn::paramNextDouble (double *num)
{
  char *str_num;
  char *num_end;
  if (paramNextString (&str_num))
    return -1;
  if (!strcmp (str_num, "nan"))
    {
      *num = nan ("f");
      return 0;
    }
  *num = strtod (str_num, &num_end);
  if (*num_end)
    return -1;
  return 0;
}

int
Rts2Conn::paramNextFloat (float *num)
{
  char *str_num;
  char *num_end;
  if (paramNextString (&str_num))
    return -1;
  *num = strtof (str_num, &num_end);
  if (*num_end)
    return -1;
  return 0;
}

void
Rts2Conn::dataReceived (Rts2ClientTCPDataConn * dataConn)
{
  // calculate mean..
  double mean = 0;
  unsigned short *pixel = dataConn->getData ();
  unsigned short *top = dataConn->getTop ();
  while (pixel < top)
    {
      mean += *pixel;
      pixel++;
    }
  mean /= dataConn->getSize ();
  syslog (LOG_DEBUG, "Rts2Conn::dataReceived mean: %f", mean);
  if (otherDevice)
    {
      otherDevice->dataReceived (dataConn);
    }
}

Rts2Value *
Rts2Conn::getValue (char *value_name)
{
  if (otherDevice)
    {
      return otherDevice->getValue (value_name);
    }
  return NULL;
}

Rts2Block::Rts2Block (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
  idle_timeout = USEC_SEC * 10;
  priority_client = -1;
  openlog (NULL, LOG_PID, LOG_LOCAL0);
  for (int i = 0; i < MAX_CONN; i++)
    {
      connections[i] = NULL;
    }
  addOption ('M', "mail-to", 1, "send report mails to this adresses");
  addOption ('h', "help", 0, "write this help");
  addOption ('i', "interactive", 0,
	     "run in interactive mode, don't loose console");

  deamonize = 1;
  mailAddress = NULL;
  signal (SIGPIPE, SIG_IGN);

  masterState = 0;
}

Rts2Block::~Rts2Block (void)
{
  if (sock)
    close (sock);
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

int
Rts2Block::init ()
{
  int ret;
  ret = initOptions ();
  if (ret)
    return ret;

  if (deamonize)
    {
      int ret = fork ();
      if (ret < 0)
	{
	  perror ("Rts2Block::Rts2Device deamonize fork");
	  exit (2);
	}
      if (ret)
	exit (0);
      close (0);
      close (1);
      close (2);
      int f = open ("/dev/null", O_RDWR);
      dup (f);
      dup (f);
      dup (f);
    }

  socklen_t client_len;
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    {
      perror ("socket");
      return -errno;
    }
  const int so_reuseaddr = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
	      sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (port);
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  syslog (LOG_DEBUG, "Rts2Block::init binding to port: %i", port);
  ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
  if (ret == -1)
    {
      syslog (LOG_ERR, "Rts2Block::init bind %m");
      return -errno;
    }
  ret = listen (sock, 1);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2Block::init cannot accept: %m");
      close (sock);
      sock = -1;
      return -1;
    }
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
Rts2Block::addConnection (int in_sock)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  syslog (LOG_DEBUG, "Rts2Block::addConnection add conn: %i", i);
	  connections[i] = createConnection (in_sock, i);
	  return 0;
	}
    }
  syslog (LOG_ERR,
	  "Rts2Block::addConnection Cannot find empty connection!\n");
  return -1;
}

int
Rts2Block::addConnection (Rts2Conn * conn)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  syslog (LOG_DEBUG, "Rts2Block::addConnection add conn: %i", i);
	  connections[i] = conn;
	  return 0;
	}
    }
  syslog (LOG_ERR,
	  "Rts2Block::addConnection Cannot find empty connection!\n");
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

int
Rts2Block::sendMessage (char *message)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	{
	  conn->send (message);
	}
    }
  return 0;
}

int
Rts2Block::sendMessage (char *message, int val1, int val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "M %s %i %i", message, val1, val2);
  ret = sendMessage (msg);
  free (msg);
  return ret;
}

int
Rts2Block::sendStatusMessage (char *state_name, int state)
{
  char *msg;
  int ret;

  asprintf (&msg, "S %s %i", state_name, state);
  ret = sendMessage (msg);
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

int
Rts2Block::run ()
{
  int ret;
  int client;
  struct timeval read_tout;
  Rts2Conn *conn;
  fd_set read_set;
  int i;

  end_loop = 0;

  while (!end_loop)
    {
      read_tout.tv_sec = idle_timeout / USEC_SEC;
      read_tout.tv_usec = idle_timeout % USEC_SEC;

      FD_ZERO (&read_set);
      for (i = 0; i < MAX_CONN; i++)
	{
	  conn = connections[i];
	  if (conn)
	    {
	      conn->add (&read_set);
	    }
	}
      FD_SET (sock, &read_set);
      ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
      if (ret)
	{
	  // accept connection on master
	  if (FD_ISSET (sock, &read_set))
	    {
	      struct sockaddr_in other_side;
	      socklen_t addr_size = sizeof (struct sockaddr_in);
	      client =
		accept (sock, (struct sockaddr *) &other_side, &addr_size);
	      if (client == -1)
		{
		  syslog (LOG_DEBUG, "client accept: %m %i", sock);
		}
	      else
		{
		  addConnection (client);
		}
	    }
	  for (i = 0; i < MAX_CONN; i++)
	    {
	      conn = connections[i];
	      if (conn)
		{
		  if (conn->receive (&read_set) == -1)
		    {
		      ret = deleteConnection (conn);
		      // delete connection only when it really requested to be deleted..
		      if (!ret)
			connections[i] = NULL;
		    }
		}
	    }
	}
      ret = idle ();
      if (ret == -1)
	break;
    }
}

int
Rts2Block::deleteConnection (Rts2Conn * conn)
{
  int ret;
  if (conn->havePriority ())
    {
      cancelPriorityOperations ();
    }
  ret = conn->connectionsBreak ();
  if (!ret)
    {
      delete conn;
    }
  return ret;
}

int
Rts2Block::setPriorityClient (int in_priority_client, int timeout)
{
  int discard_priority = -1;
  for (int i = 0; i < MAX_CONN; i++)
    {
      // discard old priority client
      if (connections[i] && connections[i]->havePriority ())
	{
	  discard_priority = i;
	  break;
	}
    }

  for (int i = 0; i < MAX_CONN; i++)
    {
      if (connections[i]
	  && connections[i]->getCentraldId () == in_priority_client)
	{
	  if (discard_priority != i)
	    {
	      cancelPriorityOperations ();
	      if (discard_priority >= 0)
		connections[discard_priority]->setHavePriority (0);
	    }
	  connections[i]->setHavePriority (1);
	  break;
	}
    }
  priority_client = in_priority_client;
}

int
Rts2Block::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'i':
      deamonize = 0;
      break;
    case 'M':
      mailAddress = optarg;
      break;
    default:
      return Rts2App::processOption (in_opt);
    }
  return 0;
}

void
Rts2Block::cancelPriorityOperations ()
{
}

void
Rts2Block::childReturned (pid_t child_pid)
{
  syslog (LOG_DEBUG, "child returned: %i", child_pid);
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

int
Rts2Block::sendMail (char *subject, char *text)
{
  int ret;
  char *cmd;
  FILE *mailFile;

  // no mail will be send
  if (!mailAddress)
    return 0;

  // fork so we will not inhibit calling process..
  ret = fork ();
  if (ret == -1)
    {
      syslog (LOG_ERR, "Rts2Block::sendMail fork: %m");
      return -1;
    }
  if (ret != 0)
    {
      return 0;
    }
  asprintf (&cmd, "/usr/bin/mail -s '%s' '%s'", subject, mailAddress);
  mailFile = popen (cmd, "w");
  if (!mailFile)
    {
      syslog (LOG_ERR, "Rts2Block::sendMail popen: %m");
      exit (0);
    }
  fprintf (mailFile, "%s", text);
  pclose (mailFile);
  free (cmd);
  exit (0);
}

Rts2Address *
Rts2Block::findAddress (char *blockName)
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
  std::list < Rts2Address * >::iterator addr_iter;
  for (addr_iter = blockAddress.begin (); addr_iter != blockAddress.end ();
       addr_iter++)
    {
      ret = (*addr_iter)->update (p_name, p_host, p_port, p_device_type);
      if (!ret)
	return;
    }
  addAddress (new Rts2Address (p_name, p_host, p_port, p_device_type));
}

int
Rts2Block::addAddress (Rts2Address * in_addr)
{
  Rts2Conn *conn;
  blockAddress.push_back (in_addr);
  // recheck all connections waiting for our address
  conn = getOpenConnection (in_addr->getName ());
  if (conn)
    conn->addressAdded (in_addr);
  else if (willConnect (in_addr))
    {
      conn = createClientConnection (in_addr);
      if (conn)
	addConnection (conn);
    }
  return 0;
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
    case DEVICE_TYPE_PHOT:
      return new Rts2DevClientPhot (conn);
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
      syslog (LOG_ERR,
	      "Cannot find device with name '%s', creating new connection",
	      deviceName);
      conn = createClientConnection (deviceName);
      addConnection (conn);
      return conn;
    }

  // open connection to given address..
  conn = createClientConnection (devAddr);
  addConnection (conn);
  return conn;
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
      Rts2Command *newCommand = new Rts2Command (command);
      conn = getConnection ((*addr_iter)->getName ());
      conn->queCommand (newCommand);
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
