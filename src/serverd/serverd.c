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

#include <argz.h>

#include "../utils/devdem.h"
#include "../status.h"

#define PORT	5557

#define NOT_DEFINED_SERVER	0
#define CLIENT_SERVER		1
#define DEVICE_SERVER		2

#define	DEVICE_NAME_SIZE	50
#define CLIENT_LOGIN_SIZE	50

// maximal number of devices
#define MAX_DEVICE		10

int port;

int msgid;			// id of server message que

struct device
{
  pid_t pid;
  char name[DEVICE_NAME_SIZE];
};

struct client
{
  pid_t pid;
  char login[CLIENT_LOGIN_SIZE];
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

/*!
 * Send messsage to all registered devices.
 *
 * @param format	message format
 * @param ...		other arguments
 * 
 * @return 0 on success, -1 and set errno on any error
 */
int
devices_message_send (char *format, ...)
{
  struct serverd_info *info = devdem_shm_data_at ();
  struct device *devices = info->devices;
  struct devdem_msg msg;
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
      if (*devices[i].name)
	{
	  msg.mtype = devices[i].pid;
	  if (devdem_msg_snd (&msg))
	    {
	      devdem_shm_data_dt (info);
	      return -1;
	    }
	}
    }

  devdem_shm_data_dt (info);
  return 0;
}

int
device_serverd_handle_command (char *command)
{
  if (strcmp (command, "priority_set") == 0)
    {

    }
  else
    {
      devdem_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s",
				command);
      return -1;
    }

  return 0;
}

/*!
 * Made priority updates, distribute messages to devices
 * about priority update.
 *
 * @param info		server info, should be locked (some write
 *			access will be performed
 *			
 * @return 0 on success, -1 and set errno otherwise
 */
int
clients_change_priority (struct serverd_info *info)
{
  struct client *clients = info->clients;	// temporary pointer to save some computation
  int new_priority_client = info->priority_client;
  int new_priority_max = -2;
  int i;

  // change priority and find new client with highest priority
  for (i = 0; i < MAX_CLIENT; i++)
    {
      if (*clients[i].login && clients[i].priority > new_priority_max)
	{
	  new_priority_client = i;
	  new_priority_max = clients[i].priority;
	}
    }
  info->priority_client = new_priority_client;
  return devices_message_send ("priority %i", new_priority_client);
}

/*!
 * Called on serverd_exit
 *
 * Delete client|device login|name, updates priorities.
 *
 * @see on_exit(3)
 */
void
serverd_exit (int status, void *arg)
{
  struct serverd_info *info = (struct serverd_info *) devdem_shm_data_at ();
  devdem_shm_data_lock ();

  switch (server_type)
    {
    case CLIENT_SERVER:
      *info->clients[server_id].login = 0;
      clients_change_priority (info);
      break;
    case DEVICE_SERVER:
      *info->devices[server_id].name = 0;
      break;
    default:
      // I'm not expecting that one to occur, since we
      // are registering at_exit after registering device,
      // but no one never knows..
      syslog (LOG_DEBUG, "exit of unknow type server %i", server_type);
    }

  devdem_shm_data_unlock ();
  devdem_shm_data_dt (info);
}

int
client_serverd_handle_command (char *command)
{
  if (strcmp (command, "priority") == 0)
    {
      struct serverd_info *info =
	(struct serverd_info *) devdem_shm_data_at ();
      struct client *clients = info->clients;
      // prevent others from accesing priority
      // since we don't wont any other process to change
      // priority while we are testing it
      devdem_shm_data_lock ();

      if (devdem_param_test_length (1))
	goto exit_error;

      devdem_dprintf ("old_priority %i %i", info->priority_client,
		      clients[server_id].priority);

      if (devdem_param_next_integer (&(clients[server_id].priority)))
	goto exit_error;

      if (clients_change_priority (info))
	{
	  devdem_write_command_end (DEVDEM_E_PRIORITY,
				    "error when processing priority request");
	  goto exit_error;
	}

      devdem_dprintf ("new_priority %i %i", info->priority_client,
		      clients[server_id].priority);

      devdem_shm_data_unlock ();
      devdem_shm_data_dt (info);
      return 0;

    exit_error:
      // unlock, cleanup
      devdem_shm_data_unlock ();
      devdem_shm_data_dt (info);
      return -1;
    }
  else
    {
      devdem_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s",
				command);
      return -1;
    }
  return 0;

}

