/*!
 * @file Server for devices.
 * $Id$
 *
 * Combines server functions (for client log-in) and client functions
 * (for access to serverd) in one package.
 *
 * @author petr
 */
#define _GNU_SOURCE

#include "devdem.h"
#include "devser.h"
#include "../status.h"

#include "devconn.h"

#include "devcli.h"

#include "param.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <mcheck.h>
#include <math.h>
#include <pthread.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun
{
  int val;			/* value for SETVAL */
  struct semid_ds *buf;		/* buffer for IPC_STAT, IPC_SET */
  unsigned short *array;	/* array for GETALL, SETALL */
  /* Linux specific part: */
  struct seminfo *__buf;	/* buffer for IPC_INFO */
};
#endif

//! client number
int client_id = -1;

//! client is authorized
int client_authorized = 0;

//! number of status registers
int status_num;

//! key to shared memory segment, holding status flags.
int status_shm;

//! key to semaphore for access control to status shared memory segment
int status_sem;

//! holds status informations
typedef struct
{
  char name[STATUSNAME + 1];
  int status;
}
status_t;

//! array holding status structures; stored in shared mem, hence just pointer
status_t *statutes;

//! handler functions
devser_handle_command_t cmd_device_handler;

//! information about running devices; held in shared memory
struct client_info
{
  int priority_client;		// client with maximal priority
  int designated_priority_client;	// next priority client
  struct
  {
    int authorized;
    pid_t pid;
  }
  clients[MAX_CLIENT];
};

//! status of priorities - set || unset
int client_status;

struct client_info *clients_info;	// mapped to shared memory

//! block priority calls
pthread_mutex_t priority_block = PTHREAD_MUTEX_INITIALIZER;

//! block counter - for deferred priority
int block_counter;

//! block mutex - start/stop of mutex
pthread_mutex_t block_mutex_main = PTHREAD_MUTEX_INITIALIZER;

//! block mutex - defence
pthread_mutex_t block_mutex_counter = PTHREAD_MUTEX_INITIALIZER;

//! condition for block count change
pthread_cond_t block_cond_counter = PTHREAD_COND_INITIALIZER;

#define SERVER_CLIENT		MAX_CLIENT + 1	// id of process connected to server - for messages

/*!
 * Perform operation on status semaphore.
 *
 * @param sem_num	semaphor number
 * @param sem_op	operation
 * @param sem_flg	flags
 * @return 0 on success, -1 and set errno on error.
 */
int
status_op (int sem_num, int sem_op, int sem_flg)
{
  struct sembuf sb;

  sb.sem_num = sem_num;
  sb.sem_op = sem_op;
  sb.sem_flg = sem_flg;
  if (semop (status_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "status_op semaphore %i op %i flags %i: %m", sem_num,
	      sem_op, sem_flg);
      return -1;
    }
  return 0;
}

/*! 
 * Locks given status semaphore.
 *
 * @param sem 		semaphore index
 *
 * @return 0 on success, -1 and set errno on failure; log failure to syslog
 */
int
status_lock (int sem)
{
  return status_op (sem, -1, SEM_UNDO);
}

/*! 
 * Unlocks given status semaphore.
 *
 * @param sem		semaphore index
 *
 * @return 0 on success, -1 and set errno on failure; log failure to syslog
 */
int
status_unlock (int sem)
{
  return status_op (sem, 1, SEM_UNDO);
}

int
status_message (int subdevice, char *description)
{
  status_t *st;
  st = &(statutes[subdevice]);
  return devser_message ("%s %i %s", st->name, st->status, description);
}

/*! 
 * Send status message to client, log it, lock subdevices.
 *
 * @param subdevice 	subdevice index
 * @param description 	message description, could be null
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int
devdem_status_message (int subdevice, char *description)
{
  if (subdevice >= status_num || subdevice < 0)
    {
      errno = EINVAL;
      syslog (LOG_ERR, "invalid subdevice in devser_status_message: %i",
	      status_num);
      return -1;
    }
  if (status_lock (subdevice) < 0)
    return -1;
  if (status_message (subdevice, description) < 0)
    {
      status_unlock (subdevice);
      return -1;
    }
  return status_unlock (subdevice);
}

/*! 
 * Routine to perform bit "and" operation on status bits.
 *
 * @param subdevice 	subdevice to change.
 * @param mask 		<code>statutes[subdevice].status &= !mask;</code>
 * @param operand  	<code>statutes[subdevice].status |= operand;</code>
 * @param message 	optional message to transmit
 *
 * @return -1 and set errno on error, 0 otherwise
 */
