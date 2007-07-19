/** 
 * Centrald - RTS2 coordinator
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek,net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*!
 * @file Server deamon source.
 *
 * Source for server deamon - a something between client and device,
 * what takes care of priorities and authentification.
 *
 * Contains list of clients with their id's and with their access rights.
 *
 * @author petr
 */
#include "riseset.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <libnova/libnova.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include <config.h>
#include "../utils/rts2daemon.h"
#include "../utils/rts2config.h"
#include "status.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX		255
#endif

class Rts2ConnCentrald;

class Rts2Centrald:public Rts2Daemon
{
private:
  int priority_client;

  int next_event_type;
  time_t next_event_time;
  struct ln_lnlat_posn *observer;

  int morning_off;
  int morning_standby;

  char *configFile;
    std::string logFile;
  // which sets logfile
  enum
  { LOGFILE_ARG, LOGFILE_DEF, LOGFILE_CNF } logFileSource;

    std::ofstream * fileLog;

  void openLog ();
  int reloadConfig ();

  int connNum;
protected:
  int changeState (int new_state, const char *user);

  virtual int processOption (int in_opt);

  // those callbacks are now empty; they can be used in future
  // to link two centrald to enable cooperative observation
  virtual Rts2Conn *createClientConnection (char *in_deviceName)
  {
    return NULL;
  }
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr)
  {
    return NULL;
  }

  virtual bool queValueChange (Rts2CondValue * old_value)
  {
    return false;
  }

public:
  Rts2Centrald (int in_argc, char **in_argv);
  virtual ~ Rts2Centrald (void);

  virtual int idle ();

  virtual int init ();

  int changePriority (time_t timeout);

  int changeStateOn (const char *user)
  {
    return changeState ((next_event_type + 5) % 6, user);
  }
  int changeStateStandby (const char *user)
  {
    return changeState (SERVERD_STANDBY | ((next_event_type + 5) % 6), user);
  }
  int changeStateOff (const char *user)
  {
    return changeState (SERVERD_OFF, user);
  }
  inline int getPriorityClient ()
  {
    return priority_client;
  }

  virtual Rts2Conn *createConnection (int in_sock);
  void connAdded (Rts2ConnCentrald * added);
  Rts2Conn *getConnection (int conn_num);

  void sendMessage (messageType_t in_messageType,
		    const char *in_messageString);

  virtual void message (Rts2Message & msg);
  void processMessage (Rts2Message & msg)
  {
    sendMessageAll (msg);
  }

  virtual void sigHUP (int sig);

  void bopMaskChanged (Rts2ConnCentrald * conn);
};

class Rts2ConnCentrald:public Rts2Conn
{
private:
  int authorized;
  char login[CLIENT_LOGIN_SIZE];
  Rts2Centrald *master;
  char hostname[HOST_NAME_MAX];
  int port;
  int device_type;

  int command ();
  int commandDevice ();
  int commandClient ();
  // command handling functions
  int priorityCommand ();
  int sendDeviceKey ();
  int sendInfo ();
  int sendStatusInfo ();
  int sendAValue (char *name, int value);
  int messageMask;
protected:
    virtual void setState (int in_value);
public:
    Rts2ConnCentrald (int in_sock, Rts2Centrald * in_master,
		      int in_centrald_id);
    virtual ~ Rts2ConnCentrald (void);
  virtual int sendMessage (Rts2Message & msg);
  virtual int sendInfo (Rts2Conn * conn);
};

void
Rts2ConnCentrald::setState (int in_value)
{
  Rts2Conn::setState (in_value);
  if (serverState->maskValueChanged (BOP_MASK))
    {
      master->bopMaskChanged (this);
    }
}

Rts2ConnCentrald::Rts2ConnCentrald (int in_sock, Rts2Centrald * in_master,
				    int in_centrald_id):
Rts2Conn (in_sock, in_master)
{
  master = in_master;
  setCentraldId (in_centrald_id);
  messageMask = 0x00;
}

/*!
 * Called on connection exit.
 *
 * Delete client|device login|name, updates priorities, detach shared
 * memory.
 */
