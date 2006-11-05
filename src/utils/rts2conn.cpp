#include "rts2block.h"
#include "rts2conn.h"
#include "rts2command.h"
#include "rts2devclient.h"
#include "rts2dataconn.h"
#include "rts2event.h"
#include "rts2value.h"

#include <errno.h>

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

  connectionTimeout = 300;	// 5 minutes timeout

  time (&lastGoodSend);
  lastData = lastGoodSend;
  lastSendReady = lastGoodSend - connectionTimeout;
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

  connectionTimeout = 300;	// 5 minutes timeout (150 + 150)

  time (&lastGoodSend);
  lastData = lastGoodSend;
  lastSendReady = lastData - connectionTimeout;
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
  return 0;
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
  int ret;
  if (connectionTimeout > 0)
    {
      time_t now;
      time (&now);
      if (now > lastData + getConnTimeout ()
	  && now > lastSendReady + getConnTimeout () / 4)
	{
	  ret = send (PROTO_TECHNICAL " ready");
#ifdef DEBUG_EXTRA
	  syslog (LOG_DEBUG, "Send T ready ret: %i name: '%s' type:%i", ret,
		  getName (), type);
#endif
	  time (&lastSendReady);
	}
      if (now > (lastData + getConnTimeout () * 2))
	{
	  syslog (LOG_DEBUG, "Connection timeout: %li %li %li '%s' type: %i",
		  lastGoodSend, lastData, now, getName (), type);
	  connectionError (-1);
	}
    }
  return 0;
}

int
Rts2Conn::authorizationOK ()
{
  syslog (LOG_ERR, "authorization called on wrong connection");
  return -1;
}