/*!
 * Handle receiving of message.
 *
 * @param message	string message
 */
void
serverd_handle_msg (char *message)
{
  devdem_status_message (0, message);
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
  if (strcmp (command, "info") == 0)
    {
      struct serverd_info *info =
	(struct serverd_info *) devdem_shm_data_at ();
      struct client *clients = info->clients;
      struct device *devices = info->devices;
      int i;
      if (devdem_param_test_length (0))
	return -1;

      for (i = 0; i < MAX_CLIENT; i++)
	if (*clients[i].login)
	  devdem_dprintf ("user %s %i", clients[i].login, i);

      for (i = 0; i < MAX_DEVICE; i++)
	if (*devices[i].name)
	  devdem_dprintf ("device %s %i", devices[i].name, i);

      devdem_shm_data_dt (info);
    }
  else if (strcmp (command, "login") == 0)
    {
      if (server_type == NOT_DEFINED_SERVER)
	{
	  struct serverd_info *info =
	    (struct serverd_info *) devdem_shm_data_at ();
	  struct client *clients = info->clients;
	  char *login;
	  int i;
	  if (devdem_param_test_length (1))
	    return -1;

	  if (devdem_param_next_string (&login) < 0)
	    return -1;

	  devdem_shm_data_lock ();

	  // find not used client
	  for (i = 0; i < MAX_CLIENT; i++)
	    if (!*clients[i].login)
	      {
		strncpy (clients[i].login, login, CLIENT_LOGIN_SIZE);
		clients[i].priority = -1;
		server_id = i;
		break;
	      }

	  devdem_shm_data_unlock ();
	  devdem_shm_data_dt (info);

	  if (i == MAX_CLIENT)
	    {
	      devdem_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate client - not enough resources");
	      return -1;
	    }

	  devices_message_send ("login %s", clients[i].login);

	  server_type = CLIENT_SERVER;
	  on_exit (serverd_exit, NULL);
	  return 0;
	}
      else
	{
	  devdem_write_command_end (DEVDEM_E_COMMAND,
				    "cannot switch server type to CLIENT_SERVER");
	  return -1;
	}
    }
  else if (strcmp (command, "register") == 0)
    {
      if (server_type == NOT_DEFINED_SERVER)
	{
	  struct serverd_info *info =
	    (struct serverd_info *) devdem_shm_data_at ();
	  struct device *devices = info->devices;
	  char *reg_device;
	  int i;

	  if (devdem_param_test_length (1))
	    return -1;
	  if (devdem_param_next_string (&reg_device))
	    return -1;

	  devdem_shm_data_lock ();
	  // find not used device
	  for (i = 0; i < MAX_DEVICE; i++)
	    if (!*(devices[i].name))
	      {
		strncpy (devices[i].name, reg_device, DEVICE_NAME_SIZE);
		devices[i].pid = devdem_child_pid;
		devdem_msg_set_handler (serverd_handle_msg);
		server_id = i;
		break;
	      }
	    else if (strcmp (devices[i].name, reg_device) == 0)
	      {
		devdem_shm_data_unlock ();
		devdem_shm_data_dt (info);

		devdem_write_command_end (DEVDEM_E_SYSTEM,
					  "name %s already registered",
					  reg_device);
		return -1;
	      }

	  devdem_shm_data_unlock ();
	  devdem_shm_data_dt (info);

	  if (i == MAX_DEVICE)
	    {
	      devdem_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate new device - not enough resources");
	      return -1;
	    }

	  server_type = DEVICE_SERVER;
	  on_exit (serverd_exit, NULL);
	  return 0;
	}
      else
	{
	  devdem_write_command_end (DEVDEM_E_COMMAND,
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
      devdem_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s",
				command);
      return -1;
    }
  return 0;
}

int
main (void)
{
  char *stats[] = { "status" };
#ifdef DEBUG
  mtrace ();
#endif
  openlog (NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  return devdem_run (PORT, serverd_handle_command, stats, 1,
		     sizeof (struct serverd_info));
}
