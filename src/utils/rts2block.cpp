#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "rts2block.h"

Rts2Conn::Rts2Conn (Rts2Block * in_master)
{
  sock = -1;
  master = in_master;
  buf_top = buf;
  *name = '\0';
  priority = -1;
  have_priority = 0;
  centrald_id = -1;
  conn_state = 0;
  type = NOT_DEFINED_SERVER;
}

Rts2Conn::Rts2Conn (int in_sock, Rts2Block * in_master)
{
  sock = in_sock;
  master = in_master;
  buf_top = buf;
  *name = '\0';
  priority = -1;
  have_priority = 0;
  centrald_id = -1;
  conn_state = 0;
  type = NOT_DEFINED_SERVER;
}

Rts2Conn::~Rts2Conn (void)
{
  close (sock);
}

int
Rts2Conn::add (fd_set * set)
{
  if (sock >= 0)
    {
      FD_SET (sock, set);
    }
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
      conn_state = 0;
      return 0;
    }
}

int
Rts2Conn::receive (fd_set * set)
{
  int data_size = 0;
  // connections market for deletion
  if (conn_state == 5)
    return -1;
  if ((sock >= 0) && FD_ISSET (sock, set))
    {
      if (conn_state == 1)
	{
	  return acceptConn ();
	}
      int ret;
      char *command_end;
      data_size = read (sock, buf_top, MAX_DATA - (buf_top - buf));
      if (!data_size)
	return connectionError ();
      buf_top[data_size] = '\0';
      // put old data size into account..
      data_size = (buf_top - buf) + data_size;
      buf_top = buf;
      command_start = buf_top;
      command_end = buf_top;
      while (*buf_top)
	{
	  while (isspace (*buf_top))
	    buf_top++;
	  command_start = buf_top;
	  // find command end..
	  while (*buf_top && (!isspace (*buf_top) || *buf_top == '\n')
		 && *buf_top != '\r')
	    buf_top++;
	  syslog (LOG_DEBUG, "Rts2Conn::receive command: %s offset: %i",
		  command_start, buf_top - buf);
	  // commands should end with (at worst case) \r..
	  if (*buf_top)
	    {
	      // find command parameters end
	      command_end = buf_top;
	      while (*command_end && *command_end != '\r')
		command_end++;
	      if (*command_end == '\r')
		{
		  *command_end = '\0';
		  command_end++;
		  *buf_top = '\0';
		  buf_top++;
		  if (*buf_top == '\n')
		    buf_top++;
		  if (*command_end == '\n')
		    command_end++;
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
		  else
		    ret = command ();
		  syslog (LOG_DEBUG,
			  "Rts2Conn::receive [%i] command: %s ret: %i",
			  getCentraldId (), buf, ret);
		  if (!ret)
		    sendCommandEnd (0, "OK");
		  else if (ret == -2)
		    sendCommandEnd (DEVDEM_E_COMMAND,
				    "invalid parameters/invalid number of parameters");
		  // we processed whole received string..
		  syslog (LOG_DEBUG, "Rts2Conn::receive command_end: %i",
			  command_end - (buf + data_size));
		  if (command_end == buf + data_size)
		    {
		      syslog (LOG_WARNING, "Rts2Conn::receive null command");
		      command_end = buf;
		      buf[0] = '\0';
		    }
		}
	      buf_top = command_end;
	      syslog (LOG_DEBUG, "Rts2Conn::receive buf_top: %c", *buf_top);
	    }
	  syslog (LOG_DEBUG, "Rts2Conn::command ret: %i", data_size);
	}
      if (buf != command_start && command_end > command_start)
	{
	  memmove (buf, command_start, (command_end - command_start + 1));
	  // move buffer to the end..
	  buf_top -= command_start - buf;
	}
    }
  return data_size;
}

int
Rts2Conn::getName (struct sockaddr_in *addr)
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

void
Rts2Conn::setCentraldId (int in_centrald_id)
{
  centrald_id = in_centrald_id;
  master->checkPriority (this);
};

int
Rts2Conn::sendPriorityInfo (int number)
{
  char *msg;
  int ret;
  asprintf (&msg, "I status %i priority", number);
  ret = sendValue (msg, havePriority ());
  free (msg);
  return ret;
}

int
Rts2Conn::command ()
{
//  send (buf);
  sendCommandEnd (-4, "Unknow command");
  return -4;
}

int
Rts2Conn::informations ()
{
  return 0;
}

int
Rts2Conn::status ()
{
  return 0;
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
      return -1;
    }
  syslog (LOG_DEBUG, "Rts2Conn::send [%i:%i] send %i: '%s'", getCentraldId (),
	  sock, ret, message);
  write (sock, "\r\n", 2);
  return 0;
}

int
Rts2Conn::sendValue (char *name, int value)
{
  char *msg;
  int ret;

  asprintf (&msg, "%s %i", name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, int val1, int val2)
{
  char *msg;
  int ret;

  asprintf (&msg, "%s %i %i", name, val1, val2);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, char *value)
{
  char *msg;
  int ret;

  asprintf (&msg, "%s %s", name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2Conn::sendValue (char *name, double value)
{
  char *msg;
  int ret;

  asprintf (&msg, "%s %0.2f", name, value);
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

int
Rts2Conn::paramEnd ()
{
  return !*buf_top;
}

int
Rts2Conn::paramNextString (char **str)
{
  while (isspace (*buf_top))
    buf_top++;
  *str = buf_top;
  if (!*buf_top)
    return -1;
  while (!isspace (*buf_top) && *buf_top)
    buf_top++;
  if (*buf_top)
    {
      *buf_top = '\0';
      buf_top++;
    }
  return 0;
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

Rts2Block::Rts2Block ()
{
  idle_timeout = 1000000;
  priority_client = -1;
  openlog (NULL, LOG_PID, LOG_LOCAL0);
  for (int i = 0; i < MAX_CONN; i++)
    {
      connections[i] = NULL;
    }
}

Rts2Block::~Rts2Block (void)
{
  close (sock);
}

void
Rts2Block::setPort (int in_port)
{
  port = in_port;
}

int
Rts2Block::init ()
{
  int ret;
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
  listen (sock, 1);
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
	  connections[i] = new Rts2Conn (in_sock, this);
	  return 0;
	}
    }
  syslog (LOG_ERR,
	  "Rts2Block::addConnection Cannot find empty connection!\n");
  return -1;
}

Rts2Conn *
Rts2Block::findName (char *in_name)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	if (!strcmp (conn->getName (), in_name))
	  return conn;
    }
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
  syslog (LOG_DEBUG, "Rts2Block::idle");
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

  while (1)
    {
      read_tout.tv_sec = idle_timeout / 1000000;
      read_tout.tv_usec = idle_timeout % 1000000;

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
		  perror ("client accept");
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
		      delete conn;
		      connections[i] = NULL;
		    }
		}
	    }
	}
      idle ();
    }
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
	    }
	  if (discard_priority >= 0)
	    {
	      connections[discard_priority]->setHavePriority (0);
	    }
	  connections[i]->setHavePriority (1);
	  break;
	}
    }
  priority_client = in_priority_client;
}
