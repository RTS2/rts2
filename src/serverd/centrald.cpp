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
#include <mcheck.h>
#include <libnova/libnova.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "../utils/rts2block.h"
#include "../utils/config.h"
#include "status.h"

#define PORT	5557

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX		255
#endif

class Rts2Centrald:public Rts2Block
{
  int priority_client;
  int current_state;

  int next_event_type;
  time_t next_event_time;
  struct ln_lnlat_posn observer;

protected:
  int changeState (int new_state);
  int idle ();

  virtual int processOption (int in_opt);
public:
    Rts2Centrald (int in_argc, char **in_argv);

  int init ();
  int changePriority (time_t timeout);

  int changeStateOn ()
  {
    return changeState ((next_event_type + 5) % 6);
  };
  int changeStateStandby ()
  {
    return changeState (SERVERD_STANDBY | ((next_event_type + 5) % 6));
  };
  int changeStateOff ()
  {
    return changeState (SERVERD_OFF);
  };
  inline int getState ()
  {
    return current_state;
  };
  inline int getPriorityClient ()
  {
    return priority_client;
  };

  virtual Rts2Conn *createConnection (int in_sock, int conn_num);
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
  int sendDeviceKey ();
  int sendInfo ();
  int sendStatusInfo ();
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
Rts2ConnCentrald::sendDeviceKey ()
{
  int key = random ();
  char *msg;
  // device number could change..device names don't
  char *dev_name;
  Rts2Conn *dev;
  int dev_id;
  if (paramNextString (&dev_name) || !paramEnd ())
    return -2;
  // find device, set it authorization key
  dev = master->findName (dev_name);
  if (!dev)
    {
      sendCommandEnd (DEVDEM_E_SYSTEM, "cannot find device with name");
      return -1;
    }
  setKey (key);
  asprintf (&msg, "authorization_key %s %i", dev_name, key);
  send (msg);
  free (msg);
  return 0;
}

int
Rts2ConnCentrald::sendInfo ()
{
  int i;
  Rts2Conn *dev;

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
      asprintf (&msg, "device %i %s %s:%i %i",
		getCentraldId (), getName (), hostname, port, device_type);
      ret = conn->send (msg);
      free (msg);
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
      int key;
      if (paramNextInteger (&client)
	  || paramNextInteger (&key) || !paramEnd ())
	return -2;

      // client wanished when we processed data..
      if (master->connections[client] == NULL)
	return -1;

      if (master->connections[client]->getKey () == 0)
	{
	  sendValue ("authorization_failed", client);
	  sendCommandEnd (DEVDEM_E_SYSTEM,
			  "client didn't ask for authorization");
	  return -1;
	}

      if (master->connections[client]->getKey () != key)
	{
	  sendValue ("authorization_failed", client);
	  sendCommandEnd (DEVDEM_E_SYSTEM, "invalid authorization key");
	  master->connections[client]->setKey (0);
	  return -1;
	}
      master->connections[client]->setKey (0);

      sendValue ("authorization_ok", client);

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
  if (isCommand ("standby"))
    {
      return master->changeStateStandby ();
    }
  if (isCommand ("off"))
    {
      return master->changeStateOff ();
    }
  if (isCommand ("S"))
    {
      syslog (LOG_DEBUG, "Rts2ConnCentrald::commandDevice Status command");
      return 0;
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
Rts2ConnCentrald::commandClient ()
{
  if (isCommand ("password"))
    {
      char *passwd;
      if (paramNextString (&passwd) || !paramEnd ())
	return -2;

      if (strncmp (passwd, login, CLIENT_LOGIN_SIZE) == 0)
	{
	  authorized = 1;
	  sendValue ("logged_as", getCentraldId ());
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

	  sendValue ("actual_priority", master->getPriorityClient (),
		     getPriority ());

	  setPriority (new_priority);

	  if (master->changePriority (timeout))
	    {
	      sendCommandEnd (DEVDEM_E_PRIORITY,
			      "error when processing priority request");
	      return -1;
	    }

	  sendValue ("new_priority", master->getPriorityClient (),
		     getPriority ());

	  return 0;
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
	  unsigned int in_port;

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
	    sendValue ("M priority_change", master->getPriorityClient (), 0);

	  asprintf (&msg, "device %i %s %s:%i %i",
		    master->getPriorityClient (), reg_device, hostname, port,
		    device_type);
	  ret = send (msg);
	  free (msg);
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
  observer.lng = get_double_default ("longtitude", 0);
  observer.lat = get_double_default ("latitude", 0);
  current_state =
    (strcmp (get_device_string_default ("centrald", "reboot_on", "N"), "Y") ?
     SERVERD_OFF : 0);

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

  if (priority_client >= 0 && connections[priority_client])
    connections[priority_client]->setHavePriority (0);

  priority_client = new_priority_client;

  if (priority_client >= 0 && connections[priority_client])
    connections[priority_client]->setHavePriority (1);

  return sendMessage ("priority_change", priority_client, timeout);
}

int
Rts2Centrald::idle ()
{
  time_t curr_time;
  struct ln_lnlat_posn obs;

  int call_state;
  int old_current_state;

  curr_time = time (NULL);

  next_event (&observer, &curr_time, &call_state, &next_event_type,
	      &next_event_time);

  if (current_state != SERVERD_OFF && current_state != call_state)
    {
      old_current_state = current_state;
      if ((current_state & SERVERD_STATUS_MASK) == SERVERD_MORNING
	  && call_state == SERVERD_DAY
	  &&
	  !strcmp (get_device_string_default
		   ("centrald", "morning_off", "N"), "Y"))
	{
	  current_state = SERVERD_OFF;
	}
      else
	{
	  current_state = (current_state & SERVERD_STANDBY_MASK) | call_state;
	}
      syslog (LOG_DEBUG, "riseset thread sleeping %li seconds for %i",
	      next_event_time - curr_time + 1, next_event_type);
      if (current_state != old_current_state)
	{
	  sendStatusMessage (SERVER_STATUS, current_state);
	}
    }
}



int
main (int argc, char **argv)
{
  int i;
  Rts2Centrald *centrald;
  mtrace ();
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
  if (read_config (CONFIG_FILE) == -1)
    syslog (LOG_ERR,
	    "Cannot open config file " CONFIG_FILE ", defaults will be used");
  else
    syslog (LOG_INFO, "Config readed from " CONFIG_FILE);
  centrald = new Rts2Centrald (argc, argv);
  centrald->init ();
  centrald->run ();

  return 0;
}
