#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "rts2daemon.h"

int
Rts2Daemon::addConnection (int in_sock)
{
  int i;
  for (i = 0; i < MAX_CONN; i++)
    {
      if (!connections[i])
	{
#ifdef DEBUG_ALL
	  logStream (MESSAGE_DEBUG) << "Rts2Block::addConnection add conn: "
	    << i << sendLog;
#endif
	  connections[i] = createConnection (in_sock, i);
	  return 0;
	}
    }
  logStream (MESSAGE_ERROR) <<
    "Rts2Block::addConnection Cannot find empty connection!" << sendLog;
  return -1;
}

Rts2Daemon::Rts2Daemon (int in_argc, char **in_argv):
Rts2Block (in_argc, in_argv)
{
  daemonize = true;
  addOption ('i', "interactive", 0,
	     "run in interactive mode, don't loose console");
}

Rts2Daemon::~Rts2Daemon (void)
{
  if (listen_sock >= 0)
    close (listen_sock);
}

int
Rts2Daemon::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'i':
      daemonize = false;
      break;
    default:
      return Rts2Block::processOption (in_opt);
    }
  return 0;
}

int
Rts2Daemon::init ()
{
  int ret;
  ret = Rts2Block::init ();
  if (ret)
    return ret;

  if (daemonize)
    {
      ret = fork ();
      if (ret < 0)
	{
	  std::
	    cerr << "Rts2Daemon::int deamonize fork " << strerror (errno) <<
	    std::endl;
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

  listen_sock = socket (PF_INET, SOCK_STREAM, 0);
  if (listen_sock == -1)
    {
      std::
	cerr << "Rts2Daemon::init create listen socket " << strerror (errno)
	<< std::endl;
      return -1;
    }
  const int so_reuseaddr = 1;
  setsockopt (listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
	      sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (getPort ());
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  std::cerr << "Rts2Daemon::init binding to port: " << getPort () << std::
    endl;
  ret = bind (listen_sock, (struct sockaddr *) &server, sizeof (server));
  if (ret == -1)
    {
      std::cerr << "Rts2Daemon::init bind " << strerror (errno) << std::endl;
      return -1;
    }
  socklen_t sock_size = sizeof (server);
  ret = getsockname (listen_sock, (struct sockaddr *) &server, &sock_size);
  if (ret)
    {
      std::
	cerr << "Rts2Daemon::init getsockname " << strerror (errno) << std::
	endl;
      return -1;
    }
  setPort (ntohs (server.sin_port));
  ret = listen (listen_sock, 5);
  if (ret)
    {
      std::
	cerr << "Rts2Block::init cannot listen: " << strerror (errno) << std::
	endl;
      close (listen_sock);
      listen_sock = -1;
      return -1;
    }
  return 0;
}

void
Rts2Daemon::forkedInstance ()
{
  if (listen_sock >= 0)
    close (listen_sock);
  Rts2Block::forkedInstance ();
}

void
Rts2Daemon::addSelectSocks (fd_set * read_set)
{
  FD_SET (listen_sock, read_set);
  Rts2Block::addSelectSocks (read_set);
}

void
Rts2Daemon::selectSuccess (fd_set * read_set)
{
  int client;
  // accept connection on master
  if (FD_ISSET (listen_sock, read_set))
    {
      struct sockaddr_in other_side;
      socklen_t addr_size = sizeof (struct sockaddr_in);
      client =
	accept (listen_sock, (struct sockaddr *) &other_side, &addr_size);
      if (client == -1)
	{
	  logStream (MESSAGE_DEBUG) << "client accept: " << strerror (errno)
	    << " " << listen_sock << sendLog;
	}
      else
	{
	  addConnection (client);
	}
    }
  Rts2Block::selectSuccess (read_set);
}