int
devdem_status_mask (int subdevice, int mask, int operand, char *message)
{
  if (status_lock (subdevice) < 0)
    return -1;
  // critical section begins

  statutes[subdevice].status &= !mask;
  statutes[subdevice].status |= operand;

  if (status_message (subdevice, message) < 0)
    {
      status_unlock (subdevice);
      return -1;
    }

  // critical section ends
  return status_unlock (subdevice);
}

// priority block helper functions

/*!
 * 
 * @return 1 if we have priority -> can execute power functions, 0
 * 		otherwise
 */
void
priority_block_end ()
{
  if ((errno = pthread_mutex_unlock (&priority_block)))
    syslog (LOG_ERR,
	    "error in devser_priority_block_end unlocking priority_block mutex: %m");
}

int
priority_block_start ()
{
  int has_priority;

  if ((errno = pthread_mutex_lock (&priority_block)))
    {
      syslog (LOG_ERR,
	      "while locking priority_block in devser_priority_block_start: %m");
      return -1;
    }
  has_priority = client_id >= 0 && clients_info->priority_client == client_id;

  if (!has_priority)
    {
      syslog (LOG_DEBUG, "haven't priority");
      priority_block_end ();
      return -1;
    }

  syslog (LOG_DEBUG, "entering priority block");
  return 0;
}

/*!
 * Start priority block.
 *
 * @return 0 on success, -1 on failure (not priority or other system
 * 	failure.) On failure write back command end.
 */
int
devdem_priority_block_start ()
{
  if (priority_block_start ())
    {
      devser_write_command_end (DEVDEM_E_PRIORITY,
				"you haven't priority to execute this command");
      return -1;
    }
  return 0;
}

int
devdem_priority_block_end ()
{
  priority_block_end ();
  syslog (LOG_DEBUG, "priority block ended");
  return 0;
}

void
client_priority_lost ()
{
  syslog (LOG_INFO, "calling thread cancel");

  priority_block_start ();
  devser_thread_cancel_all ();
  devdem_priority_block_end ();

  devser_thread_wait ();

  clients_info->priority_client = -1;

  status_unlock (client_status);

  // inform client
  devser_message ("priority lost");
}

void
client_priority_receive ()
{
  status_lock (client_status);
  if (clients_info->designated_priority_client != client_id)
    {
      // designated_priority_client is other than our client_id, when
      // some observation receive priority before us, sucessfully lock 
      // & observe. In such case we don't have any reason to persist
      // on priority, since we already have to lose it before
      // we authorize.
      //
      // client_id is initialized, since we started message receive
      // thread after sucessfull auth.
      syslog (LOG_INFO, "designated_priority_client != client_id => exit");
      client_priority_lost ();
      return;
    }
  clients_info->priority_client = client_id;

  devser_message ("priority received");
}

void
client_priority_deferred_lost (time_t timeout)
{
  if (timeout && pthread_mutex_trylock (&block_mutex_main) == EBUSY)
    {
      int old_counter;
      struct timespec abstime;
      // we are at cancelation control block
      pthread_mutex_lock (&block_mutex_counter);
      old_counter = block_counter;

      abstime.tv_sec = timeout;
      abstime.tv_nsec = 0;

      // wait until blockcheck or blockend is perfromed, or timeouted
      while (block_counter == old_counter)
	{
	  if (pthread_cond_timedwait
	      (&block_cond_counter, &block_mutex_counter,
	       &abstime) == ETIMEDOUT)
	    break;
	}
      pthread_mutex_unlock (&block_mutex_counter);
      client_priority_lost ();
    }
  else
    {
      // we are NOT at cancelation control block or timeout is 0
      client_priority_lost ();
      pthread_mutex_unlock (&block_mutex_main);
    }
}

