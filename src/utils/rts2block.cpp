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

Rts2Conn::Rts2Conn (int in_sock)
{
  sock = in_sock;
  buf_top = buf;
  *name = '\0';
  priority = -1;
  have_priority = 0;
  centrald_id = -1;
  type = NOT_DEFINED_SERVER;
}

int
Rts2Conn::add (fd_set * set)
{
  FD_SET (sock, set);
}

int
Rts2Conn::receive (fd_set * set)
{
  if (FD_ISSET (sock, set))
    {
      int ret;
      char *command_end;
      data_size = read (sock, buf_top, MAX_DATA - (buf_top - buf));
      if (!data_size)
	return -1;
      buf_top[data_size] = '\0';
      buf_top = buf;
      syslog (LOG_DEBUG, "command: %s", buf);
      while (*buf_top && !isspace (*buf_top) && *buf_top != '\r')
	buf_top++;
      if (*buf_top)
	{
	  command_end = buf_top;
	  while (*command_end && *command_end != '\r')
		  command_end ++;
	  if (*command_end == '\r')
	  {
	    *command_end = '\0';
	    *buf_top = '\0';
	    buf_top++;
	    if (*buf_top == '\n')
	      *buf_top = '\0';
	    printf ("[%i] command: %s\n", getCentraldId (), buf);
            ret = command ();
            if (!ret)
     	      sendCommandEnd (0, "OK");
            else if (ret == -2)
	      sendCommandEnd (DEVDEM_E_COMMAND,
			"invalid parameters/invalid number of parameters");
	    buf_top = buf;
	  }
	  else 
	  {
            buf_top = buf + data_size;
	  }
	}
    }
  return data_size;
}

int
Rts2Conn::command ()
{
  send ("%s", buf);
}

int
Rts2Conn::send (char *message, ...)
{
  va_list va;
  char *msg;
  int len;

  va_start (va, message);
  len = vasprintf (&msg, message, va);
  va_end (va);
  write (sock, msg, len);
  printf ("[%i] send: %s\n", getCentraldId(), msg);
  free (msg);
  write (sock, "\r\n", 2);
  return 0;
}

int
Rts2Conn::sendCommandEnd (int num, char *message, ...)
{
  va_list va;
  char *msg;

  va_start (va, message);
  vasprintf (&msg, message, va);
  va_end (va);
  send ("%+04i %s", num, msg);
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
};

Rts2Block::Rts2Block (int in_port)
{
  port = in_port;
  idle_timeout = 1000000;
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
  printf ("binding to port: %i\n", port);
  ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
  if (ret == -1)
    {
      perror ("bind");
      return -errno;
    }
  listen (sock, 1);
}

int
Rts2Block::addConnection (int sock)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
	  printf ("add conn: %i\n", i);
	  connections[i] = new Rts2Conn (sock);
	  return 0;
	}
    }
  fprintf (stderr, "Cannot find empty connection!\n");
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
Rts2Block::sendMessage (char *format, ...)
{
  int i;
  char *msg;
  va_list va;

  va_start (va, format);
  vasprintf (&msg, format, va);
  va_end (va);
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	{
	  conn->send ("%s", msg);
	}
    }
  free (msg);
  return 0;
}

int
Rts2Block::idle ()
{
  printf ("idle\n");
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
      read_tout.tv_sec = idle_timeout / 100000;
      read_tout.tv_usec = idle_timeout % 100000;
      printf ("timeout: %i\n", idle_timeout);

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

/*
int
main (int argc, char **argv)
{
  class Rts2Block *block = new Rts2Block (5554);
  block->init ();
  block->run ();
  delete block;
}
*/
