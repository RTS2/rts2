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

#define _GNU_SOURCE

#include "riseset.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <libnova.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "../utils/devser.h"
#include "../utils/location.h"
#include "status.h"

#define PORT	5557

#define NOT_DEFINED_SERVER	0
#define CLIENT_SERVER		1
#define DEVICE_SERVER		2

int msgid;			// id of server message que

struct device
{
  pid_t pid;
  char name[DEVICE_NAME_SIZE];
  char hostname[DEVICE_URI_SIZE];
  uint16_t port;
  int type;
  int authorizations[MAX_CLIENT];
};

struct client
{
  pid_t pid;
  char login[CLIENT_LOGIN_SIZE];
  int authorized;
  int priority;
  char status_txt[MAX_STATUS_TXT];
};

//! information about serverd stored in shared mem
struct serverd_info
{
  int priority_client;
  int last_client;		// last client id

  int current_state;

  int next_event_type;
  time_t next_event_time;

  struct device devices[MAX_DEVICE];
  struct client clients[MAX_CLIENT];
};

//! global server type
int server_type = NOT_DEFINED_SERVER;
int serverd_id = -1;

//! pointers to shared memory
struct serverd_info *shm_info;
struct device *shm_devices;
struct client *shm_clients;

/*!
 * Find one device.
 * 
 * @param name		device name
 * @param id		device id; if NULL, device id is not returned
 * @param device	device
 *
 * @return 0 and set pointer do device on success, -1 and set errno on error
 */
int
find_device (char *name, int *id, struct device **device)
{
  int i;
  for (i = 0; i < MAX_DEVICE; i++)
    if (strcmp (shm_devices[i].name, name) == 0)
      {
	*device = &shm_devices[i];
	if (id)
	  *id = i;
	return 0;
      }
  errno = ENODEV;
  return -1;
}

/*!
 * Send IPC message to all clients.
 *
 * @param format	message format
 * @param ...		format arguments
 */
int
clients_all_msg_snd (char *format, ...)
{
  va_list ap;
  int i;
  va_start (ap, format);
  for (i = 0; i < MAX_CLIENT; i++)
    {
      /* client exists */
      if (*shm_clients[i].login)
	{
	  if (devser_2devser_message_va (i, format, ap))
	    return -1;
	}
    }
  va_end (ap);
  return 0;
}

/*!
 * Send IPC message to one device.
 *
 * @param name		device name
 * @param format	message format
 * @param ...		format arguments
 *
 * @execption ENODEV	device don't exists
 *
 * @return 0 on success, -1 on error, set errno
 */
int
device_msg_snd (char *name, char *format, ...)
{
  struct device *device;
  int id;
  if (find_device (name, &id, &device))
    {
      int ret;
      va_list ap;

      va_start (ap, format);
      ret = devser_2devser_message_va (id + MAX_CLIENT + 1, format, ap);
      va_end (ap);
    }
  errno = ENODEV;
  return -1;
}

/*!
 * Send IPC messsages to all registered devices.
 *
 * @param format	message format
 * @param ...		format arguments
 * 
 * @return 0 on success, -1 and set errno on any error
 */
int
devices_all_msg_snd (char *format, ...)
{
  va_list ap;
  int i;
  va_start (ap, format);
  for (i = 0; i < MAX_DEVICE; i++)
    {
      /* device exists */
      if (*shm_devices[i].name)
	{
	  if (devser_2devser_message_va (i + MAX_CLIENT, format, ap))
	    return -1;
	}
    }
  va_end (ap);
  return 0;
}

int
get_device_key ()
{
  int key = random ();
  // device number could change..device names don't
  char *dev_name;
  struct device *device;
  int dev_id;
  if (devser_param_test_length (1) || devser_param_next_string (&dev_name))
    return -1;
  // find device, set it authorization key
  if (find_device (dev_name, &dev_id, &device))
    {
      devser_write_command_end (DEVDEM_E_SYSTEM,
				"cannot find device with name %s", dev_name);
      return -1;
    }

  devser_shm_data_lock ();
  device->authorizations[serverd_id] = key;
  devser_shm_data_unlock ();

  devser_dprintf ("authorization_key %s %i", dev_name, key);
  return 0;
}

int
get_info ()
{
  int i;
  struct device *dev;

  if (devser_param_test_length (0))
    return -1;

  for (i = 0; i < MAX_CLIENT; i++)
    if (*shm_clients[i].login)
      devser_dprintf ("user %i %i %c %s %s", i,
		      shm_clients[i].priority,
		      shm_info->priority_client == i ? '*' : '-',
		      shm_clients[i].login, shm_clients[i].status_txt);

  for (i = 0, dev = shm_devices; i < MAX_DEVICE; i++, dev++)

    if (*dev->name)
      devser_dprintf ("device %i %s %s:%i %i", i, dev->name,
		      dev->hostname, dev->port, dev->type);
  return 0;
}