int
Rts2Conn::authorizationFailed ()
{
  syslog (LOG_ERR, "authorization failed on wrong connection");
  return -1;
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
      syslog (LOG_ERR, "Rts2Conn::acceptConn data accept %m");
      return -1;
    }
  else
    {
      close (sock);
      sock = new_sock;
#ifdef DEBUG_EXTRA
      syslog (LOG_DEBUG, "Rts2Conn::acceptConn connection accepted");
#endif
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
	  state->setValue (in_value);
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
  // priority change
  if (isCommand (PROTO_PRIORITY))
    {
      ret = priorityChange ();
    }
  // informations..
  else if (isCommand (PROTO_INFO))
    {
      ret = informations ();
    }
  // status
  else if (isCommand (PROTO_STATUS))
    {
      ret = status ();
    }
  // technical - to keep connection working
  else if (isCommand (PROTO_TECHNICAL))
    {
      char *msg;
      if (paramNextString (&msg) || !paramEnd ())
	return -1;
      if (!strcmp (msg, "ready"))
	{
#ifdef DEBUG_EXTRA
	  syslog (LOG_DEBUG, "Send T OK");
#endif
	  send (PROTO_TECHNICAL " OK");
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
#ifdef DEBUG_ALL
  syslog (LOG_DEBUG, "Rts2Conn::processLine [%i] command: %s ret: %i",
	  getCentraldId (), getCommand (), ret);
#endif
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
      data_size = read (sock, buf_top, MAX_DATA - (buf_top - buf));
      // ingore EINTR error
      if (data_size == -1 && errno == EINTR)
	return 0;
      if (data_size <= 0)
	return connectionError (data_size);
      buf_top[data_size] = '\0';
      successfullRead ();
#ifdef DEBUG_ALL
      syslog (LOG_DEBUG, "Rts2Conn::receive reas: %s full_buf: %s size: %i",
	      buf_top, buf, data_size);
#endif
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
	  // weird error on when we get \r in one and \n in next read
	  if (*buf_top == '\r' && !*(buf_top + 1))
	    {
	      // we get to 0, while will ends, and we get out, waiting for \n in next packet
	      buf_top++;
	      break;
	    }

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
Rts2Conn::getOurAddress (struct sockaddr_in *in_addr)
{
  // get our address and pass it to data conn
  socklen_t size;
  size = sizeof (struct sockaddr_in);

  return getsockname (sock, (struct sockaddr *) in_addr, &size);
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
Rts2Conn::queCommand (Rts2Command * cmd)
{
  cmd->setConnection (this);
  if (runningCommand
      || isConnState (CONN_CONNECTING)
      || isConnState (CONN_AUTH_PENDING) || isConnState (CONN_UNKNOW))
    {
      commandQue.push_back (cmd);
      return 0;
    }
  runningCommand = cmd;
  return cmd->send ();
}

int
Rts2Conn::queSend (Rts2Command * cmd)
{
  cmd->setConnection (this);
  if (isConnState (CONN_CONNECTING) || isConnState (CONN_UNKNOW))
    {
      commandQue.push_front (cmd);
      return 0;
    }
  if (runningCommand)
    commandQue.push_front (runningCommand);
  runningCommand = cmd;
  return runningCommand->send ();
}

int
Rts2Conn::commandReturn (Rts2Command * cmd, int in_status)
{
  if (otherDevice)
    otherDevice->commandReturn (cmd, in_status);
  return 0;
}

void
Rts2Conn::queClear ()
{
  std::list < Rts2Command * >::iterator que_iter;
  for (que_iter = commandQue.begin (); que_iter != commandQue.end ();
       que_iter++)
    {
      Rts2Command *cmd;
      cmd = (*que_iter);
      delete cmd;
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
  if (isCommand (PROTO_DATA))
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
  // don't respond to values with error - otherDevice does respond to
  // values, if there is no other device, we have to take resposibility
  // as it can fails (V without value), not with else
  if (isCommand (PROTO_VALUE))
    return -1;
  syslog (LOG_DEBUG,
	  "Rts2Conn::command unknow command: getCommand %s state: %i type: %i name: %s",
	  getCommand (), conn_state, getType (), getName ());
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
#ifdef DEBUG_EXTRA
      syslog (LOG_DEBUG, "Rts2Conn::commandReturn null!");
#endif
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
Rts2Conn::priorityChange ()
{
  // we don't want any messages yet..
  return -1;
}

int
Rts2Conn::send (const char *msg)
{
  int len;
  int ret;

  if (sock == -1)
    return -1;

  len = strlen (msg);

  ret = write (sock, msg, len);

  if (ret != len)
    {
#ifdef DEBUG_EXTRA
      syslog (LOG_ERR,
	      "Rts2Conn::send [%i:%i] error %i state: %i sending '%s':%m",
	      getCentraldId (), conn_state, sock, ret, msg);
#endif
      connectionError (ret);
      return -1;
    }
#ifdef DEBUG_ALL
  syslog (LOG_DEBUG, "Rts2Conn::send [%i:%i] send %i: '%s'", getCentraldId (),
	  sock, ret, msg);
#endif
  write (sock, "\r\n", 2);
  successfullSend ();
  return 0;
}

int
Rts2Conn::send (std::string msg)
{
  return send (msg.c_str ());
}

void
Rts2Conn::successfullSend ()
{
  time (&lastGoodSend);
}

void
Rts2Conn::getSuccessSend (time_t * in_t)
{
  *in_t = lastGoodSend;
}

int
Rts2Conn::reachedSendTimeout ()
{
  time_t now;
  time (&now);
  return now > lastGoodSend + getConnTimeout ();
}

void
Rts2Conn::successfullRead ()
{
  time (&lastData);
}

int
Rts2Conn::connectionError (int last_data_size)
{
  setConnState (CONN_DELETE);
  if (sock >= 0)
    close (sock);
  sock = -1;
  if (strlen (getName ()))
    master->deleteAddress (getName ());
  return -1;
}

int
Rts2Conn::sendMessage (Rts2Message & msg)
{
  return send (msg.toConn ());
}

int
Rts2Conn::sendValue (char *val_name, int value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i", val_name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *val_name, int val1, int val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i %i", val_name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *val_name, int val1, double val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i %f", val_name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *val_name, char *value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %s", val_name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *val_name, double value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %f", val_name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *val_name, char *val1, int val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %s %i", val_name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *val_name, int val1, int val2, double val3,
		     double val4, double val5, double val6)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %i %i %f %f %f %f", val_name, val1, val2, val3, val4,
	    val5, val6);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValueTime (char *val_name, time_t * value)
{
  char *msg;
  int ret;

  asprintf (&msg, "V %s %li", val_name, *value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendCommandEnd (int num, char *in_msg)
{
  char *msg;

  asprintf (&msg, "%+04i %s", num, in_msg);
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
  if (new_conn_state == CONN_AUTH_FAILED)
    {
      connectionError (-1);
    }
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
  if (otherDevice)
    {
      otherDevice->dataReceived (dataConn);
    }
}

Rts2Value *
Rts2Conn::getValue (const char *value_name)
{
  if (otherDevice)
    {
      return otherDevice->getValue (value_name);
    }
  return NULL;
}