/*!
 * Increase block counter.
 *
 * Performs all required locking, signal counter change..
 */
int
client_block_counter_increase ()
{
  // check for block_mutex_main lock..
  if (pthread_mutex_trylock (&block_mutex_main) == EBUSY)
    {
      if (pthread_mutex_lock (&block_mutex_counter))
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "counter mutex cannot be locked");
	  return -1;
	}
      block_counter++;
      pthread_cond_broadcast (&block_cond_counter);
      pthread_mutex_unlock (&block_mutex_counter);
      return 0;
    }
  devser_write_command_end (DEVDEM_E_SYSTEM, "block wasn't started");
  return -1;
}

#ifdef DEBUG
/*!
 * Write status of devdem.
 */
int
devdem_status ()
{
  int i;
  devser_dprintf ("priority_client %i", clients_info->priority_client);
  for (i = 0; i < MAX_CLIENT; i++)
    if (clients_info->clients[i].pid)
      devser_dprintf ("client_id_pid %i %i", i, clients_info->clients[i].pid);
  if (pthread_mutex_trylock (&block_mutex_main) == EBUSY)
    {
      devser_dprintf ("block_main_lock locked");
    }
  else
    {
      devser_dprintf ("block_main_lock unlocked");
      pthread_mutex_unlock (&block_mutex_main);
    }
  devser_dprintf ("block_counter %i", block_counter);
  return 0;
}
#endif /* DEBUG */

/*!
 * @return 0 processed, -1 invalid message => not processed
 */
int
client_handle_msg (char *msg)
{
  struct param_status *params;
  param_init (&params, msg, ' ');
  if (strcmp (params->param_argv, "priority_receive") == 0)
    {
      client_priority_receive ();
    }
  else if (strcmp (params->param_argv, "priority_lost") == 0)
    {
      time_t timeout;
      if (param_next_time_t (params, &timeout))
	{
	  syslog (LOG_ERR, "client_handle_msg invalid timeout: %li error: %m",
		  timeout);
	}
      else
	client_priority_deferred_lost (timeout);
    }
  else
    {
      param_done (params);
      return -1;
    }
  param_done (params);
  return 0;
}

int
server_handle_msg (char *msg)
{
  if (strncmp (msg, "authorize", 9) == 0)
    {
      if (devcli_server_command (msg))
	syslog (LOG_ERR, "devcli_command %s: %m", msg);
      return 0;
    }
  return -1;
}

int
client_authorize ()
{
  struct sembuf sb;
  int new_client_id;
  int key;
  if (devser_param_test_length (2))
    return -1;
  if (devser_param_next_integer (&new_client_id)
      || devser_param_next_integer (&key))
    return -1;
  if (new_client_id < 0 || new_client_id >= MAX_CLIENT)
    {
      devser_write_command_end (DEVDEM_E_SYSTEM, "invalid client number: %i",
				new_client_id);
      return -1;
    }
  sb.sem_num = status_num + new_client_id;
  sb.sem_op = -2;
  sb.sem_flg = SEM_UNDO | IPC_NOWAIT;

  if (semop (status_sem, &sb, 1))
    {
      if (errno == EAGAIN)
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "cannot lock authorize semaphore - paralel authorization");
	  return -1;
	}
      else
	{
	  syslog (LOG_ERR,
		  "client_authorize semop -2 (authorization lock): %m");
	  devser_write_command_end (DEVDEM_E_SYSTEM, "invalid semctl call");
	  return -1;
	}
    }
  // yes, we could set it now, since it's guared by authorization
  // semaphore 
  clients_info->clients[new_client_id].authorized = 0;

  // get authorization key
  devser_2devser_message_format (SERVER_CLIENT, "authorize %i %i",
				 new_client_id, key);

  // wait for server client to unblock us..
  sb.sem_op = -1;
  sb.sem_flg = 0;
  if (semop (status_sem, &sb, 1))
    {
      syslog (LOG_ERR, "client_authorize semop # %i: %m", sb.sem_num);
      devser_write_command_end (DEVDEM_E_SYSTEM, "error by semop call");
      return -1;
    }

  // compare authorization key
  if (!clients_info->clients[new_client_id].authorized)
    {
      devser_write_command_end (DEVDEM_E_SYSTEM, "invalid authorization key");
      exit (0);
    }

  // if we got there, we are authorized and so we can proceed
  client_authorized = 1;

  client_id = new_client_id;

  if (devser_set_server_id (client_id, client_handle_msg))
    {
      syslog (LOG_ERR, "client_authorize devser_set_server_id");
      devser_write_command_end (DEVDEM_E_SYSTEM,
				"error while setting up client");
      return -1;
    }
  // TODO remove pid - not needed ???
  clients_info->clients[client_id].pid = devser_child_pid;
  return 0;
}

