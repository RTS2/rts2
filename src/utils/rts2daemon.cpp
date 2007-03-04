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
  if (sendMetaInfo (conn))
    {
      delete conn;
      return;
    }
  addConnection (conn);
}

Rts2Daemon::Rts2Daemon (int in_argc, char **in_argv):
Rts2Block (in_argc, in_argv)
{
  lockf = 0;

  daemonize = DO_DEAMONIZE;

  createValue (info_time, "infotime",
	       "time when this informations were correct", false);

  idleInfoInterval = -1;
  nextIdleInfo = 0;

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

int
Rts2Daemon::initValues ()
{
  return 0;
}

void
Rts2Daemon::initDaemon ()
{
  int ret;
  ret = init ();
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Cannot init daemon, exiting" << sendLog;
      exit (ret);
    }
  ret = initValues ();
  if (ret)
    {
      logStream (MESSAGE_ERROR) << "Cannot init values in daemon, exiting" <<
	sendLog;
      exit (ret);
    }
}

int
Rts2Daemon::run ()
{
  initDaemon ();
  return Rts2Block::run ();
}

int
Rts2Daemon::idle ()
{
  time_t now = time (NULL);
  if (idleInfoInterval >= 0 && now > nextIdleInfo)
    {
      infoAll ();
    }
  return Rts2Block::idle ();
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

void
Rts2Daemon::addValue (Rts2Value * value)
{
  values.push_back (value);
}

Rts2Value *
Rts2Daemon::getValue (const char *v_name)
{
  Rts2ValueList::iterator iter;
  for (iter = values.begin (); iter != values.end (); iter++)
    {
      Rts2Value *val = *iter;
      if (val->isValue (v_name))
	return val;
    }
  return NULL;
}

void
Rts2Daemon::addConstValue (Rts2Value * value)
{
  constValues.push_back (value);
}

void
Rts2Daemon::addConstValue (char *in_name, const char *in_desc, char *in_value)
{
  Rts2ValueString *val = new Rts2ValueString (in_name, std::string (in_desc));
  val->setValueString (in_value);
  addConstValue (val);
}

void
Rts2Daemon::addConstValue (char *in_name, const char *in_desc,
			   double in_value)
{
  Rts2ValueDouble *val = new Rts2ValueDouble (in_name, std::string (in_desc));
  val->setValueDouble (in_value);
  addConstValue (val);
}

void
Rts2Daemon::addConstValue (char *in_name, const char *in_desc, int in_value)
{
  Rts2ValueInteger *val =
    new Rts2ValueInteger (in_name, std::string (in_desc));
  val->setValueInteger (in_value);
  addConstValue (val);
}

void
Rts2Daemon::addConstValue (char *in_name, char *in_value)
{
  Rts2ValueString *val = new Rts2ValueString (in_name);
  val->setValueString (in_value);
  addConstValue (val);
}

void
Rts2Daemon::addConstValue (char *in_name, double in_value)
{
  Rts2ValueDouble *val = new Rts2ValueDouble (in_name);
  val->setValueDouble (in_value);
  addConstValue (val);
}

void
Rts2Daemon::addConstValue (char *in_name, int in_value)
{
  Rts2ValueInteger *val = new Rts2ValueInteger (in_name);
  val->setValueInteger (in_value);
  addConstValue (val);
}

int
Rts2Daemon::setValue (Rts2Value * old_value, Rts2Value * newValue)
{
  // we don't know how to set values, so return -2
  return -2;
}

int
Rts2Daemon::baseInfo ()
{
  return 0;
}

int
Rts2Daemon::baseInfo (Rts2Conn * conn)
{
  int ret;
  ret = baseInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
      return -1;
    }
  return sendBaseInfo (conn);
}

int
Rts2Daemon::sendBaseInfo (Rts2Conn * conn)
{
  for (Rts2ValueList::iterator iter = constValues.begin ();
       iter != constValues.end (); iter++)
    {
      Rts2Value *val = *iter;
      int ret;
      ret = val->sendInfo (conn);
      if (ret)
	return ret;
    }
  return 0;
}

