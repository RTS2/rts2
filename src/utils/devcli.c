/*!
 * @file Client library
 * $Id$
 *
 * Provides basic functions for communicating with a server.
 * Basically use @see handle_command.
 * 
 * @author petr
 */

#define _GNU_SOURCE

#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <argz.h>
#include <mcheck.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#include "devcli.h"
#include "devconn.h"
#include "param.h"
#include "status.h"
#include "thread_attr.h"

#define DEVICE_TIMEOUT		120	// default device timeout

pid_t main_pid;

//! Structure to hold parameters for read_data thread.
struct read_data_attrs
{
  int socket;
  devcli_handle_data_t data_handler;
  size_t size;
  int id;
  struct image_info image;
  void *data_handler_args;
};

//! holds mutex and thread for running data connections
struct
{
  pthread_t thread;
  pthread_mutex_t mutex;
}
data_threads[MAXDATACONS];

//! info about status message processing
struct status_message_processing
{
  struct device *dev;		// affected device
  struct devconn_status *status;	// affected status
  int status_val;		// new device value
};

struct serverd_info *info;

struct device server_device;

/*!
 * Find status for given device.
 *
 * @param dev			given device
 * @param status_name		name of status
 * @param st			founded status
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int
status_find (struct device *dev, char *status_name,
	     struct devconn_status **st)
{
  int i;
  for (i = 0; i < dev->status_num; i++)
    {
      *st = &dev->statutes[i];
      if ((*st)->name && !strcmp ((*st)->name, status_name))
	return 0;
    }
  return -1;
}

/*!
 *
 * @return -1 and set errno on error - ENOENT in case there is no such host.
 */
int
init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port)
{
  struct hostent *host_info;

  name->sin_family = AF_INET;
  name->sin_port = htons (port);
  if ((host_info = gethostbyname (hostname)))
    {
      name->sin_addr = *(struct in_addr *) host_info->h_addr;
      return 0;
    }
  errno = ENOENT;
  return -1;
}

/*!
 * Internall, read data threads.
 */
void *
read_data_thread (void *arg)
{
#define READ_DATA_ATTRS ((struct read_data_attrs*) arg)
  READ_DATA_ATTRS->data_handler (READ_DATA_ATTRS->socket,
				 READ_DATA_ATTRS->size,
				 &READ_DATA_ATTRS->image,
				 READ_DATA_ATTRS->data_handler_args);
  if (close (READ_DATA_ATTRS->socket) < 0)
    {
#ifdef DEBUG
      perror ("read_data_attrs close socket");
#endif
    }
  pthread_mutex_unlock (&data_threads[READ_DATA_ATTRS->id].mutex);
#undef READ_DATA_ATTRS
  free (arg);
  return NULL;
}

void *
call_notifiers (void *arg)
{
#define STATUS_VAL ((struct status_message_processing*) arg)->status_val
#define DEV	((struct status_message_processing*) arg)->dev
#define ST	((struct status_message_processing*) arg)->status
  if (ST->notifier)
    {
      ST->notifier (STATUS_VAL, ST->status);
    }

  if (DEV->status_notifier)
    DEV->status_notifier (DEV, ST->name, STATUS_VAL);
#undef ST
#undef DEV
  free (arg);
  return NULL;
}

/*
 * Gets called, when status value changed.
 * Initiate processing threads, takes care of conditions
 * broadcast.
 */
void
proces_status_change (struct device *dev, struct devconn_status *st,
		      int status_val, int call_not)
{
  pthread_t thr;
  pthread_attr_t attrs;
  if (call_not)
    {
      struct status_message_processing *status_change;
      status_change =
	(struct status_message_processing *)
	malloc (sizeof (struct status_message_processing));
      status_change->dev = dev;
      status_change->status = st;
      status_change->status_val = status_val;

      thread_attrs_set (&attrs);
      pthread_create (&thr, &attrs, call_notifiers, status_change);
    }
  pthread_mutex_lock (&dev->status_lock);
  st->status = status_val;
  time (&st->last_update);
  pthread_cond_broadcast (&dev->status_cond);
  pthread_mutex_unlock (&dev->status_lock);
}

/*!
 * Receives and parse device informations.
 *
 * @param params	device information array
 *
 * @return 	0 on success, -1 and set errno on error.
 */