int
client_handle_commands (char *command)
{
  if (strcmp (command, "auth") == 0)
    return client_authorize ();
  else if (!client_authorized)
    {
      devser_write_command_end (DEVDEM_E_SYSTEM, "you aren't authorized");
      return -1;
    }
#ifdef DEBUG
  else if (strcmp (command, "devdem_status") == 0)
    return devdem_status ();
#endif /* DEBUG */
  else if (strcmp (command, "blockstart") == 0)
    {
      if (pthread_mutex_trylock (&block_mutex_main) == EBUSY)
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "instruction block already started");
	  return -1;
	}
      if (pthread_mutex_lock (&block_mutex_counter))
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "counter lock cannot by locked");
	  return -1;
	}
      block_counter = 0;
      pthread_mutex_unlock (&block_mutex_counter);
      return 0;
    }
  else if (strcmp (command, "blockcheck") == 0)
    {
      return client_block_counter_increase ();
    }
  else if (strcmp (command, "blockend") == 0)
    {
      // following unlock is posible, because in cond_waited threads
      // doesn't matter, if block_mutex_main is locked or not..
      int ret = client_block_counter_increase ();
      pthread_mutex_unlock (&block_mutex_main);
      return ret;
    }
  else if (cmd_device_handler)
    {
      return cmd_device_handler (command);
    }
  devser_write_command_end (DEVDEM_E_SYSTEM, "NULL cmd_device_handler");
  return -1;
}

/*!
 * Handle command responses, comming from central server
 *
 * @param params	parsed line 
 *
 * @return 0 on success, -1 and set errno on error
 */
int
server_command_handler (struct param_status *params)
{
  if (strcmp (params->param_argv, "authorization_ok") == 0)
    {
      struct sembuf sb;
      int auth_id;

      if (param_next_integer (params, &auth_id) || auth_id < 0
	  || auth_id >= MAX_CLIENT)
	{
	  syslog (LOG_ERR,
		  "server_command_handler authorization_ok invalid auth_id: %i",
		  auth_id);
	  return -1;
	}

      clients_info->clients[auth_id].authorized = 1;

      // unlock device semaphore..
      sb.sem_num = status_num + auth_id;
      sb.sem_op = +1;
      sb.sem_flg = SEM_UNDO;
      if (semop (status_sem, &sb, 1))
	{
	  syslog (LOG_ERR, "server_command_handler semop: %m");
	  return -1;
	}
      syslog (LOG_DEBUG, "client authorized: %i", auth_id);
    }
  else if (strcmp (params->param_argv, "authorization_failed") == 0)
    {
      struct sembuf sb;
      int auth_id;

      if (param_next_integer (params, &auth_id) || auth_id < 0
	  || auth_id >= MAX_CLIENT)
	{
	  syslog (LOG_ERR,
		  "server_command_handler authorization_failed invalid auth_id: %i",
		  auth_id);
	  return -1;
	}

      clients_info->clients[auth_id].authorized = 0;

      // unlock device semaphore..
      sb.sem_num = status_num + auth_id;
      sb.sem_op = +1;
      sb.sem_flg = SEM_UNDO;
      if (semop (status_sem, &sb, 1))
	{
	  syslog (LOG_ERR, "server_command_handler semop: %m");
	  return -1;
	}

      syslog (LOG_DEBUG, "client non authorized: %i", auth_id);
    }
  else
    {
      printf ("returned: %s\n", params->param_argv);
      fflush (stdout);

      return 0;
    }
  return 0;
}