Rts2ConnCentrald::~Rts2ConnCentrald (void)
{
  setPriority (-1);
  master->changePriority (0);
}

int
Rts2ConnCentrald::priorityCommand ()
{
  int timeout;
  int new_priority;

  if (isCommand ("priority"))
    {
      if (paramNextInteger (&new_priority) || !paramEnd ())
	return -1;
      timeout = 0;
    }
  else
    {
      if (paramNextInteger (&new_priority) ||
	  paramNextInteger (&timeout) || !paramEnd ())
	return -1;
      timeout += time (NULL);
    }

  sendValue ("old_priority", getPriority (), getCentraldId ());

  sendValue ("actual_priority", master->getPriorityClient (), getPriority ());

  setPriority (new_priority);

  if (master->changePriority (timeout))
    {
      sendCommandEnd (DEVDEM_E_PRIORITY,
		      "error when processing priority request");
      return -1;
    }

  sendValue ("new_priority", master->getPriorityClient (), getPriority ());

  return 0;
}

int
Rts2ConnCentrald::sendDeviceKey ()
{
  int dev_key = random ();
  char *msg;
  // device number could change..device names don't
  char *dev_name;
  Rts2Conn *dev;
  if (paramNextString (&dev_name) || !paramEnd ())
    return -2;
  // find device, set it authorization key
  dev = master->findName (dev_name);
  if (!dev)
    {
      sendCommandEnd (DEVDEM_E_SYSTEM, "cannot find device with name");
      return -1;
    }
  setKey (dev_key);
  asprintf (&msg, "authorization_key %s %i", dev_name, getKey ());
  send (msg);
  free (msg);
  return 0;
}

int
Rts2ConnCentrald::sendMessage (Rts2Message & msg)
{
  if (msg.passMask (messageMask))
    return Rts2Conn::sendMessage (msg);
  return -1;
}

int
Rts2ConnCentrald::sendInfo ()
{
  if (!paramEnd ())
    return -2;

  connections_t::iterator iter;
  for (iter = master->connectionBegin ();
       iter != master->connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      conn->sendInfo (this);
    }
  return 0;
}

int
Rts2ConnCentrald::sendInfo (Rts2Conn * conn)
{
  char *msg;
  int ret = -1;

  switch (getType ())
    {
    case CLIENT_SERVER:
      asprintf (&msg, "user %i %i %c %s",
		getCentraldId (),
		getPriority (), havePriority ()? '*' : '-', login);
      ret = conn->send (msg);
      free (msg);
      break;
    case DEVICE_SERVER:
      asprintf (&msg, "device %i %s %s %i %i",
		getCentraldId (), getName (), hostname, port, device_type);
      ret = conn->send (msg);
      free (msg);
      break;
    default:
      break;
    }
  return ret;
}

int
Rts2ConnCentrald::commandDevice ()
{
  if (isCommand ("authorize"))
    {
      int client;
      int dev_key;
      if (paramNextInteger (&client)
	  || paramNextInteger (&dev_key) || !paramEnd ())
	return -2;

      if (client < 0)
	{
	  return -2;
	}

      Rts2Conn *conn = master->getConnection (client);

      // client vanished when we processed data..
      if (conn == NULL)
	return -1;

      if (conn->getKey () == 0)
	{
	  sendAValue ("authorization_failed", client);
	  sendCommandEnd (DEVDEM_E_SYSTEM,
			  "client didn't ask for authorization");
	  return -1;
	}

      if (conn->getKey () != dev_key)
	{
	  sendAValue ("authorization_failed", client);
	  sendCommandEnd (DEVDEM_E_SYSTEM, "invalid authorization key");
	  return -1;
	}

      sendAValue ("authorization_ok", client);
      sendInfo ();

      return 0;
    }
  if (isCommand ("key"))
    {
      return sendDeviceKey ();
    }
  if (isCommand ("info"))
    {
      return sendInfo ();
    }
  if (isCommand ("on"))
    {
      return master->changeStateOn (getName ());
    }
  if (isCommand ("priority") || isCommand ("prioritydeferred"))
    {
      return priorityCommand ();
    }
  if (isCommand ("standby"))
    {
      return master->changeStateStandby (getName ());
    }
  if (isCommand ("off"))
    {
      return master->changeStateOff (getName ());
    }
  return -1;
}