int
handle_device_info (struct device *dev, struct param_status *params)
{
  char *str;
  int i;
  if (param_next_string (params, &str))
    return -1;
  // that one comes just from serverd
  // that comes from device
  if (!strcmp (str, "status_num"))
    {
      if (param_get_length (params) != 2 || param_next_integer (params, &i))
	return -1;
      dev->status_num = i;
      free (dev->statutes);
      dev->statutes =
	(struct devconn_status *) malloc (sizeof (struct devconn_status) *
					  dev->status_num);
      for (i = 0; i < dev->status_num; i++)
	{
	  dev->statutes[i].notifier = NULL;
	}
    }
  else if (!strcmp (str, "status"))
    {
      struct devconn_status *st;
      int state, j;
      if (param_get_length (params) != 4 ||
	  param_next_integer (params, &j)
	  || param_next_string (params, &str)
	  || param_next_integer (params, &state))
	return -1;
      st = &dev->statutes[j];
      strncpy (st->name, str, STATUSNAME);
      st->name[STATUSNAME] = 0;
      proces_status_change (dev, st, state, 0);
    }

  return 0;
}

/*!
 * Recieves and handle device status - those beast starting with "S "
 *
 * @param params	message to handle
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int
handle_device_status (struct device *dev, struct param_status *params)
{
  char *status_name;
  struct devconn_status *st;
  int status_val;

  if (param_next_string (params, &status_name)
      || param_next_integer (params, &status_val))
    return -1;

  if (status_find (dev, status_name, &st))
    return -1;
  proces_status_change (dev, st, status_val, 1);
  return 0;
}

/*! 
 * Handle data connection reguest.
 *
 * Parse data connection request string, initiate data connection,
 * call data processing thread, clean (close socket, free mem) and
 * exits.
 * 
 * @param dev       	device to connect on
 * @param params        data connection request params 
 *
 * @return      0 on success, -1 and set errno on error.
 */
