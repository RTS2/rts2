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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <mcheck.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../utils/devser.h"
#include "../status.h"

#define PORT	5557

#define NOT_DEFINED_SERVER	0
#define CLIENT_SERVER		1
#define DEVICE_SERVER		2

#define	DEVICE_NAME_SIZE	50
#define CLIENT_LOGIN_SIZE	50
#define CLIENT_PASSWD_SIZE	50

// maximal number of devices
#define MAX_DEVICE		10

int port;

int msgid;			// id of server message que

struct device
{
  pid_t pid;
  char name[DEVICE_NAME_SIZE];
  int authorizations[MAX_CLIENT];
};

struct client
{
  pid_t pid;
  char login[CLIENT_LOGIN_SIZE];
  int authorized;
  int priority;
};

//! information about serverd stored in shared mem
struct serverd_info
{
  int priority_client;

  struct device devices[MAX_DEVICE];
  struct client clients[MAX_CLIENT];
};

//! global server type
int server_type = NOT_DEFINED_SERVER;
int server_id = -1;

//! pointers to shared memory
struct serverd_info *shm_info;
struct device *shm_devices;
struct client *shm_clients;

/*!
 * Find one device.
 * 
 * @param name		device name
 * @param device	device
 *
 * @return 0 and set pointer do device on success, -1 and set errno on error
 */
int
find_device (char *name, struct device **device)
{
  int i;
  for (i = 0; i < MAX_DEVICE; i++)
    if (strcmp (shm_devices[i].name, name) == 0)
      {
	*device = &shm_devices[i];
	return 0;
      }
  errno = ENODEV;
  return -1;
}

/*!
 * Send message to one device.
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
device_message_send (char *name, char *format, ...)
{
  struct device *device;
  if (find_device (name, &device))
    {
      struct devser_msg msg;
      char *mtext;
      va_list ap;

      va_start (ap, format);
      vasprintf (&mtext, format, ap);
      va_end (ap);

      strncpy (msg.mtext, mtext, MSG_SIZE);
      msg.mtype = device->pid;

      free (mtext);

      return devser_msg_snd (&msg);
    }
  errno = ENODEV;
  return -1;
}

/*!
 * Send messsage to all registered devices.
 *
 * @param format	message format
 * @param ...		format arguments
 * 
 * @return 0 on success, -1 and set errno on any error
 */
int
devices_message_send (char *format, ...)
{
  struct devser_msg msg;
  char *mtext;
  va_list ap;
  int i;

  va_start (ap, format);
  vasprintf (&mtext, format, ap);
  va_end (ap);

  strncpy (msg.mtext, mtext, MSG_SIZE);

  free (mtext);

  for (i = 0; i < MAX_DEVICE; i++)
    {
      /* device exists */
      if (*shm_devices[i].name)
	{
	  msg.mtype = shm_devices[i].pid;
	  if (devser_msg_snd (&msg))
	    return -1;
	}
    }
  return 0;
}

int
device_serverd_handle_command (char *command)
{
  if (strcmp (command, "priority_set") == 0)
    {

    }
  else if (strcmp (command, "authorize") == 0)
    {
      int client;
      int key;
      if (devser_param_test_length (2))
	return -1;
      if (devser_param_next_integer (&client)
	  || devser_param_next_integer (&key))
	return -1;

      devser_shm_data_lock ();
      if (shm_devices[server_id].authorizations[client] == 0)
	{
	  devser_dprintf ("authorization_failed %i", client);
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "client %i didn't ask for authorization",
				    client);
	  devser_shm_data_unlock ();
	  return -1;
	}

      if (shm_devices[server_id].authorizations[client] != key)
	{
	  devser_dprintf ("authorization_failed %i", client);
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "invalid authorization key");
	  shm_devices[server_id].authorizations[client] = 0;
	  devser_shm_data_unlock ();
	  return -1;
	}
      shm_devices[server_id].authorizations[client] = 0;
      devser_shm_data_unlock ();

      devser_dprintf ("authorization_ok %i", client);

      return 0;
    }
  devser_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s", command);
  return -1;
}

/*!
 * Made priority update, distribute messages to devices
 * about priority update.
 *
 * @return 0 on success, -1 and set errno otherwise
 */
int
clients_change_priority ()
{
  int new_priority_client = shm_info->priority_client;
  int new_priority_max = -2;
  int i;

  // change priority and find new client with highest priority
  for (i = 0; i < MAX_CLIENT; i++)
    {
      if (*shm_clients[i].login && shm_clients[i].priority > new_priority_max)
	{
	  new_priority_client = i;
	  new_priority_max = shm_clients[i].priority;
	}
    }
  shm_info->priority_client = new_priority_client;
  return devices_message_send ("priority %i", new_priority_client);
}

/*!
 * Attach shared memory segments.
 */
void
serverd_init ()
{
  if (!shm_info)
    {
      shm_info = (struct serverd_info *) devser_shm_data_at ();
      shm_clients = shm_info->clients;
      shm_devices = shm_info->devices;
    }
}

/*!
 * Called on server exit.
 *
 * Delete client|device login|name, updates priorities, detach shared
 * memory.
 *
 * @see on_exit(3)
 */