int
device_serverd_handle_command (char *command)
{
  if (strcmp (command, "authorize") == 0)
    {
      int client;
      int key;
      if (devser_param_test_length (2))
	return -1;
      if (devser_param_next_integer (&client)
	  || devser_param_next_integer (&key))
	return -1;

      devser_shm_data_lock ();
      if (shm_devices[serverd_id].authorizations[client] == 0)
	{
	  devser_dprintf ("authorization_failed %i", client);
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "client %i didn't ask for authorization",
				    client);
	  devser_shm_data_unlock ();
	  return -1;
	}

      if (shm_devices[serverd_id].authorizations[client] != key)
	{
	  devser_dprintf ("authorization_failed %i", client);
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "invalid authorization key: %i %i", key,
				    shm_devices[serverd_id].
				    authorizations[client]);
	  shm_devices[serverd_id].authorizations[client] = 0;
	  devser_shm_data_unlock ();
	  return -1;
	}
      shm_devices[serverd_id].authorizations[client] = 0;
      devser_shm_data_unlock ();

      devser_dprintf ("authorization_ok %i", client);

      return 0;
    }
  if (strcmp (command, "key") == 0)
    {
      return get_device_key ();
    }
  if (strcmp (command, "info") == 0)
    {
      return get_info ();
    }

  devser_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s", command);
  return -1;
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
clients_change_priority (time_t timeout)
{
  int new_priority_client = -1;
  int new_priority_max = 0;
  int i;

  if (shm_info->priority_client >= 0
      && *shm_clients[shm_info->priority_client].login)
    {
      new_priority_client = shm_info->priority_client;
      new_priority_max = shm_clients[shm_info->priority_client].priority;
      if (new_priority_max == -1)
	{
	  new_priority_client = -1;
	  new_priority_max = 0;
	}
    }

  // change priority and find new client with highest priority
  for (i = 0; i < MAX_CLIENT; i++)
    {
      if (*shm_clients[i].login && shm_clients[i].priority > new_priority_max)
	{
	  new_priority_client = i;
	  new_priority_max = shm_clients[i].priority;
	}
    }

  if (shm_info->priority_client == -1 ||
      shm_clients[shm_info->priority_client].priority < new_priority_max)
    {
      shm_info->priority_client = new_priority_client;
    }
  return devices_all_msg_snd ("M priority_change %i %i",
			      new_priority_client, timeout);
}

void *
serverd_riseset_thread (void *arg)
{
  time_t curr_time;
  struct ln_lnlat_posn obs;
  syslog (LOG_DEBUG, "riseset thread start");

  obs.lng = get_longtitude ();
  obs.lat = get_latititude ();

  while (1)
    {
      int call_state;

      curr_time = time (NULL);
      devser_shm_data_lock ();

      next_event (&obs, &curr_time, &call_state, &shm_info->next_event_type,
		  &shm_info->next_event_time);

      if (shm_info->current_state != SERVERD_OFF
	  && shm_info->current_state != call_state)
	{
	  shm_info->current_state =
	    (shm_info->current_state & SERVERD_STANDBY_MASK) | call_state;
	  devices_all_msg_snd ("S %s %i", SERVER_STATUS,
			       shm_info->current_state);
	  clients_all_msg_snd ("S %s %i", SERVER_STATUS,
			       shm_info->current_state);
	}

      devser_shm_data_unlock ();

      syslog (LOG_DEBUG, "riseset thread sleeping %li seconds for %i",
	      shm_info->next_event_time - curr_time + 1,
	      shm_info->next_event_type);
      sleep (shm_info->next_event_time - curr_time + 1);
    }
}

/*!
 * Called on server exit.
 *
 * Delete client|device login|name, updates priorities, detach shared
 * memory.
 */
void
serverd_exit (void)
{
  devser_shm_data_lock ();

  switch (server_type)
    {
    case CLIENT_SERVER:
      *shm_clients[serverd_id].login = 0;
      clients_change_priority (0);
      break;
    case DEVICE_SERVER:
      *shm_devices[serverd_id].name = 0;
      break;
    default:
      // I'm not expecting that one to occur, since we
      // are registering at_exit after registering device,
      // but no one never knows..
      syslog (LOG_DEBUG, "exit of unknow type server %i", server_type);
    }

  devser_shm_data_unlock ();
}

/*!
 * @param new_state		new state, if -1 -> 3
 */
int
serverd_change_state (int new_state)
{
  devser_shm_data_lock ();
  shm_info->current_state = new_state;
  devices_all_msg_snd ("S %s %i", SERVER_STATUS, shm_info->current_state);
  clients_all_msg_snd ("S %s %i", SERVER_STATUS, shm_info->current_state);
  devser_shm_data_unlock ();
  return 0;
}