/*!
 * Handle message with changing priority, comming from central server.
 *
 * Have to send priority_lost IPC message to currently active devser
 * (if any), wait till all threads are killed, change actual priority 
 * and send priority_receive message to new devser process.
 *
 * @param params		arguments with priority setting (as
 * 				parsed by param_init)
 *
 * @return 0 on success, -1 and set errno otherwise
 */
int
server_message_priority (struct param_status *params)
{
  int new_priority_client;
  // get new priority client from message
  if (param_next_integer (params, &new_priority_client))
    {
      syslog (LOG_ERR,
	      "while processing priority request - invalid number: %i",
	      new_priority_client);
      return -1;
    }
  else
    {
      if (new_priority_client < 0 || new_priority_client >= MAX_CLIENT)
	{
	  syslog (LOG_ERR, "invalid new priority client %i",
		  new_priority_client);
	  errno = EINVAL;
	  return -1;
	}
      // For mechanism with designated priority_server
      // see priority receive.
      // Note that at that point, if some newly connected client is
      // waiting on lock after receiving priority_receive IPC message
      // on startup, designated_priority_client already have other
      // value, since other observation with other client_id is
      // running
      clients_info->designated_priority_client = new_priority_client;
      // change priority only if there is such change
      if (clients_info->priority_client != new_priority_client)
	{
	  // no current priority client => priority_client == -1
	  // we would like to stop some client only if some is
	  if (clients_info->priority_client != -1)
	    {
	      // send stop signal to request thread stopping
	      // on given device
	      // don't forget to include timeout
	      int timeout;
	      if (param_next_integer (params, &timeout))
		return -1;
	      if (devser_2devser_message_format
		  (clients_info->priority_client,
		   "priority_lost %li", timeout))
		return -1;
	    }
	  if (devser_2devser_message
	      (new_priority_client, "priority_receive"))
	    return -1;
	}
      else
	{
	  syslog (LOG_INFO,
		  "same old and new priority clients, don't change it : %i",
		  new_priority_client);
	}
      return 0;
    }
}

int
server_message_handler (struct param_status *params)
{
  char *command = params->param_argv;
  if (strcmp (command, "priority") == 0
      || strcmp (command, "priority_deferred") == 0)
    return server_message_priority (params);

  printf ("command: %s\n", command);
  fflush (stdout);
  return 0;
}

/*!
 * On exit handler.
 */
void
devdem_on_exit ()
{
  if (!devser_child_pid)
    {
      if (shmdt (statutes))
	syslog (LOG_ERR, "shmdt: %m");
      if (shmctl (status_shm, IPC_RMID, NULL))
	syslog (LOG_ERR, "IPC_RMID status_shm shmctl: %m");
      if (semctl (status_sem, 1, IPC_RMID))
	syslog (LOG_ERR, "IPC_RMID status_sem semctl: %m");
      if (shmdt (clients_info))
	syslog (LOG_ERR, "shmdt clients_info: %m");
    }
  else
    {
      if (clients_info->priority_client == client_id && client_id >= 0)	// we have priority and we exit => we must give up priority
	client_priority_lost ();
      clients_info->clients[client_id].pid = 0;
    }
  syslog (LOG_INFO, "exiting");
}

/*!
 * Init devdem internals.
 *
 * Calls devser_init, ..
 * 
 * @param status_names 	names of status, which are part of process status space
 * @param status_num_in	number of status_names
 */
