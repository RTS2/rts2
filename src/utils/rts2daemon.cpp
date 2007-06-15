#include <stdio.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
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

  daemonize = DO_DAEMONIZE;

  state = 0;

  createValue (info_time, "infotime",
	       "time when this informations were correct", false);

  idleInfoInterval = -1;
  nextIdleInfo = 0;

  addOption ('i', "interactive", 0,
	     "run in interactive mode, don't loose console");
  addOption (OPT_LOCALPORT, "local-port", 1,
	     "define local port on which we will listen to incoming requests");
}

Rts2Daemon::~Rts2Daemon (void)
{
  savedValues.clear ();
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
      daemonize = DONT_DAEMONIZE;
      break;
    case OPT_LOCALPORT:
      setPort (atoi (optarg));
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
  mode_t old_mask = umask (022);
  lockf = open (lock_fname, O_RDWR | O_CREAT, 0666);
  umask (old_mask);
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
  if (daemonize != DO_DAEMONIZE)
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
  daemonize = IS_DAEMONIZED;
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
    {
      logStream (MESSAGE_ERROR) << "Rts2Daemon::init Rts2block returns " <<
	ret << sendLog;
      return ret;
    }

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
  while (!getEndLoop ())
    {
      oneRunLoop ();
    }
  return 0;
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
    case IS_DAEMONIZED:
    case DO_DAEMONIZE:
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
      if (daemonize == IS_DAEMONIZED)
	break;
    case DONT_DAEMONIZE:
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
  if (daemonize == IS_DAEMONIZED)
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
      daemonize = IS_DAEMONIZED;
      logStream (MESSAGE_WARNING) << "connection to centrald lost" << sendLog;
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
Rts2Daemon::saveValue (Rts2CondValue * val)
{
  Rts2Value *old_value = duplicateValue (val->getValue (), true);
  savedValues.push_back (old_value);
  val->setValueSave ();
}

void
Rts2Daemon::deleteSaveValue (Rts2CondValue * val)
{
  for (Rts2ValueVector::iterator iter = savedValues.begin ();
       iter != savedValues.end (); iter++)
    {
      Rts2Value *new_val = *iter;
      if (new_val->isValue (val->getValue ()->getName ().c_str ()))
	{
	  savedValues.erase (iter);
	  return;
	}
    }
}

void
Rts2Daemon::loadValues ()
{
  int ret;
  for (Rts2ValueVector::iterator iter = savedValues.begin ();
       iter != savedValues.end ();)
    {
      Rts2Value *new_val = *iter;
      Rts2CondValue *old_val = getValue (new_val->getName ().c_str ());
      if (old_val == NULL)
	{
	  logStream (MESSAGE_ERROR) <<
	    "Rts2Daemon::loadValues cannot get value " << new_val->
	    getName () << sendLog;
	}
      // we don't need to save this one
      else if (old_val->ignoreLoad ())
	{
	  // just reset it..
	  old_val->clearIgnoreSave ();
	}
      // if there was error setting value
      else
	{
	  ret = setValue (old_val, '=', new_val);
	  if (ret == -2)
	    {
	      logStream (MESSAGE_ERROR) <<
		"Rts2Daemon::loadValues cannot set value " << new_val->
		getName () << sendLog;
	    }
	  // if value change is qued, inform value, that it should delete after next load..
	  if (ret == 0)
	    {
	      old_val->clearValueSave ();
	      // this will put to iter next value..
	      iter = savedValues.erase (iter);
	      continue;
	    }
	  else
	    {
	      old_val->loadFromQue ();
	    }
	}
      iter++;
    }
}

void
Rts2Daemon::addValue (Rts2Value * value, int queCondition, bool save_value)
{
  values.push_back (new Rts2CondValue (value, queCondition, save_value));
}

Rts2CondValue *
Rts2Daemon::getValue (const char *v_name)
{
  Rts2CondValueVector::iterator iter;
  for (iter = values.begin (); iter != values.end (); iter++)
    {
      Rts2CondValue *val = *iter;
      if (val->getValue ()->isValue (v_name))
	return val;
    }
  return NULL;
}

Rts2Value *
Rts2Daemon::duplicateValue (Rts2Value * old_value, bool withVal)
{
  // create new value, which will be passed to hook
  Rts2Value *dup_val;
  switch (old_value->getValueType ())
    {
    case RTS2_VALUE_STRING:
      dup_val = new Rts2ValueString (old_value->getName (),
				     old_value->getDescription (),
				     old_value->getWriteToFits ());
      if (withVal)
	((Rts2ValueString *) dup_val)->setValueString (old_value->
						       getValue ());
      break;
    case RTS2_VALUE_INTEGER:
      dup_val = new Rts2ValueInteger (old_value->getName (),
				      old_value->getDescription (),
				      old_value->getWriteToFits ());
      if (withVal)
	((Rts2ValueInteger *) dup_val)->setValueInteger (old_value->
							 getValueInteger ());
      break;
    case RTS2_VALUE_TIME:
      dup_val = new Rts2ValueTime (old_value->getName (),
				   old_value->getDescription (),
				   old_value->getWriteToFits ());
      if (withVal)
	((Rts2ValueTime *) dup_val)->setValueDouble (old_value->
						     getValueDouble ());
      break;
    case RTS2_VALUE_DOUBLE:
      dup_val = new Rts2ValueDouble (old_value->getName (),
				     old_value->getDescription (),
				     old_value->getWriteToFits ());
      if (withVal)
	((Rts2ValueDouble *) dup_val)->setValueDouble (old_value->
						       getValueDouble ());
      break;
    case RTS2_VALUE_FLOAT:
      dup_val = new Rts2ValueFloat (old_value->getName (),
				    old_value->getDescription (),
				    old_value->getWriteToFits ());
      if (withVal)
	((Rts2ValueFloat *) dup_val)->setValueFloat (old_value->
						     getValueFloat ());
      break;
    case RTS2_VALUE_BOOL:
      dup_val = new Rts2ValueBool (old_value->getName (),
				   old_value->getDescription (),
				   old_value->getWriteToFits ());
      if (withVal)
	((Rts2ValueBool *) dup_val)->
	  setValueBool (((Rts2ValueBool *) old_value)->getValueBool ());
      break;
    case RTS2_VALUE_SELECTION:
      dup_val = new Rts2ValueSelection (old_value->getName (),
					old_value->getDescription (),
					old_value->getWriteToFits ());
      ((Rts2ValueSelection *) dup_val)->
	copySel ((Rts2ValueSelection *) old_value);
      if (withVal)
	((Rts2ValueInteger *) dup_val)->
	  setValueInteger (old_value->getValueInteger ());
      break;
    default:
      logStream (MESSAGE_ERROR) << "unknow value type: " << old_value->
	getValueType () << sendLog;
      return NULL;
    }
  return dup_val;
}

void
Rts2Daemon::addConstValue (Rts2Value * value)
{
  constValues.push_back (value);
}

void
Rts2Daemon::addBopValue (Rts2Value * value)
{
  bopValues.push_back (value);
  // create status mask and send it..
  checkBopStatus ();
}

void
Rts2Daemon::removeBopValue (Rts2Value * value)
{
  for (Rts2ValueVector::iterator iter = bopValues.begin ();
       iter != bopValues.end ();)
    {
      if (*iter == value)
	iter = bopValues.erase (iter);
      else
	iter++;
    }
  checkBopStatus ();
}

void
Rts2Daemon::checkBopStatus ()
{
  int new_state = getState ();
  for (Rts2ValueVector::iterator iter = bopValues.begin ();
       iter != bopValues.end (); iter++)
    {
      Rts2Value *val = *iter;
      new_state |= val->getBopMask ();
    }
  setState (new_state, "changed due to bop");
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
Rts2Daemon::setValue (Rts2CondValue * old_value_cond, char op,
		      Rts2Value * new_value)
{
  // que change if that's necessary
  if (queValueChange (old_value_cond))
    {
      queValues.push_back (new Rts2ValueQue (old_value_cond, op, new_value));
      return -1;
    }

  return doSetValue (old_value_cond, op, new_value);
}

int
Rts2Daemon::doSetValue (Rts2CondValue * old_cond_value, char op,
			Rts2Value * new_value)
{
  int ret;

  Rts2Value *old_value = old_cond_value->getValue ();

  // save values before first change
  if (old_cond_value->needSaveValue ())
    saveValue (old_cond_value);

  ret = new_value->doOpValue (op, old_value);
  if (ret)
    goto err;

  // call hook
  ret = setValue (old_value, new_value);
  if (ret)
    goto err;

  // set value after sucessfull return..
  old_value->setFromValue (new_value);

  // if in previous step we put ignore load, reset it now
  if (old_cond_value->ignoreLoad ())
    {
      saveValue (old_cond_value);
      old_cond_value->clearIgnoreSave ();
    }

  // if the previous one was postponed, clear that flag..
  if (old_cond_value->loadedFromQue ())
    {
      old_cond_value->clearLoadedFromQue ();
      deleteSaveValue (old_cond_value);
    }
  else
    {
      delete new_value;
    }

  sendValueAll (old_value);

  return 0;
err:
  delete new_value;
  return ret;
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
  for (Rts2ValueVector::iterator iter = constValues.begin ();
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
  for (Rts2CondValueVector::iterator iter = values.begin ();
       iter != values.end (); iter++)
    {
      Rts2Value *val = (*iter)->getValue ();
      int ret;
      ret = val->sendInfo (conn);
      if (ret)
	return ret;
    }
  return 0;
}

void
Rts2Daemon::sendValueAll (Rts2Value * value)
{
  connections_t::iterator iter;
  for (iter = connectionBegin (); iter != connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      value->sendInfo (conn);
    }
}

int
Rts2Daemon::sendMetaInfo (Rts2Conn * conn)
{
  for (Rts2ValueVector::iterator iter = constValues.begin ();
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
  for (Rts2CondValueVector::iterator iter = values.begin ();
       iter != values.end (); iter++)
    {
      Rts2Value *val = (*iter)->getValue ();
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
Rts2Daemon::setValue (Rts2Conn * conn, bool overwriteSaved)
{
  char *v_name;
  char *op;
  int ret;
  if (conn->paramNextString (&v_name) || conn->paramNextString (&op))
    return -2;
  Rts2CondValue *old_value_cond = getValue (v_name);
  if (!old_value_cond)
    return -2;
  Rts2Value *old_value = old_value_cond->getValue ();
  if (!old_value)
    return -2;
  Rts2Value *newValue;

  newValue = duplicateValue (old_value);

  if (newValue == NULL)
    return -2;

  ret = newValue->setValue (conn);
  if (ret)
    goto err;

  if (overwriteSaved)
    {
      old_value_cond->setIgnoreSave ();
      deleteSaveValue (old_value_cond);
    }

  ret = setValue (old_value_cond, *op, newValue);

  // value change was qued
  if (ret == -1)
    {
      conn->sendCommandEnd (DEVDEM_I_QUED, "value change was qued");
    }
  return ret;

err:
  delete newValue;
  return ret;
}

void
Rts2Daemon::setState (int new_state, const char *description)
{
  if (state == new_state)
    return;
  stateChanged (new_state, state, description);
}

void
Rts2Daemon::stateChanged (int new_state, int old_state,
			  const char *description)
{
  state = new_state;
}

void
Rts2Daemon::maskState (int state_mask, int new_state, const char *description)
{
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) <<
    "Rts2Device::maskState state: " << " state_mask: " <<
    state_mask << " new_state: " << new_state << " desc: " << description <<
    sendLog;
#endif
  int masked_state = state;
  // null from state all errors..
  masked_state &= ~(DEVICE_ERROR_MASK | state_mask);
  masked_state |= new_state;
  setState (masked_state, description);
}