/*!
 * Print standart status header.
 *
 * It needs to be called after establishing of every new connection.
 */
int
serverd_status_info ()
{
  int ret;
  if ((ret = devser_dprintf ("I status_num 1")))
    return ret;
  ret =
    devser_dprintf ("I status 0 %s %i", SERVER_STATUS,
		    shm_info->current_state);
  return ret;
}

int
client_serverd_handle_command (char *command)
{
  if (strcmp (command, "password") == 0)
    {
      char *passwd;
      if (devser_param_test_length (1))
	return -1;
      if (devser_param_next_string (&passwd))
	return -1;

      /* authorize password
       *
       * TODO some more complicated code would be necessary */

      if (strncmp (passwd, shm_clients[serverd_id].login, CLIENT_LOGIN_SIZE)
	  == 0)
	{
	  shm_clients[serverd_id].authorized = 1;
	  devser_dprintf ("logged_as %i", serverd_id);
	  serverd_status_info ();
	  return 0;
	}
      else
	{
	  sleep (5);		// wait some time to prevent repeat attack
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "invalid login or password");
	  return -1;
	}
    }
  if (shm_clients[serverd_id].authorized)
    {
      if (strcmp (command, "ready") == 0)
	{
	  return 0;
	}

      if (strcmp (command, "info") == 0)
	{
	  return get_info ();
	}
      if (strcmp (command, "status_txt") == 0)
	{
	  char *new_st;
	  if (devser_param_test_length (1))
	    return -1;

	  if (devser_param_next_string (&new_st))
	    return -1;

	  strncpy (shm_clients[serverd_id].status_txt, new_st,
		   MAX_STATUS_TXT - 1);
	  shm_clients[serverd_id].status_txt[MAX_STATUS_TXT - 1] = 0;

	  return 0;
	}
      if (strcmp (command, "priority") == 0
	  || strcmp (command, "prioritydeferred") == 0)
	{
	  int timeout;
	  int new_priority;

	  if (strcmp (command, "priority") == 0)
	    {
	      if (devser_param_test_length (1))
		return -1;
	      if (devser_param_next_integer (&new_priority))
		return -1;
	      timeout = 0;
	    }
	  else
	    {
	      if (devser_param_test_length (2))
		return -1;
	      if (devser_param_next_integer (&new_priority))
		return -1;
	      if (devser_param_next_integer (&timeout))
		return -1;
	      timeout += time (NULL);
	    }

	  // prevent others from accesing priority
	  // since we don't want any other process to change
	  // priority while we are testing it
	  devser_shm_data_lock ();

	  devser_dprintf ("old_priority %i %i",
			  shm_clients[serverd_id].priority, serverd_id);

	  devser_dprintf ("actual_priority %i %i", shm_info->priority_client,
			  shm_clients[shm_info->priority_client].priority);

	  shm_clients[serverd_id].priority = new_priority;

	  if (clients_change_priority (timeout))
	    {
	      devser_shm_data_unlock ();
	      devser_write_command_end (DEVDEM_E_PRIORITY,
					"error when processing priority request");
	      return -1;
	    }

	  devser_dprintf ("new_priority %i %i", shm_info->priority_client,
			  shm_clients[shm_info->priority_client].priority);

	  devser_shm_data_unlock ();

	  return 0;
	}
      if (strcmp (command, "key") == 0)
	{
	  return get_device_key ();
	}
      if (strcmp (command, "on") == 0)
	{
	  return serverd_change_state ((shm_info->next_event_type + 5) % 6);
	}
      if (strcmp (command, "standby") == 0)
	{
	  return serverd_change_state (SERVERD_STANDBY |
				       ((shm_info->next_event_type + 5) % 6));
	}
      if (strcmp (command, "off") == 0)
	{
	  return serverd_change_state (SERVERD_OFF);
	}
    }
  devser_write_command_end (DEVDEM_E_COMMAND,
			    "unknow command / not authorized (please use pasword): %s",
			    command);
  return -1;
}

/*!
 * Handle receiving of IPC message.
 *
 * @param message	string message
 */
int
serverd_client_handle_msg (char *message)
{
  return devser_dprintf (message);
}

/*!
 * Handle receiving of IPC message.
 *
 * @param message	string message
 */
int
serverd_device_handle_msg (char *message)
{
  return devser_dprintf (message);
}


/*! 
 * Handle serverd commands.
 *
 * @param command 
 * @return -2 on exit, -1 and set errno on HW failure, 0 otherwise
 */
