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
#define CONTROLER_SERVER	1
#define DEVICE_SERVER		2

#define	DEVICE_NAME_SIZE	50
#define CLIENT_LOGIN_SIZE	50

// maximal number of devices
#define MAX_DEVICE		10
// maximal number of clients
#define MAX_CLIENT		10

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
};

//! information about serverd stored in shared mem
struct serverd_info
{
  int max_priority;
  int max_client;

  struct device devices[MAX_DEVICE];
  struct client clients[MAX_CLIENT];
};

//! global server type
int server_type = NOT_DEFINED_SERVER;
int server_id = -1;

int
device_serverd_handle_command (char *command)
{
  if (strcmp (command, "list") == 0)
    {
      struct serverd_info *info =
	(struct serverd_info *) devdem_shm_data_at ();
      struct client *clients = info->clients;
      int i;
      if (devdem_param_test_length (0))
	return -1;
      for (i = 0; i < MAX_CLIENT; i++)
	if (*clients[i].login)
	  devdem_dprintf ("user %s", clients[i].login);

      devdem_shm_data_dt ();
    }
}

int
controler_serverd_handle_command (char *command)
{
  if (strcmp (command, "list") == 0)
    {
      struct serverd_info *info =
	(struct serverd_info *) devdem_shm_data_at ();
      struct device *devices = info->devices;
      int i;
      if (devdem_param_test_length (0))
	return -1;
      for (i = 0; i < MAX_DEVICE; i++)
	if (*devices[i].name)
	  devdem_dprintf ("device %s", devices[i].name);
      devdem_shm_data_dt (info);
    }
  else if (strcmp (command, "priority") == 0)
    {
//      struct device *s = devdem_shm_data_at ();
      int new_priority;
      if (devdem_param_test_length (1))
	return -1;
      if (devdem_param_next_integer (&new_priority))
	return -1;
//      devdem_shm_data_dt ();
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
	    return -1;
	}
    }

  devdem_shm_data_dt (info);
  return 0;
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
	  for (i = 0; i < MAX_DEVICE; i++)
	    if (!*clients[i].login)
	      {
		strncpy (clients[i].login, login, CLIENT_LOGIN_SIZE);
		server_id = i;
		break;
	      }

	  devdem_shm_data_unlock ();

	  devices_message_send ("login %s", clients[i].login);

	  server_type = CONTROLER_SERVER;
	  devdem_shm_data_dt (info);
	  return 0;
	}
      else
	{
	  devdem_write_command_end (DEVDEM_E_COMMAND,
				    "cannot switch server type to CONTROLER_SERVER");
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
	    if (!*(devices[server_id].name))
	      {
		strncpy (devices[i].name, reg_device, DEVICE_NAME_SIZE);
		devices[i].pid = devdem_child_pid;
		devdem_msg_set_handler (serverd_handle_msg);
		server_id = i;
		break;
	      }
	    else if (strcmp (devices[i].name, reg_device) == 0)
	      {
		// delete registration - if registered
		if (server_id >= 0)
		  {
		    *(devices[server_id].name) = 0;
		    devices[server_id].pid = 0;
		    devdem_write_command_end (DEVDEM_E_SYSTEM,
					      "name %s already registered",
					      reg_device);
		    return -1;
		  }
	      }

	  devdem_shm_data_unlock ();
	  devdem_shm_data_dt (info);

	  if (server_id == MAX_DEVICE)
	    {
	      devdem_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate new client ID");
	      return -1;
	    }

	  server_type = DEVICE_SERVER;

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
  else if (server_type == CONTROLER_SERVER)
    return controler_serverd_handle_command (command);
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