/*!
 * Print standart status header.
 *
 * It needs to be called after establishing of every new connection.
 */
int
Rts2ConnCentrald::sendStatusInfo ()
{
  char *msg;
  int ret;

  asprintf (&msg, PROTO_INFO " %i", master->getState ());
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2ConnCentrald::sendAValue (char *val_name, int value)
{
  char *msg;
  int ret;
  asprintf (&msg, PROTO_AUTH " %s %i", val_name, value);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2ConnCentrald::commandClient ()
{
  if (isCommand ("password"))
    {
      char *passwd;
      if (paramNextString (&passwd) || !paramEnd ())
	return -2;

      if (strncmp (passwd, login, CLIENT_LOGIN_SIZE) == 0)
	{
	  char *msg;
	  authorized = 1;
	  asprintf (&msg, "logged_as %i", getCentraldId ());
	  send (msg);
	  free (msg);
	  sendStatusInfo ();
	  return 0;
	}
      else
	{
	  sleep (5);		// wait some time to prevent repeat attack
	  sendCommandEnd (DEVDEM_E_SYSTEM, "invalid login or password");
	  return -1;
	}
    }
  if (authorized)
    {
      if (isCommand ("ready"))
	{
	  return 0;
	}

      if (isCommand ("info"))
	{
	  return sendInfo ();
	}
      if (isCommand ("priority") || isCommand ("prioritydeferred"))
	{
	  return priorityCommand ();
	}
      if (isCommand ("key"))
	{
	  return sendDeviceKey ();
	}
      if (isCommand ("on"))
	{
	  return master->changeStateOn (login);
	}
      if (isCommand ("standby"))
	{
	  return master->changeStateStandby (login);
	}
      if (isCommand ("off"))
	{
	  return master->changeStateOff (login);
	}
    }
  return Rts2Conn::command ();
}

/*! 
 * Handle serverd commands.
 *
 * @return -2 on exit, -1 and set errno on HW failure, 0 otherwise
 */
int
Rts2ConnCentrald::command ()
{
  if (isCommand ("login"))
    {
      if (getType () == NOT_DEFINED_SERVER)
	{
	  char *in_login;

	  srandom (time (NULL));

	  if (paramNextString (&in_login) || !paramEnd ())
	    return -2;

	  strncpy (login, in_login, CLIENT_LOGIN_SIZE);

	  setType (CLIENT_SERVER);
	  master->connAdded (this);
	  return 0;
	}
      else
	{
	  sendCommandEnd (DEVDEM_E_COMMAND,
			  "cannot switch server type to CLIENT_SERVER");
	  return -1;
	}
    }
  else if (isCommand ("register"))
    {
      if (getType () == NOT_DEFINED_SERVER)
	{
	  char *reg_device;
	  char *in_hostname;
	  char *msg;
	  int ret;

	  if (paramNextString (&reg_device) || paramNextInteger (&device_type)
	      || paramNextString (&in_hostname) || paramNextInteger (&port)
	      || !paramEnd ())
	    return -2;

	  if (master->findName (reg_device))
	    {
	      sendCommandEnd (DEVDEM_E_SYSTEM, "name already registered");
	      return -1;
	    }

	  setName (reg_device);
	  strncpy (hostname, in_hostname, HOST_NAME_MAX);

	  setType (DEVICE_SERVER);
	  sendStatusInfo ();
	  if (master->getPriorityClient () > -1)
	    {
	      asprintf (&msg, PROTO_PRIORITY " %i %i",
			master->getPriorityClient (), 0);
	      send (msg);
	      free (msg);
	    }

	  asprintf (&msg, "device %i %s %s %i %i",
		    master->getPriorityClient (), reg_device, hostname, port,
		    device_type);
	  ret = send (msg);
	  free (msg);
	  sendAValue ("registered_as", getCentraldId ());
	  master->connAdded (this);
	  sendInfo ();
	  return ret;
	}
      else
	{
	  sendCommandEnd (DEVDEM_E_COMMAND,
			  "cannot switch server type to DEVICE_SERVER");
	  return -1;
	}
    }
  else if (isCommand ("message_mask"))
    {
      int newMask;
      if (paramNextInteger (&newMask) || !paramEnd ())
	return -2;
      messageMask = newMask;
      return 0;
    }
  else if (getType () == DEVICE_SERVER)
    return commandDevice ();
  else if (getType () == CLIENT_SERVER)
    return commandClient ();
  // else
  return Rts2Conn::command ();
}

Rts2Centrald::Rts2Centrald (int in_argc, char **in_argv):Rts2Daemon (in_argc,
	    in_argv)
{
  connNum = 0;

  setState (SERVERD_OFF, "Initial configuration");

  configFile = NULL;
  logFileSource = LOGFILE_DEF;
  fileLog = NULL;

  addOption (OPT_CONFIG, "config", 1, "configuration file");
  addOption (OPT_PORT, "port", 1, "port on which centrald will listen");
  addOption (OPT_LOGFILE, "logfile", 1, "log file (put '-' to log to stderr");
}

Rts2Centrald::~Rts2Centrald (void)
{
  fileLog->close ();
  delete fileLog;
}

void
Rts2Centrald::openLog ()
{
  if (fileLog)
    {
      fileLog->close ();
      delete fileLog;
    }
  if (logFile == std::string ("-"))
    {
      fileLog = NULL;
      return;
    }
  fileLog = new std::ofstream ();
  fileLog->open (logFile.c_str (), std::ios_base::out | std::ios_base::app);
}

int
Rts2Centrald::reloadConfig ()
{
  int ret;
  Rts2Config *config = Rts2Config::instance ();
  ret = config->loadFile (configFile);
  if (ret)
    return ret;

  if (logFileSource != LOGFILE_ARG)
    {
      ret = config->getString ("centrald", "logfile", logFile);
      if (ret)
	{
	  logFileSource = LOGFILE_DEF;
	  logFile = "/var/log/rts2-debug";
	}
      else
	{
	  logFileSource = LOGFILE_CNF;
	}
    }

  openLog ();

  observer = config->getObserver ();

  morning_off = config->getBoolean ("centrald", "morning_off");
  morning_standby = config->getBoolean ("centrald", "morning_standby");

  next_event_time = 0;

  return 0;
}

int
Rts2Centrald::processOption (int in_opt)
{
  switch (in_opt)
    {
    case OPT_CONFIG:
      configFile = optarg;
      break;
    case OPT_PORT:
      setPort (atoi (optarg));
      break;
    case OPT_LOGFILE:
      logFile = std::string (optarg);
      logFileSource = LOGFILE_ARG;
      break;
    default:
      return Rts2Daemon::processOption (in_opt);
    }
  return 0;
}

int
Rts2Centrald::init ()
{
  int ret;
  setPort (atoi (CENTRALD_PORT));
  ret = Rts2Daemon::init ();
  if (ret)
    return ret;

  ret = reloadConfig ();
  if (ret)
    return ret;

  Rts2Config *config = Rts2Config::instance ();

  setState (config->getBoolean ("centrald", "reboot_on") ? 0 : SERVERD_OFF,
	    "init");

  centraldConnRunning ();
  ret = checkLockFile (LOCK_PREFIX "centrald");
  if (ret)
    return ret;
  ret = doDeamonize ();
  if (ret)
    return ret;
  return lockFile ();
}

Rts2Conn *
Rts2Centrald::createConnection (int in_sock)
{
  connNum++;
  while (getConnection (connNum))
    connNum++;
  return new Rts2ConnCentrald (in_sock, this, connNum);
}

void
Rts2Centrald::connAdded (Rts2ConnCentrald * added)
{
  connections_t::iterator iter;
  for (iter = connectionBegin (); iter != connectionEnd (); iter++)
    {
      Rts2Conn *conn = *iter;
      added->sendInfo (conn);
    }
}

Rts2Conn *
Rts2Centrald::getConnection (int conn_num)
{
  connections_t::iterator iter;
  for (iter = connectionBegin (); iter != connectionEnd (); iter++)
    {
      Rts2ConnCentrald *conn = (Rts2ConnCentrald *) * iter;
      if (conn->getCentraldId () == conn_num)
	return conn;
    }
  return NULL;
}

/*!
 * @param new_state		new state, if -1 -> 3
 */
int
Rts2Centrald::changeState (int new_state, const char *user)
{
  logStream (MESSAGE_INFO) << "State switched to " << new_state << " by " <<
    user << sendLog;
  setState (new_state, user);
  return sendStatusMessage (getState ());
}

/*!
 * Made priority update, distribute messages to devices
 * about priority update.
 *
 * @param timeout	time to wait for priority change.. 
 *
 * @return 0 on success, -1 and set errno otherwise
 */
int
Rts2Centrald::changePriority (time_t timeout)
{
  int new_priority_client = -1;
  int new_priority_max = 0;
  connections_t::iterator iter;
  Rts2Conn *conn = getConnection (priority_client);

  if (priority_client >= 0 && conn)	// old priority client
    {
      new_priority_client = priority_client;
      new_priority_max = conn->getPriority ();
    }
  // find new client with highest priority
  for (iter = connectionBegin (); iter != connectionEnd (); iter++)
    {
      conn = *iter;
      if (conn->getPriority () > new_priority_max)
	{
	  new_priority_client = conn->getCentraldId ();
	  new_priority_max = conn->getPriority ();
	}
    }

  if (priority_client != new_priority_client)
    {
      conn = getConnection (priority_client);
      if (priority_client >= 0 && conn)
	conn->setHavePriority (0);

      priority_client = new_priority_client;

      conn = getConnection (priority_client);
      if (priority_client >= 0 && conn)
	conn->setHavePriority (1);
    }
  return sendPriorityChange (priority_client, timeout);
}

int
Rts2Centrald::idle ()
{
  time_t curr_time;

  int call_state;
  int old_current_state;

  if (getState () == SERVERD_OFF)
    return Rts2Daemon::idle ();

  curr_time = time (NULL);

  if (curr_time < next_event_time)
    return Rts2Daemon::idle ();

  next_event (observer, &curr_time, &call_state, &next_event_type,
	      &next_event_time);

  if (getState () != call_state)
    {
      old_current_state = getState ();
      if ((getState () & SERVERD_STATUS_MASK) == SERVERD_MORNING
	  && call_state == SERVERD_DAY)
	{
	  if (morning_off)
	    setState (SERVERD_OFF, "by idle routine");
	  else if (morning_standby)
	    setState (call_state | SERVERD_STANDBY, "by idle routine");
	  else
	    setState ((getState () & SERVERD_STANDBY_MASK) | call_state,
		      "by idle routine");
	}
      else
	{
	  setState ((getState () & SERVERD_STANDBY_MASK) | call_state,
		    "by idle routine");
	}

      // distribute new state
      if (getState () != old_current_state)
	{
	  logStream (MESSAGE_INFO) << "changed state from " <<
	    old_current_state << " to " << getState () << sendLog;
	  sendStatusMessage (getState ());
	}
    }
  return Rts2Daemon::idle ();
}

void
Rts2Centrald::sendMessage (messageType_t in_messageType,
			   const char *in_messageString)
{
  Rts2Message msg =
    Rts2Message ("centrald", in_messageType, in_messageString);
  Rts2Daemon::sendMessage (in_messageType, in_messageString);
  sendMessageAll (msg);
}

void
Rts2Centrald::message (Rts2Message & msg)
{
  processMessage (msg);
  if (fileLog)
    {
      (*fileLog) << msg;
    }
  else
    {
      std::cerr << msg.toString () << std::endl;
    }
}

void
Rts2Centrald::sigHUP (int sig)
{
  reloadConfig ();
  idle ();
}

void
Rts2Centrald::bopMaskChanged (Rts2ConnCentrald * conn)
{
  int bopState = 0;
  for (connections_t::iterator iter = connectionBegin ();
       iter != connectionEnd (); iter++)
    {
      bopState |= (*iter)->getBopState ();
    }
  maskState (BOP_MASK, bopState, "changed BOP state");
}

int
main (int argc, char **argv)
{
  Rts2Centrald centrald = Rts2Centrald (argc, argv);
  return centrald.run ();
}