int
devdem_init (char **status_names, int status_num_in)
{
  status_t *st;
  union semun sem_un;
  int i;

  // first of all init devser
  if (devser_init (sizeof (struct client_info)))
    return -1;

  clients_info = devser_shm_data_at ();
  memset (clients_info, 0, sizeof (struct client_info));
  clients_info->priority_client = -1;
  clients_info->designated_priority_client = -1;
  // don't detach clients_info - we will require it during
  // IPC message processing (at this process, not at childrens)

  // creates semaphore for control of status_shm
  // for client we have some two extras - priority (one semaphore -
  // lock) and message receive lock (MAX_CLIENT semaphores)
  status_num = status_num_in + 1;

  if ((status_sem = semget (IPC_PRIVATE, status_num + MAX_CLIENT, 0644)) < 0)
    // continue after #endif
    {
      syslog (LOG_ERR, "status semget: %m");
      return -1;
    }
  // initializes shared memory
  if ((status_shm =
       shmget (IPC_PRIVATE, sizeof (status_t) * status_num, 0644)) < 0)
    {
      syslog (LOG_ERR, "shmget: %m");
      return -1;
    }

  // writes names and null to shared memory
  if ((int) (statutes = (status_t *) shmat (status_shm, NULL, 0)) < 0)
    {
      syslog (LOG_ERR, "shmat: %m");
      return -1;
    }

  st = statutes;
  sem_un.val = 1;
  for (i = 0; i < status_num_in; i++, st++)
    {
      strncpy (st->name, status_names[i], STATUSNAME);
      st->status = 0;
      if (semctl (status_sem, i, SETVAL, sem_un))
	{
	  syslog (LOG_ERR, "status_sem # %i semctl: %m", i);
	  return -1;
	}
    }
  strncpy (st->name, "priority_lock", STATUSNAME);
  st->status = 0;
  client_status = i;
  i++;
  semctl (status_sem, client_status, SETVAL, sem_un);

  // init authorization semaphores
  // those beast will have value 2
  sem_un.val = 2;
  for (; i < status_num + MAX_CLIENT; i++)
    {
      if (semctl (status_sem, i, SETVAL, sem_un))
	{
	  syslog (LOG_ERR, "status_sem # %i semctl: %m", i);
	  return -1;
	}
    }
  // don't detach shm segment, since we will need it
  // in IPC message receivers to set priority flag

  atexit (devdem_on_exit);

  return 0;
}

/*!
 * Make connection to central server.
 *
 * Connect to central server, register to central server, sets
 * functions for message processing.
 *
 * Return after connection is sucessfully initialized, or connection cannot be made.
 *
 * @param device_name		device name as reported on central
 * 				server
 * @param device_type		device type - use predefined constants
 * @param server_host		central server address
 * @param server_port		central server port
 * @param device_type		device type; @see ../status.h for detail
 * @param device_host		hostname of device computer
 * @param device_port		port on which I will be listenig
 *
 * @return 0 on success, -1 and set errno on error
 */
int
devdem_register (char *server_host, uint16_t server_port,
		 char *device_name, int device_type, char *device_host,
		 uint16_t device_port)
{
  struct devcli_channel_handlers handlers;
  handlers.command_handler = server_command_handler;
  handlers.message_handler = server_message_handler;

  handlers.data_handler = NULL;

  /* connect to the server */
  if (devcli_server_register
      (server_host, server_port, device_name, device_type, device_host,
       device_port, &handlers) < 0)
    return -1;

  devser_set_server_id (SERVER_CLIENT, server_handle_msg);
  return 0;
}

int
child_init (void)
{
  // attach shared memory with status
  if ((int) (statutes = (status_t *) shmat (status_shm, NULL, 0)) < 0)
    {
      syslog (LOG_ERR, "shmat: %m");
      return -1;
    }

  // attach shared memory with clients_info
  if ((int) (clients_info = (struct client_info *) devser_shm_data_at ()) < 0)
    {
      syslog (LOG_ERR, "shmat: %m");
      return -1;
    }
  return 0;
}

/*! 
 * Run device daemon.
 *
 * This function will run the device deamon on given port, and will
 * call handler for every command it received.
 * 
 * @param port 		port to run device deamon
 * @param in_handler 	address of command handler code
 * 
 * @return 0 on success, -1 and set errno on error
 */
int
devdem_run (uint16_t port, devser_handle_command_t in_handler)
{
  cmd_device_handler = in_handler;
  return devser_run (port, client_handle_commands, child_init);
}