int
Rts2Daemon::info ()
{
  struct timeval infot;
  gettimeofday (&infot, NULL);
  info_time->setValueDouble (infot.tv_sec + infot.tv_usec / USEC_SEC);
  return 0;
}

int
Rts2Daemon::info (Rts2Conn * conn)
{
  int ret;
  ret = info ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
      return -1;
    }
  return sendInfo (conn);
}

int
Rts2Daemon::infoAll ()
{
  int ret;
  nextIdleInfo = time (NULL) + idleInfoInterval;
  ret = info ();
  if (ret)
    return -1;
  connections_t::iterator iter;
  for (iter = connectionBegin (); iter != connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      sendInfo (conn);
    }
  return 0;
}

int
Rts2Daemon::sendInfo (Rts2Conn * conn)
{
  for (Rts2ValueList::iterator iter = values.begin ();
       iter != values.end (); iter++)
    {
      Rts2Value *val = *iter;
      int ret;
      ret = val->sendInfo (conn);
      if (ret)
	return ret;
    }
  return 0;
}

int
Rts2Daemon::sendMetaInfo (Rts2Conn * conn)
{
  for (Rts2ValueList::iterator iter = constValues.begin ();
       iter != constValues.end (); iter++)
    {
      Rts2Value *val = *iter;
      int ret;
      ret = val->sendMetaInfo (conn);
      if (ret < 0)
	{
	  return -1;
	}
    }
  for (Rts2ValueList::iterator iter = values.begin ();
       iter != values.end (); iter++)
    {
      Rts2Value *val = *iter;
      int ret;
      ret = val->sendMetaInfo (conn);
      if (ret < 0)
	{
	  return -1;
	}
    }
  return 0;
}

int
Rts2Daemon::setValue (Rts2Conn * conn)
{
  char *v_name;
  char *op;
  int ret;
  if (conn->paramNextString (&v_name) || conn->paramNextString (&op))
    return -2;
  Rts2Value *old_value = getValue (v_name);
  if (!old_value)
    return -2;
  Rts2Value *newValue;

  // create new value, which will be passed to hook
  switch (old_value->getValueType ())
    {
    case RTS2_VALUE_STRING:
      newValue =
	new Rts2ValueString (old_value->getName ().c_str (),
			     old_value->getDescription (),
			     old_value->getWriteToFits ());
      break;
    case RTS2_VALUE_INTEGER:
      newValue =
	new Rts2ValueInteger (old_value->getName ().c_str (),
			      old_value->getDescription (),
			      old_value->getWriteToFits ());
      break;
    case RTS2_VALUE_TIME:
      newValue =
	new Rts2ValueTime (old_value->getName ().c_str (),
			   old_value->getDescription (),
			   old_value->getWriteToFits ());
      break;
    case RTS2_VALUE_DOUBLE:
      newValue =
	new Rts2ValueDouble (old_value->getName ().c_str (),
			     old_value->getDescription (),
			     old_value->getWriteToFits ());
      break;
    case RTS2_VALUE_FLOAT:
      newValue =
	new Rts2ValueFloat (old_value->getName ().c_str (),
			    old_value->getDescription (),
			    old_value->getWriteToFits ());
      break;
    case RTS2_VALUE_BOOL:
      newValue =
	new Rts2ValueBool (old_value->getName ().c_str (),
			   old_value->getDescription (),
			   old_value->getWriteToFits ());
      break;
    default:
      logStream (MESSAGE_ERROR) << "unknow value type: " << old_value->
	getValueType () << sendLog;
      return -2;
    }
  ret = newValue->setValue (conn);
  if (ret)
    return ret;
  ret = newValue->doOpValue (*op, old_value);
  if (ret)
    return ret;

  // call hook
  ret = setValue (old_value, newValue);
  if (ret)
    return ret;

  // set value after sucessfull return..
  old_value->setFromValue (newValue);
  return 0;
}