int
handle_connect (struct device *dev, struct param_status *params)
{
  int port, id, sock;
  struct sockaddr_in serverdata;
  struct read_data_attrs *read_data_attrs;
  char *hostname;
  size_t data_size;
  char *str;
  pthread_attr_t attrs;
  devcli_handle_data_t data_handler;

  data_handler = dev->data_handler;

  if (!data_handler)
    // handler is null, return error
    return -1;
  if (param_next_string (params, &str))
    return -1;

  if (param_next_integer (params, &id))
    return -1;
  id = 0;
  while (id < MAXDATACONS)
    {
      if (!pthread_mutex_trylock (&data_threads[id].mutex))
	break;
      id++;
    }

  if (id == MAXDATACONS)
    return -1;

  if (param_next_ip_address (params, &hostname, &port))
    goto err;
  if (param_next_integer (params, &data_size))
    goto err;

  if (init_sockaddr (&serverdata, hostname, port) < 0)
    goto err;
  // connect

  if ((sock = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    goto err;
  if (connect (sock, &serverdata, sizeof (serverdata)) < 0)
    goto err;
  if (!
      (read_data_attrs =
       (struct read_data_attrs *) malloc (sizeof (struct read_data_attrs))))
    goto data_err;

  read_data_attrs->socket = sock;
  read_data_attrs->data_handler = data_handler;
  read_data_attrs->size = data_size;
  read_data_attrs->id = id;
  read_data_attrs->data_handler_args = dev->data_handler_args;

  memcpy (&read_data_attrs->image, &dev->channel.handlers.image,
	  sizeof (struct image_info));

  thread_attrs_set (&attrs);

  if ((errno =
       pthread_create (&data_threads[id].thread, &attrs, read_data_thread,
		       (void *) read_data_attrs)))
    goto data_err;

  return 0;
data_err:
  free (read_data_attrs);
err:
  pthread_mutex_unlock (&data_threads[id].mutex);
  return -1;
}

/*!
 * Handle special device communication, e.g. all responses that =~
 * /^\w\s/
 *
 * @param cmd_start		message to parse
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int
handle_special (struct device *dev, char *cmd_start)
{
  struct param_status *params;
  int ret = -1;
  if (param_init (&params, cmd_start, ' '))
    ret = -1;
  // messages - starts with M
  else if (*cmd_start == 'M')
    if (dev->channel.handlers.message_handler)
      ret = dev->channel.handlers.message_handler (params, &dev->info);
    else
      ret = -1;
  // device information - starts with I 
  else if (*cmd_start == 'I')
    ret = handle_device_info (dev, params);
  // status messages - starts with S
  else if (*cmd_start == 'S')
    ret = handle_device_status (dev, params);
  // data commands - starts with D
  else if (*cmd_start == 'D')
    ret = handle_connect (dev, params);
  param_done (params);
  return ret;
}

/*!
 * Parse received response.
 *
 * Read and execute all commands (separated by '\r' or '\n') from buffer.
 * Fill buffer with unparsed command part, return pointer to end of unparsed
 * command part +1.
 *
 * @param buffer        buffer to parse
 * @param channel       channel informations
 *
 * @return NULL and set errno on error, otherwise pointer to end of parsing.
 */
char *
parse_server_response (char *buffer, struct device *dev)
{
  char *pos = buffer;
  char *cmd_start = buffer;
  while (*pos)
    {
      if (*pos == '\r' || *pos == '\n')
	{
	  *pos = 0;		// end of message
	  if (dev->response_handler)
	    dev->response_handler (dev, cmd_start);
	  // parse response
	  // end of command block - starts with + or -
	  if (*cmd_start == '+' || *cmd_start == '-')
	    {
	      char *ret_end;
	      int ret_code;
	      ret_code = strtol (cmd_start, &ret_end, 10);
	      // length must be = 3, e.g only +nnn <something> is valid as response
	      if (ret_end - cmd_start == 4)
		{
		  pthread_mutex_lock (&dev->channel.ret_lock);
		  dev->channel.ret_code = ret_code;
		  pthread_cond_broadcast (&dev->channel.ret_cond);
		  pthread_mutex_unlock (&dev->channel.ret_lock);
		}
	    }
	  else if (cmd_start[1] == ' ')
	    handle_special (dev, cmd_start);
	  // all other stuff - commands
	  else if (*cmd_start)
	    {
	      if (dev->channel.handlers.command_handler)
		{
		  struct param_status *params;
		  if (!param_init (&params, cmd_start, ' '))
		    dev->channel.handlers.command_handler (params,
							   &dev->info);
		  param_done (params);
		}
	    }
	  if (pos[1] == '\n')
	    pos++;

	  cmd_start = pos;
	  cmd_start++;
	}
      pos++;
    }
  memmove (buffer, cmd_start, pos - cmd_start);
  // return pointer to char next to the end of message
  return buffer + (pos - cmd_start);
}

#define DEV ((struct device *) dev)
void
read_thread_finish (void *dev)
{
  fprintf (stderr, "broken connection of device %s, exiting\n", DEV->name);
  // inform clients, that we lose connection and cannot reply
  // to their requests
  close (DEV->channel.socket);
  DEV->channel.socket = -1;
  pthread_mutex_lock (&DEV->channel.ret_lock);
  DEV->channel.ret_code = DEVDEM_E_TIMEOUT;
  pthread_cond_broadcast (&DEV->channel.ret_cond);
  pthread_mutex_unlock (&DEV->channel.ret_lock);
  // all threads waiting for status should be also notified,
  // that something is wrong
  pthread_mutex_lock (&DEV->status_lock);
  pthread_cond_broadcast (&DEV->status_cond);
  pthread_mutex_unlock (&DEV->status_lock);
  fprintf (stderr, "read finished!\n");
  fflush (stderr);
}


/*! 
 * Called as thread to readout channel socket. 
 *
 * @param dev		struct device * with device information
 * @return 		always NULL
 */
void *
read_server_responses (void *dev)
{
#define DEV ((struct device *) dev)
  char buffer[MAXMSG];
  int nbytes;
  char *endptr;			// end of message
  int sock;

  sock = DEV->channel.socket;
  pthread_cleanup_push (read_thread_finish, dev);
  endptr = buffer;
  while (DEV->channel.socket == sock)
    {
      fd_set read_set;
      FD_ZERO (&read_set);
      FD_SET (sock, &read_set);
      nbytes = select (FD_SETSIZE, &read_set, NULL, NULL, NULL);
      if (nbytes < 0)
	break;
      nbytes = read (sock, endptr, MAXMSG - (endptr - buffer));
      if (nbytes <= 0)
	{
	  perror ("read");
	  break;
	}
      endptr[nbytes] = 0;
      endptr = parse_server_response (buffer, DEV);
    }
#undef DEV
  pthread_cleanup_pop (1);	// execute it
  return NULL;
}

int
dev_init ()
{
  info = (struct serverd_info *) &server_device.info;

  main_pid = getpid ();

  // we need to init pthreads mutex there

  info->devices = NULL;
  info->clients = NULL;

  server_device.type = DEVICE_TYPE_SERVERD;

  server_device.channel.handlers.command_handler = NULL;
  server_device.channel.handlers.message_handler = NULL;
  server_device.data_handler = NULL;
  server_device.channel.socket = -1;
  return 0;
}

/*!
 * Init channel, connect to device.
 *
 * Init ret_lock and ret_cond.
 *
 * @param dev		device to connect on
 * @param hostname	hostname to connect on
 * @param port		port to connect on
 *
 * @return	-1 and set errno on error, 0 otherwise. Fill in socket and
 * 		sockaddr in channel. 
 */
int
dev_connect (struct device *dev, const char *hostname, uint16_t port)
{
  int i;
  pthread_attr_t attrs;
  pthread_t a_thread;

  if (dev->channel.socket != -1)
    return 0;			// already connected
  memset (&dev->info, 0, sizeof (dev->info));
  if (init_sockaddr (&dev->channel.address, hostname, port) < 0)
    return -1;
  if ((dev->channel.socket = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    goto err;
  if (connect
      (dev->channel.socket, (struct sockaddr *) &dev->channel.address,
       sizeof (dev->channel.address)) < 0)
    goto err;

  pthread_mutex_init (&dev->channel.ret_lock, NULL);
  pthread_cond_init (&dev->channel.ret_cond, NULL);
  for (i = 0; i < MAXDATACONS; i++)
    pthread_mutex_init (&data_threads[i].mutex, NULL);

  thread_attrs_set (&attrs);

  // run thread to dispatch command returns & messages
  if (pthread_create
      (&a_thread, &attrs, read_server_responses, (void *) dev) != 0)
    goto err;
  return 0;
err:
  if (dev->channel.socket != -1)
    close (dev->channel.socket);
  dev->channel.socket = -1;
  return -1;
}

/*!
 * Handle exit signals..
 *
 */
void
dev_sig_exit (int sig)
{
  devcli_server_disconnect ();
}

/*!
 * Close all server connection.
 *
 * Closes connection 
 */
void
devcli_server_disconnect ()
{
  for (; info->devices; info->devices = info->devices->next)
    {
      devcli_server_close (info->devices);
    }
}

/*!
 * Connect to server, login to server.
 *
 * @param hostname	hostname to connect on
 * @param port		port to connect on
 * @param login		login name
 * @param password	password for authentification
 *
 * @return	-1 and set errno on error, 0 otherwise. Fill in socket and
 * 		sockaddr in channel. 
 */
int
devcli_server_login (const char *hostname,
		     uint16_t port, char *login, char *password)
{
  if (dev_init ())
    return -1;

  server_device.channel.handlers.command_handler =
    devhnd_devices[DEVICE_TYPE_SERVERD].command_handler;
  server_device.channel.handlers.message_handler =
    devhnd_devices[DEVICE_TYPE_SERVERD].message_handler;
  // now we could initiate connection ...
  if (dev_connect (&server_device, hostname, port))
    return -1;
// ... and log in
  if (devcli_server_command (NULL, "login %s", login)
      || devcli_server_command (NULL, "password %s", password)
      || devcli_server_command (NULL, "info"))
    {
      devcli_server_disconnect ();
      return -1;
    }
  return 0;
}

/*!
* Connect to server, just register to server.
*
* @param serv_hostname	hostname to connect on
* @param serv_port	port to connect on
* @param device_name	device name
* @param device_type	device type
* @param device_port	port of the device
* @param handlers
*
* @return	-1 and set errno on error, 0 otherwise. Fill in socket and
* 		sockaddr in server_channel. 
*/
int
devcli_server_register (const char *serv_host,
			uint16_t serv_port, char *device_name,
			int device_type, char *device_host,
			uint16_t device_port,
			struct devcli_channel_handlers *handlers,
			status_notifier_t notifier)
{
  if (dev_init ())
    return -1;

// init handlers..
  if (handlers)
    server_device.channel.handlers = *handlers;

// now we could initiate connection ...
  if (dev_connect (&server_device, serv_host, serv_port))
    return -1;

  signal (SIGTERM, dev_sig_exit);
  signal (SIGQUIT, dev_sig_exit);
  signal (SIGINT, dev_sig_exit);

// ... and log in
  if (devcli_server_command
      (NULL, "register %s %i %s %i", device_name, device_type, device_host,
       device_port))
    {
      devcli_server_disconnect ();
      return -1;
    }

  devcli_set_notifier (&server_device, SERVER_STATUS, notifier);
  if (notifier)
    notifier (server_device.statutes[0].status, -1);

  return 0;
}

devcli_handle_response_t
devcli_set_command_handler (struct device * dev,
			    devcli_handle_response_t command_handler)
{
  devcli_handle_response_t ret = dev->channel.handlers.command_handler;
  dev->channel.handlers.command_handler = command_handler;
  devhnd_devices[dev->type].command_handler = command_handler;
  return ret;
}

void
devcli_server_close (struct device *dev)
{
  close (dev->channel.socket);
  free (dev->statutes);
  dev->channel.socket = -1;
}

struct device *
devcli_server ()
{
  return &server_device;
}

struct device *
devcli_devices ()
{
  return info->devices;
}

struct device *
devcli_find (const char *device_name)
{
  struct device *devices = info->devices;
  devcli_server_command (NULL, "info");
  while (devices)
    {
      if (!strcmp (devices->name, device_name))
	return devices;
      devices = devices->next;
    }
  return NULL;
}

/*!
 * Create new connection to given device, authorize to that device.
 *
 * @param channel_id
 * @param dev_name
 * @param handlers
 *
 * @return 
 */
int
dev_connectdev (struct device *dev)
{
  int ret_code;

  if (dev->channel.socket == -1
      && dev_connect (dev, dev->hostname, dev->port))
    return -1;

  // we don't need to authorize on serverd - code in serverd_login takes care of that
  if (dev->type == DEVICE_TYPE_SERVERD)
    return 0;

  // authentificate to the device
  devcli_server_command (NULL, "key %s", dev->name);

  if (devcli_command
      (dev, &ret_code, "auth %i %i", info->id, dev->key) || ret_code < 0)
    return -1;

  dev->channel.handlers.command_handler =
    devhnd_devices[dev->type].command_handler;
  dev->channel.handlers.message_handler =
    devhnd_devices[dev->type].message_handler;
  return 0;
}

/*!
* Internal - write message to server.
*/
int
write_to_server (int filedes, const char *command)
{
#define END_MSG	"\r\n"
#define END_LENGTH 2
  int nbytes;
  nbytes = strlen (command);
  if (nbytes != write (filedes, command, nbytes))
    return -1;
  if (END_LENGTH != write (filedes, END_MSG, END_LENGTH))
    return -1;
  return 0;
#undef END_LENGTH
#undef END_MSG
}

/*!
* Encapsulated read with select for timeout.
*
* @see read(2), select(2).
*/
ssize_t
devcli_read_data (int sock, void *data, size_t size)
{
  int ret;
  fd_set read_set;
  struct timeval tv;

  while (1)
    {
      FD_ZERO (&read_set);
      FD_SET (sock, &read_set);
      tv.tv_sec = 50;
      tv.tv_usec = 0;
      if ((ret = select (FD_SETSIZE, &read_set, NULL, NULL, &tv)) <= 0)
	break;

      if (FD_ISSET (sock, &read_set))
	{
	  return read (sock, data, size);
	}
      break;
    }
  return -1;
}

/*! 
* Sends command. It expect, that dev->channel.socket is connected.
*
* @param channel
* @param ret_code		command return code	
* @param cmd		 	command to send
* @param va			format parameters
*
* @return -1 and set errno on failure, 0 otherwise
*/
int
dev_command_va (struct device *dev, int *ret_code, char *cmd, va_list va)
{
  char *message = NULL;
  int ret;
  struct timespec abstime;
  sigset_t sigs, old_sigs;

  vasprintf (&message, cmd, va);
  pthread_mutex_lock (&dev->channel.ret_lock);
  dev->channel.ret_code = INT_MIN;
  ret = write_to_server (dev->channel.socket, message);
  free (message);

  if (ret < 0)
    return -1;

  sigemptyset (&sigs);
  pthread_sigmask (SIG_SETMASK, &sigs, &old_sigs);

  abstime.tv_sec = time (NULL) + 60;
  abstime.tv_nsec = 0;
  ret = 0;
  while (dev->channel.ret_code == INT_MIN && ret != ETIMEDOUT)
    ret =
      pthread_cond_timedwait (&dev->channel.ret_cond, &dev->channel.ret_lock,
			      &abstime);
  pthread_sigmask (SIG_SETMASK, &old_sigs, &sigs);

  if (ret_code)
    *ret_code = dev->channel.ret_code;
  if (dev->channel.ret_code < 0)
    ret = -1;
  pthread_mutex_unlock (&dev->channel.ret_lock);
  return ret;
}

/*!
 * Wait till given status condition is satisfied.
 *
 * Status condition si satisfied, if status = device_status &
 * status_mask.
 *
 * @param status_name		status name to wait for
 * @param status_mask		mask to use
 * @param status		status value to wait for
 * @param tmeout		maximal time to wait for; if
 * 0->unlimited
 *
 * @return -1 and set errno on error, 0 otherwise. -1 and ETIMEDOUT on
 * timeout.
 */

int
devcli_wait_for_status (struct device *dev, char *status_name,
			int status_mask, int status, time_t tmeout)
{
  struct devconn_status *st;
  struct timespec abstime;
  // status
  if (status_find (dev, status_name, &st))
    return -1;

  if (tmeout)
    abstime.tv_sec = time (NULL) + tmeout;
  else
    abstime.tv_sec = time (NULL) + DEVICE_TIMEOUT;
  abstime.tv_nsec = 11;

  // try to reconnect devices, that were broken
  if (dev->channel.socket == -1)
    devcli_command (dev, NULL, "ready");

  pthread_mutex_lock (&dev->status_lock);
  while (dev->channel.socket != -1 && status != (st->status & status_mask))
    {
      errno =
	pthread_cond_timedwait (&dev->status_cond, &dev->status_lock,
				&abstime);
      if (errno == ETIMEDOUT && tmeout)
	{
	  pthread_mutex_unlock (&dev->status_lock);
	  return -1;
	}
    }
  pthread_mutex_unlock (&dev->status_lock);
  if (dev->channel.socket == -1)
    return -1;
  return 0;
}

int
devcli_wait_for_status_all (int type, char *status_name, int status_mask,
			    int status, time_t tmeout)
{
  struct device *dev = info->devices;
  int ret = 0;
  while (dev)
    {
      if (dev->type == type)
	{
	  printf ("st: %s dev: %s\n", status_name, dev->name);
	  fflush (stdout);
	  ret |=
	    devcli_wait_for_status (dev, status_name, status_mask, status,
				    tmeout);
	}
      dev = dev->next;
    }
  return ret;
}

void
devcli_set_general_notifier (struct device *dev,
			     general_notifier_t notifier, void *data)
{
  dev->notifier_data = data;
  dev->status_notifier = notifier;
}

int
devcli_set_notifier (struct device *dev, char *status_name,
		     status_notifier_t callback)
{
  struct devconn_status *st;
  if (status_find (dev, status_name, &st))
    return -1;

  st->notifier = callback;
  return 0;
}

int
devcli_server_command (int *ret_code, char *cmd, ...)
{
  va_list va;
  int ret;

  va_start (va, cmd);
  ret = dev_command_va (&server_device, ret_code, cmd, va);
  va_end (va);
  return ret;
}

extern int
devcli_command (struct device *dev, int *ret_code, char *cmd, ...)
{
  va_list va;
  int ret;

  if (dev->channel.socket == -1 && dev_connectdev (dev) == -1)
    return -1;

  va_start (va, cmd);
  ret = dev_command_va (dev, ret_code, cmd, va);
  va_end (va);

  return ret;
}

int
devcli_command_all (int device_type, char *cmd, ...)
{
  va_list va;
  struct device *dev = info->devices;
  int ret = 0;

  va_start (va, cmd);
  for (; dev; dev = dev->next)
    {
      if (dev->type == device_type)
	{
	  if (dev->channel.socket == -1 && dev_connectdev (dev) == -1)
	    continue;
	  printf ("ret: %s %i\n", dev->name, ret);
	  fflush (stdout);
	  ret |= dev_command_va (dev, NULL, cmd, va);
	}
    }
  va_end (va);
  return ret;
}

int
devcli_device_data_handler (int device_type, devcli_handle_data_t handler)
{
  struct device *devices = info->devices;
  int ret = 0;
  while (devices)
    {
      if (devices->type == device_type)
	devices->data_handler = handler;
      devices = devices->next;
    }
  return ret;
}

int
devcli_image_info (struct device *dev, struct image_info *image)
{
  printf ("get image info for:%s\n", image->camera_name);
  memcpy (&dev->channel.handlers.image, image, sizeof (struct image_info));
  return 0;
};

// mainly just for monitor
int
devcli_execute (char *line, int *ret_code)
{
  char *sep, *space;

  space = strchr (line, ' ');
  sep = strchr (line, '.');
  if (sep && (!space || sep < space))
    {
      struct device *dev;
      // device in command, command after sep
      *sep = 0;
      sep++;
      if ((dev = devcli_find (line)))
	return devcli_command (dev, ret_code, sep);
      errno = ENOENT;
      return -1;
    }
  // default-> send it to central server
  return devcli_server_command (ret_code, line);
}

char *
serverd_status_string (int status)
{
  switch (status & SERVERD_STATUS_MASK)
    {
    case SERVERD_DAY:
      return "Day";
    case SERVERD_EVENING:
      return "Evening";
    case SERVERD_DUSK:
      return "Dusk";
    case SERVERD_NIGHT:
      return "Night";
    case SERVERD_DAWN:
      return "Dawn";
    case SERVERD_MORNING:
      return "Morning";
    case SERVERD_OFF:
      return "Off";
    default:
      return "Unknow";
    }
}

extern char *
devcli_status_string (struct device *dev, struct devconn_status *st)
{
  if (!strcmp (st->name, "priority"))
    {
      switch (st->status)
	{
	case 1:
	  return "have";
	default:
	  return "don't";
	}
    }

  switch (dev->type)
    {
    case DEVICE_TYPE_SERVERD:
      if ((st->status & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	return "standby";
      switch (st->status & SERVERD_STATUS_MASK)
	{
	case SERVERD_DAY:
	  return "day";
	case SERVERD_EVENING:
	  return "evening";
	case SERVERD_DUSK:
	  return "dusk";
	case SERVERD_NIGHT:
	  return "night";
	case SERVERD_DAWN:
	  return "dawn";
	case SERVERD_MORNING:
	  return "morning";
	case SERVERD_OFF:
	  return "off";
	}
    case DEVICE_TYPE_CCD:
      switch (st->status)
	{
	case CAM_EXPOSING:
	  return "exp";
	case CAM_DATA:
	  return "data";
	case CAM_READING:
	  return "read";
	case CAM_NOEXPOSURE:
	  return "idle";
	}
      break;
    case DEVICE_TYPE_MOUNT:
      switch (st->status)
	{
	case TEL_MOVING:
	  return "move";
	case TEL_OBSERVING:
	  return "obs";
	case TEL_PARKED:
	  return "park";
	}
    case DEVICE_TYPE_DOME:
      if (!strcmp (st->name, "weather"))
	{
	  switch (st->status)
	    {
	    case DOME_WEATHER_OK:
	      return "good";
	    case DOME_WEATHER_BAD:
	      return "bad";
	    }
	}
      else if (!strncmp (st->name, "dome", 4))
	{
	  switch (st->status)
	    {
	    case DOME_UNKNOW:
	      return "unknow";
	    case DOME_CLOSED:
	      return "closed";
	    case DOME_OPENING:
	      return "opening";
	    case DOME_OPENED:
	      return "opened";
	    case DOME_CLOSING:
	      return "closing";
	    }
	}
      break;
    case DEVICE_TYPE_MIRROR:
      switch (st->status)
	{
	case MIRROR_UNKNOW:
	  return "unknow";
	case MIRROR_A:
	  return "A";
	case MIRROR_A_B:
	  return "A->B";
	case MIRROR_B:
	  return "B";
	case MIRROR_B_A:
	  return "B->A";
	}
      break;
    case DEVICE_TYPE_FOCUS:
      switch (st->status)
	{
	case FOC_FOCUSING:
	  return "focusing";
	case FOC_SLEEPING:
	  return "sleeping";
	}
      break;
    }
  return "unknow";
}
