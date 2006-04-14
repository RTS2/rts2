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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define DEBUG

#include "riseset.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <malloc.h>
#include <libnova/libnova.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "../utils/rts2block.h"
#include "../utils/rts2config.h"
#include "status.h"

#define PORT	617

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX		255
#endif

class Rts2ConnCentrald;

class Rts2Centrald:public Rts2Block
{
  int priority_client;
  int current_state;

  int next_event_type;
  time_t next_event_time;
  struct ln_lnlat_posn *observer;

  int morning_off;
  int morning_standby;

protected:
  int changeState (int new_state);
  int idle ();

  virtual int processOption (int in_opt);

  // those callbacks are now empty; they can be use in future
  // to link two centrald to enable cooperative observation
  virtual Rts2Conn *createClientConnection (char *in_deviceName)
  {
    return NULL;
  }
  virtual Rts2Conn *createClientConnection (Rts2Address * in_addr)
  {
    return NULL;
  }

public:
  Rts2Centrald (int in_argc, char **in_argv);

  int init ();
  int changePriority (time_t timeout);

  int changeStateOn ()
  {
    return changeState ((next_event_type + 5) % 6);
  }
  int changeStateStandby ()
  {
    return changeState (SERVERD_STANDBY | ((next_event_type + 5) % 6));
  }
  int changeStateOff ()
  {
    return changeState (SERVERD_OFF);
  }
  inline int getState ()
  {
    return current_state;
  }
  inline int getPriorityClient ()
  {
    return priority_client;
  }

  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
  void connAdded (Rts2ConnCentrald * added);
};

class Rts2ConnCentrald:public Rts2Conn
{
private:
  int authorized;
  char status_txt[MAX_STATUS_TXT];
  char login[MAX_STATUS_TXT];
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
public:
    Rts2ConnCentrald (int in_sock, Rts2Centrald * in_master,
		      int in_centrald_id);
    virtual ~ Rts2ConnCentrald (void);
  int sendInfo (Rts2Conn * conn);
};

Rts2ConnCentrald::Rts2ConnCentrald (int in_sock, Rts2Centrald * in_master,
				    int in_centrald_id):
