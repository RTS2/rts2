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

#define MAX_DEVICE		10

int port;

int msgid;			// id of server message que

struct device
{
  pid_t pid;
  char name[DEVICE_NAME_SIZE];
};

int devices_sem;		// devices semaphore to control write access to shared memory
int devices_shm;		// devices shared memory id

int server_type = NOT_DEFINED_SERVER;

int server_id = -1;

int
device_serverd_handel_command (char *argv, size_t argc)
{

}

int
controler_serverd_handle_command (char *argv, size_t argc)
{

}

// macro for length test
#define test_length(npars) if (argz_count (argv, argc) != npars + 1) { \
        devdem_write_command_end (DEVDEM_E_PARAMSNUM,\
		"Unknow nmbr of params: expected %i,got %i",\
		npars, argz_count (argv, argc) ); \
	return -1; \
}

/*!
 * Handle receiving of message.
 *
 * @param message	message
 * @param size		message size (in bytes)
 *
 * @return	not inportant
 */

int
serverd_handle_msg (char *message, size_t size)
{
  devdem_status_message (0, message);
  return 0;
}

/*! 
 * Handle serverd commands.
 *
 * @param argv
 * @param argc
 * @return -2 on exit, -1 and set errno on HW failure, 0 otherwise
 */
int
serverd_handle_command (char *argv, size_t argc)
{
  if (strcmp (argv, "login") == 0)
    {
      if (server_type == NOT_DEFINED_SERVER)
	{
	  struct device *devices = devdem_shm_data_at ();
	  int i;
	  server_type = CONTROLER_SERVER;
	  for (i = 0; i < MAX_DEVICE; i++)
	    {
	      if (*devices[i].name)
		{
		  struct devdem_msg msg;
		  msg.mtype = devices[i].pid;
		  strcpy (msg.mtext, "login");
		  devdem_msg_snd (&msg);
		}
	    }
	  devdem_shm_data_dt (devices);
	  return 0;
	}
      else
	{
	  devdem_write_command_end (DEVDEM_E_COMMAND,
				    "cannot switch server type to CONTROLER_SERVER");
	  return -1;
	}
    }
  else if (strcmp (argv, "register") == 0)
    {
      if (server_type == NOT_DEFINED_SERVER)
	{
	  struct device *devices = (struct device *) devdem_shm_data_at ();
	  char *reg_device;
	  int i;
	  test_length (1);
	  reg_device = argz_next (argv, argc, argv);
	  reg_device[DEVICE_NAME_SIZE - 1] = 0;
	  server_type = DEVICE_SERVER;
	  devdem_shm_data_lock ();
	  for (i = 0; i < MAX_DEVICE; i++)
	    {
	      if ((server_id < 1) && !*(devices[server_id].name))
		{
		  strcpy (devices[i].name, reg_device);
		  devices[i].pid = devdem_child_pid;
		  devdem_msg_set_handler (serverd_handle_msg);
		  server_id = i;
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
	    }

	  devdem_shm_data_unlock ();
	  devdem_shm_data_dt (devices);

	  if (server_id == MAX_DEVICE)
	    {
	      devdem_write_command_end (DEVDEM_E_SYSTEM,
					"cannot allocate new client ID");
	      return -1;
	    }


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
    return device_serverd_handel_command (argv, argc);
  else if (server_type == CONTROLER_SERVER)
    return controler_serverd_handle_command (argv, argc);
  else
    {
      devdem_write_command_end (DEVDEM_E_COMMAND, "unknow command: %s", argv);
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
		     sizeof (struct device) * MAX_DEVICE);
}