void
serverd_exit (int status, void *arg)
{
  devser_shm_data_lock ();

  switch (server_type)
    {
    case CLIENT_SERVER:
      *shm_clients[server_id].login = 0;
      clients_change_priority ();
      break;
    case DEVICE_SERVER:
      *shm_devices[server_id].name = 0;
      break;
    default:
      // I'm not expecting that one to occur, since we
      // are registering at_exit after registering device,
      // but no one never knows..
      syslog (LOG_DEBUG, "exit of unknow type server %i", server_type);
    }

  devser_shm_data_dt (shm_info);
  devser_shm_data_unlock ();
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

      if (strncmp (passwd, shm_clients[server_id].login, CLIENT_LOGIN_SIZE)
	  == 0)
	{
	  shm_clients[server_id].authorized = 1;
	  devser_dprintf ("logged as client # %i", server_id);
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
  if (shm_clients[server_id].authorized)
    {
      if (strcmp (command, "info") == 0)
	{
	  int i;
	  if (devser_param_test_length (0))
	    return -1;

	  for (i = 0; i < MAX_CLIENT; i++)
	    if (!*shm_clients[i].login)
	      devser_dprintf ("user %s %i", shm_clients[i].login, i);

	  for (i = 0; i < MAX_DEVICE; i++)
	    if (!*shm_devices[i].name)
	      devser_dprintf ("device %s %i", shm_devices[i].name, i);
	  return 0;
	}
      else if (strcmp (command, "priority") == 0)
	{
	  // prevent others from accesing priority
	  // since we don't wont any other process to change
	  // priority while we are testing it
	  devser_shm_data_lock ();

	  if (devser_param_test_length (1))
	    return -1;

	  devser_dprintf ("old_priority %i %i", shm_info->priority_client,
			  shm_clients[server_id].priority);

	  if (devser_param_next_integer (&(shm_clients[server_id].priority)))
	    return -1;

	  if (clients_change_priority ())
	    {
	      devser_write_command_end (DEVDEM_E_PRIORITY,
					"error when processing priority request");
	      return -1;
	    }

	  devser_dprintf ("new_priority %i %i", shm_info->priority_client,
			  shm_clients[server_id].priority);

	  devser_shm_data_unlock ();
	  return 0;
	}
      if (strcmp (command, "key") == 0)
	{
	  int key = random ();
	  // device number could change..device names don't
	  char *dev_name;
	  struct device *device;
	  if (devser_param_test_length (1)
	      || devser_param_next_string (&dev_name))
	    return -1;
	  // find device, set it authorization key
	  if (find_device (dev_name, &device))
	    {
	      devser_write_command_end (DEVDEM_E_SYSTEM,
					"cannot find device with name %s",
					dev_name);
	      return -1;
	    }

	  devser_shm_data_lock ();
	  device->authorizations[server_id] = key;
	  devser_shm_data_unlock ();

	  devser_dprintf ("authorization_key %i", key);
	  return 0;
	}
    }
  devser_write_command_end (DEVDEM_E_COMMAND,
			    "unknow command / not authorized (please use pasword): %s",
			    command);
  return -1;
}

/*!
 * Handle receiving of message.
 *
 * @param message	string message
 */
void
serverd_handle_msg (char *message)
{
  devser_status_message (0, message);
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

	  serverd_init ();

	  if (devser_param_test_length (1))
	    return -1;

	  if (devser_param_next_string (&login) < 0)
	    return -1;

	  devser_shm_data_lock ();

	  // find not used client
	  for (i = 0; i < MAX_CLIENT; i++)
	    if (!*shm_clients[i].login)
	      {
		shm_clients[i].authorized = 0;
		shm_clients[i].priority = -1;
		strncpy (shm_clients[i].login, login, CLIENT_LOGIN_SIZE);
		server_id = i;
		break;
	      }

	  devser_shm_data_unlock ();

	  if (i == MAX_CLIENT)
	    {
	      devser_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate client - not enough resources");
	      return -1;
	    }

	  server_type = CLIENT_SERVER;
	  on_exit (serverd_exit, NULL);
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
	  int i;

	  serverd_init ();

	  if (devser_param_test_length (1))
	    return -1;
	  if (devser_param_next_string (&reg_device))
	    return -1;

	  devser_shm_data_lock ();
	  // find not used device
	  for (i = 0; i < MAX_DEVICE; i++)
	    if (!*(shm_devices[i].name))
	      {
		strncpy (shm_devices[i].name, reg_device, DEVICE_NAME_SIZE);
		shm_devices[i].pid = devser_child_pid;
		devser_msg_set_handler (serverd_handle_msg);
		server_id = i;
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

	  devser_shm_data_unlock ();

	  if (i == MAX_DEVICE)
	    {
	      devser_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate new device - not enough resources");
	      return -1;
	    }

	  server_type = DEVICE_SERVER;
	  on_exit (serverd_exit, NULL);
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
  srandom (time (NULL));
  char *stats[] = { "status" };
#ifdef DEBUG
  mtrace ();
#endif
  shm_info = NULL;
  shm_clients = NULL;
  shm_devices = NULL;

  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  return devser_run (PORT, serverd_handle_command, stats, 1,
		     sizeof (struct serverd_info));
}
