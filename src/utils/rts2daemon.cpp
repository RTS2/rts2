#include <stdio.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#include "rts2daemon.h"

void
Rts2Daemon::addConnectionSock (int in_sock)
{
  Rts2Conn *conn = createConnection (in_sock);
  addConnection (conn);
}

Rts2Daemon::Rts2Daemon (int in_argc, char **in_argv):
Rts2Block (in_argc, in_argv)
{
  lockf = 0;

  daemonize = DO_DEAMONIZE;
  addOption ('i', "interactive", 0,
	     "run in interactive mode, don't loose console");
}

Rts2Daemon::~Rts2Daemon (void)
{
  if (listen_sock >= 0)
    close (listen_sock);
  if (lockf)
    close (lockf);
}

int
Rts2Daemon::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'i':
      daemonize = DONT_DEAMONIZE;
      break;
    default:
      return Rts2Block::processOption (in_opt);
    }
  return 0;
}

int
Rts2Daemon::checkLockFile (const char *lock_fname)
{
  int ret;
  lockf = open (lock_fname, O_RDWR | O_CREAT);
  if (lockf == -1)
    {
      logStream (MESSAGE_ERROR) << "cannot open lock file " << lock_fname <<
	sendLog;
      return -1;
    }
  ret = flock (lockf, LOCK_EX | LOCK_NB);
  if (ret)
    {
      if (errno == EWOULDBLOCK)
	{
	  logStream (MESSAGE_ERROR) << "lock file " << lock_fname <<
	    " owned by another process" << sendLog;
	  return -1;
	}
      logStream (MESSAGE_DEBUG) << "cannot flock " << lock_fname << ": " <<
	strerror (errno) << sendLog;
      return -1;
    }
  return 0;
}

int
Rts2Daemon::doDeamonize ()
{
  if (daemonize != DO_DEAMONIZE)
    return 0;
  int ret;
  ret = fork ();
  if (ret < 0)
    {
      logStream (MESSAGE_ERROR)
	<< "Rts2Daemon::int daemonize fork " << strerror (errno) << sendLog;
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
  daemonize = IS_DEAMONIZED;
  openlog (NULL, LOG_PID, LOG_DAEMON);
  return 0;
}

int
Rts2Daemon::lockFile ()
{
  FILE *lock_file;
  int ret;
  if (!lockf)
    return -1;
  lock_file = fdopen (lockf, "w+");
  if (!lock_file)
    {
      logStream (MESSAGE_ERROR) << "cannot open lock file for writing" <<
	sendLog;
      return -1;
    }

  fprintf (lock_file, "%i\n", getpid ());

  ret = fflush (lock_file);
  if (ret)
    {
      logStream (MESSAGE_DEBUG) << "cannot flush lock file " <<
	strerror (errno) << sendLog;
      return -1;
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

  listen_sock = socket (PF_INET, SOCK_STREAM, 0);
  if (listen_sock == -1)
    {
      logStream (MESSAGE_ERROR) << "Rts2Daemon::init create listen socket " <<
	strerror (errno) << sendLog;
      return -1;
    }
  const int so_reuseaddr = 1;
  setsockopt (listen_sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
	      sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (getPort ());
  server.sin_addr.s_addr = htonl (INADDR_ANY);
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Rts2Daemon::init binding to port: " <<
    getPort () << sendLog;
#endif /* DEBUG_EXTRA */
  ret = bind (listen_sock, (struct sockaddr *) &server, sizeof (server));
  if (ret == -1)
    {
      logStream (MESSAGE_ERROR) << "Rts2Daemon::init bind " <<
	strerror (errno) << sendLog;
      return -1;
    }
  socklen_t sock_size = sizeof (server);
  ret = getsockname (listen_sock, (struct sockaddr *) &server, &sock_size);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2Daemon::init getsockname " <<
	strerror (errno) << sendLog;
      return -1;
    }
  setPort (ntohs (server.sin_port));
  ret = listen (listen_sock, 5);
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Rts2Block::init cannot listen: " <<
	strerror (errno) << sendLog;
      close (listen_sock);
      listen_sock = -1;
      return -1;
    }
  return 0;
}

void
Rts2Daemon::initDaemon ()
{
  int ret;
  ret = init ();
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Cannot init deamon, exiting" << sendLog;
      exit (ret);
    }
}

int
Rts2Daemon::run ()
{
  initDaemon ();
  return Rts2Block::run ();
}

void
Rts2Daemon::forkedInstance ()
{
  if (listen_sock >= 0)
    close (listen_sock);
  Rts2Block::forkedInstance ();
}

void
Rts2Daemon::sendMessage (messageType_t in_messageType,
			 const char *in_messageString)
{
  int prio;
  switch (daemonize)
    {
    case IS_DEAMONIZED:
    case DO_DEAMONIZE:
      switch (in_messageType)
	{
	case MESSAGE_ERROR:
	  prio = LOG_ERR;
	  break;
	case MESSAGE_WARNING:
	  prio = LOG_WARNING;
	  break;
	case MESSAGE_INFO:
	  prio = LOG_INFO;
	  break;
	case MESSAGE_DEBUG:
	  prio = LOG_DEBUG;
	  break;
	}
      syslog (prio, "%s", in_messageString);
      if (daemonize == IS_DEAMONIZED)
	break;
    case DONT_DEAMONIZE:
      // print to stdout
      Rts2Block::sendMessage (in_messageType, in_messageString);
      break;
    case CENTRALD_OK:
      break;
    }
}

void
Rts2Daemon::centraldConnRunning ()
{
  if (daemonize == IS_DEAMONIZED)
    {
      closelog ();
      daemonize = CENTRALD_OK;
    }
}

void
Rts2Daemon::centraldConnBroken ()
{
  if (daemonize == CENTRALD_OK)
    {
      openlog (NULL, LOG_PID, LOG_DAEMON);
      daemonize = IS_DEAMONIZED;
    }
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
	  addConnectionSock (client);
	}
    }
  Rts2Block::selectSuccess (read_set);
}