int
serverd_handle_command (char *command)
{
  if (strcmp (command, "login") == 0)
    {
      if (server_type == NOT_DEFINED_SERVER)
	{
	  char *login;
	  int i;

	  srandom (time (NULL));

	  if (devser_param_test_length (1))
	    return -1;

	  if (devser_param_next_string (&login) < 0)
	    return -1;

	  devser_shm_data_lock ();

	  // find not used client
	  for (i = (shm_info->last_client + 1) % MAX_CLIENT; i < MAX_CLIENT;
	       i = (i + 1) % MAX_CLIENT)
	    if (!*shm_clients[i].login)
	      {
		shm_clients[i].authorized = 0;
		shm_clients[i].priority = -1;
		shm_clients[i].status_txt[0] = 0;
		strncpy (shm_clients[i].login, login, CLIENT_LOGIN_SIZE);
		serverd_id = i;
		shm_clients[i].pid = devser_child_pid;
		if (devser_set_server_id (i, serverd_client_handle_msg))
		  {
		    devser_shm_data_unlock ();
		    return -1;
		  }
		break;
	      }
	  shm_info->last_client = i;
	  devser_shm_data_unlock ();

	  if (i == MAX_CLIENT)
	    {
	      devser_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate client - not enough resources");
	      return -1;
	    }

	  server_type = CLIENT_SERVER;
	  return 0;
	}
      else
	{
	  devser_write_command_end (DEVDEM_E_COMMAND,
				    "cannot switch server type to CLIENT_SERVER");
	  return -1;
	}
    }
  else if (strcmp (command, "register") == 0)
    {
      if (server_type == NOT_DEFINED_SERVER)
	{
	  char *reg_device;
	  int reg_type;
	  char *hostname;
	  unsigned int port;
	  int i;

	  if (devser_param_test_length (3))
	    return -1;
	  if (devser_param_next_string (&reg_device))
	    return -1;
	  if (devser_param_next_integer (&reg_type))
	    return -1;
	  if (devser_param_next_ip_address (&hostname, &port))
	    return -1;

	  devser_shm_data_lock ();
	  // find not used device
	  for (i = 0; i < MAX_DEVICE; i++)
	    if (!*(shm_devices[i].name))
	      {
		strncpy (shm_devices[i].name, reg_device, DEVICE_NAME_SIZE);
		strncpy (shm_devices[i].hostname, hostname, DEVICE_URI_SIZE);
		shm_devices[i].port = port;
		shm_devices[i].type = reg_type;
		shm_devices[i].pid = devser_child_pid;
		if (devser_set_server_id
		    (i + MAX_CLIENT, serverd_device_handle_msg))
		  {
		    devser_shm_data_unlock ();
		    return -1;
		  }
		serverd_id = i;
		break;
	      }
	    else if (strcmp (shm_devices[i].name, reg_device) == 0)
	      {
		devser_shm_data_unlock ();

		devser_write_command_end (DEVDEM_E_SYSTEM,
					  "name %s already registered",
					  reg_device);
		return -1;
	      }


	  if (i == MAX_DEVICE)
	    {
	      devser_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate new device - not enough resources");
	      devser_shm_data_unlock ();
	      return -1;
	    }

	  server_type = DEVICE_SERVER;
	  serverd_status_info ();
	  devser_shm_data_unlock ();

	  return 0;
	}
      else
	{
	  devser_write_command_end (DEVDEM_E_COMMAND,
				    "cannot switch server type to DEVICE_SERVER");
	  return -1;
	}
    }
  else if (server_type == DEVICE_SERVER)
    return device_serverd_handle_command (command);
  else if (server_type == CLIENT_SERVER)
    return client_serverd_handle_command (command);
  else
    {
      devser_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s",
				command);
      return -1;
    }
  return 0;
}

int
main (void)
{
  int i;
#ifdef DEBUG
  mtrace ();
#endif
  shm_info = NULL;
  shm_clients = NULL;
  shm_devices = NULL;

  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  if (devser_init (sizeof (struct serverd_info)))
    {
      syslog (LOG_ERR, "error in devser_init: %m");
      return -1;
    }

  shm_info = (struct serverd_info *) devser_shm_data_at ();
  shm_clients = shm_info->clients;
  shm_devices = shm_info->devices;

  for (i = 0; i < MAX_CLIENT; i++)
    {
      shm_clients[i].login[0] = 0;
      shm_clients[i].authorized = 0;
    }

  for (i = 0; i < MAX_DEVICE; i++)
    {
      shm_devices[i].name[0] = 0;
    }

  shm_info->current_state = 0;
  shm_info->priority_client = -1;
  shm_info->last_client = 0;

  devser_thread_create (serverd_riseset_thread, NULL, 0, NULL, NULL);

  devser_run (PORT, serverd_handle_command);

  serverd_exit ();

  return 0;
}