Rts2Conn (in_sock, in_master)
{
  master = in_master;
  setCentraldId (in_centrald_id);
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
Rts2ConnCentrald::sendInfo ()
{
  int i;

  if (!paramEnd ())
    return -2;

  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = master->connections[i];
      if (conn)
	{
	  conn->sendInfo (this);
	}
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
      asprintf (&msg, "user %i %i %c %s %s",
		getCentraldId (),
		getPriority (),
		havePriority ()? '*' : '-', login, status_txt);
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

      if (client >= MAX_CONN || client < 0)
	{
	  return -2;
	}

      // client wanished when we processed data..
      if (master->connections[client] == NULL)
	return -1;

      if (master->connections[client]->getKey () == 0)
	{
	  sendAValue ("authorization_failed", client);
	  sendCommandEnd (DEVDEM_E_SYSTEM,
			  "client didn't ask for authorization");
	  return -1;
	}

      if (master->connections[client]->getKey () != dev_key)
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
      return master->changeStateOn ();
    }
  if (isCommand ("priority") || isCommand ("prioritydeferred"))
    {
      return priorityCommand ();
    }
  if (isCommand ("standby"))
    {
      return master->changeStateStandby ();
    }
  if (isCommand ("off"))
    {
      return master->changeStateOff ();
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

  ret = send ("I status_num 1");
  if (ret)
    return ret;
  asprintf (&msg, "I status 0 %s %i", SERVER_STATUS, master->getState ());
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2ConnCentrald::sendAValue (char *val_name, int value)
{
  char *msg;
  int ret;
  asprintf (&msg, "A %s %i", val_name, value);
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
      if (isCommand ("status_txt"))
	{
	  char *new_st;
	  if (paramNextString (&new_st) || !paramEnd ())
	    return -1;
	  strncpy (status_txt, new_st, MAX_STATUS_TXT - 1);
	  status_txt[MAX_STATUS_TXT - 1] = 0;
	  return 0;
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
	  return master->changeStateOn ();
	}
      if (isCommand ("standby"))
	{
	  return master->changeStateStandby ();
	}
      if (isCommand ("off"))
	{
	  return master->changeStateOff ();
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
	      asprintf (&msg, "M priority_change %i %i",
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
  else if (getType () == DEVICE_SERVER)
    return commandDevice ();
  else if (getType () == CLIENT_SERVER)
    return commandClient ();
  else
    {
      return Rts2Conn::command ();
    }
  return 0;
}

Rts2Centrald::Rts2Centrald (int in_argc, char **in_argv):Rts2Block (in_argc,
	   in_argv)
{
  Rts2Config *
    config = Rts2Config::instance ();
  config->loadFile ();
  observer = config->getObserver ();

  current_state =
    config->getBoolean ("centrald", "reboot_on") ? 0 : SERVERD_OFF;

  morning_off = config->getBoolean ("centrald", "morning_off");
  morning_standby = config->getBoolean ("centrald", "morning_standby");

  addOption ('p', "port", 1, "port on which centrald will listen");
}

int
Rts2Centrald::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'p':
      setPort (atoi (optarg));
      break;
    default:
      return Rts2Block::processOption (in_opt);
    }
  return 0;
}

int
Rts2Centrald::init ()
{
  setPort (PORT);
  return Rts2Block::init ();
}

Rts2Conn *
Rts2Centrald::createConnection (int in_sock, int conn_num)
{
  return new Rts2ConnCentrald (in_sock, this, conn_num);
}

void
Rts2Centrald::connAdded (Rts2ConnCentrald * added)
{
  for (int i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn;
      conn = connections[i];
      if (conn)
	added->sendInfo (conn);
    }
}

/*!
 * @param new_state		new state, if -1 -> 3
 */
int
Rts2Centrald::changeState (int new_state)
{
  current_state = new_state;
  return sendStatusMessage (SERVER_STATUS, current_state);
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
  int i;

  if (priority_client >= 0 && connections[priority_client])	// old priority client
    {
      new_priority_client = priority_client;
      new_priority_max = connections[priority_client]->getPriority ();
    }
  // find new client with highest priority
  for (i = 0; i < MAX_CONN; i++)
    {
      Rts2Conn *conn = connections[i];
      if (conn)
	syslog (LOG_DEBUG, "Rts2Centrald::changePriority priorit: %i, %i", i,
		conn->getPriority ());
      if (conn && conn->getPriority () > new_priority_max)
	{
	  new_priority_client = i;
	  new_priority_max = conn->getPriority ();
	}
    }

  if (priority_client != new_priority_client)
    {
      if (priority_client >= 0 && connections[priority_client])
	connections[priority_client]->setHavePriority (0);

      priority_client = new_priority_client;

      if (priority_client >= 0 && connections[priority_client])
	connections[priority_client]->setHavePriority (1);
    }
  return sendMessage ("priority_change", priority_client, timeout);
}

int
Rts2Centrald::idle ()
{
  time_t curr_time;

  int call_state;
  int old_current_state;

  curr_time = time (NULL);

  next_event (observer, &curr_time, &call_state, &next_event_type,
	      &next_event_time);

  if (current_state != SERVERD_OFF && current_state != call_state)
    {
      old_current_state = current_state;
      if ((current_state & SERVERD_STATUS_MASK) == SERVERD_MORNING
	  && call_state == SERVERD_DAY)
	{
	  if (morning_off)
	    current_state = SERVERD_OFF;
	  else if (morning_standby)
	    current_state = call_state | SERVERD_STANDBY;
	  else
	    current_state =
	      (current_state & SERVERD_STANDBY_MASK) | call_state;
	}
      else
	{
	  current_state = (current_state & SERVERD_STANDBY_MASK) | call_state;
	}
      if (current_state != old_current_state)
	{
	  sendStatusMessage (SERVER_STATUS, current_state);
	}
    }
  return Rts2Block::idle ();
}

int
main (int argc, char **argv)
{
  Rts2Centrald *centrald;
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
  centrald = new Rts2Centrald (argc, argv);
  centrald->init ();
  centrald->run ();

  return 0;
}
