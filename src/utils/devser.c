/*! 
 * @file Device daemon skeleton code.
 * 
 * $Id$
 * Implements TCP/IP server, calling hander for every command it
 * receives.
 * <p/>
 * Command string is defined as follow:
 * <ul>
 * 	<li>commands ::= com | com + ';' + commands</li>
 * 	<li>com ::= name | name + ' '\+ + params</li>
 * 	<li>params ::= par | par + ' '\+ + params</li>
 * 	<li>par ::= hms | decimal | integer</li>
 * </ul>
 * In dev deamon are implemented all splits - for ';' and ' '.
 * Device deamon should not implement any splits.
 * <p/>
 * Also provides support functions - status access, thread management.
 *
 * @author petr
 */

#define _GNU_SOURCE

#include "devser.h"
#include "../status.h"

#include "devconn.h"

#ifdef DEVDEM_WITH_CLIENT
#include "devcli.h"
#endif /* DEVDEM_WITH_CLIENT */

#include "hms.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <mcheck.h>
#include <math.h>
#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include <argz.h>

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

#ifdef DEVDEM_WITH_CLIENT

//! client number
int client_id = -1;

//! information about running devices; held in shared memory
struct client_info
{
  int priority_client;		// client with maximal priority
  struct
  {
    pid_t pid;
  }
  clients[MAX_CLIENT];
};

//! shm to hold client_info
int client_shm;

//! status of priorities - set || unset
int client_status;

struct client_info *clients_info;	// mapped to shared memory

#endif /* DEVDEM_WITH_CLIENT */

//! shared memory for clients

//! Structure holding information about socket buffer processing.
struct
{
  char buf[MAXMSG + 1];
  char *endptr;
}
command_buffer;

//! Handler functions
devser_handle_command_t cmd_handler;
devser_handle_message_t msg_handler;

pid_t devser_parent_pid;	//! pid of parent process - process which forks response children
pid_t devser_child_pid;		//! pid of child process - actual pid of main thread

//! Struct for data connection
typedef struct
{
  pthread_mutex_t lock;		//! locking mutex - we allow only one connection
  pthread_t thread;		//! thread running that data processing
  struct sockaddr_in my_side;	//! my side of connection
  struct sockaddr_in their_side;	//! the other side of connection
  void *data_ptr;		//! pointer to data to send
  size_t data_size;		//! total data size
  int sock;			//! data socket
}
data_conn_info_t;

data_conn_info_t data_cons[MAXDATACONS];

//! filedes of ascii control channel
int control_fd;

#define MAX_THREADS	30

//! threads management type
struct
{
  pthread_mutex_t lock;		//! lock to lock threads
  pthread_t thread;		//! thread
}
threads[MAX_THREADS];

// hold number of active threads 
int threads_count;
// protect threads_count
pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;
// cond to wait for threads_count becoming 0 
pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;

//! threads management helper structure
struct thread_wrapper_temp
{
  pthread_mutex_t *lock;
  void *arg;
  void *(*start_routine) (void *);
  int freed;			//! 2 free || !2 free arg on end.
  devser_thread_cleaner_t clean_cancel;
};

//! number of status registers
int status_num;

//! key to shared memory segment, holding status flags.
int status_shm;

//! key to semaphore for access control to shared memory segment
int status_sem;

//! data shared memory
int data_shm;

//! data semaphore to control write access
int data_sem;

//! IPC message que identifier
int msg_id;

//! IPC message receive thread
pthread_t msg_thread;

//! holds status informations
typedef struct
{
  char name[STATUSNAME + 1];
  int status;
}
status_t;

//! array holding status structures; stored in shared mem, hence just pointer
status_t *statutes;

//! actuall processing parameter
char *param_argv;
char *param_processing;
size_t param_argc;

//! block priority calls
pthread_mutex_t priority_block = PTHREAD_MUTEX_INITIALIZER;

void *msg_receive_thread (void *arg);

/*! 
 * Locks given status lock.
 *
 * @param subdevice 	subdevice index
 *
 * @return 0 on success, -1 and set errno on failure; log failure to syslog
 */
int
status_lock (int subdevice)
{
  struct sembuf sb;

  if (subdevice >= status_num || subdevice < 0)
    {
      syslog (LOG_ERR, "invalid device number: %i", subdevice);
      errno = EINVAL;
      return -1;
    }

  sb.sem_num = subdevice;
  sb.sem_op = -1;
  sb.sem_flg = SEM_UNDO;
  if (semop (status_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "semaphore -1 for subdevice %i: %m", subdevice);
      return -1;
    }
  return 0;
}

int
status_unlock (int subdevice)
{
  struct sembuf sb;

  if (subdevice >= status_num || subdevice < 0)
    {
      errno = EINVAL;
      return -1;
    }

  sb.sem_num = subdevice;
  sb.sem_op = 1;
  sb.sem_flg = SEM_UNDO;
  if (semop (status_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "semphore +1 for subdevice %i: %m", subdevice);
      return -1;
    }
  return 0;
}

/*! 
 * Printf to descriptor, log to syslogd, adds \r\n to message end.
 * 
 * @param format buffer format
 * @param ... thinks to print 
 * @return -1 and set errno on error, 0 otherwise
 */
int
devser_dprintf (const char *format, ...)
{
  int ret;
  va_list ap;
  char *msg, *msg_with_end;
  int count, count_with_end;

  va_start (ap, format);
  vasprintf (&msg, format, ap);
  va_end (ap);

  count = strlen (msg);
  count_with_end = count + 2;
  msg_with_end = (char *) malloc (count_with_end + 1);
  if (memcpy (msg_with_end, msg, count) < 0)
    {
      syslog (LOG_ERR, "memcpy in dprintf: %m");
      return -1;
    }
  free (msg);
  // now msg is used as help pointer!!!
  msg = msg_with_end + count;
  *msg = '\r';
  msg++;
  *msg = '\n';
  msg++;
  *msg = 0;

  if ((ret =
       write (control_fd, msg_with_end, count_with_end)) != count_with_end)
    {
      syslog (LOG_ERR,
	      "devser_write: ret:%i count:%i desc:%i error:%m msg_with_end:%s",
	      ret, count_with_end, control_fd, msg_with_end);
      free (msg_with_end);
      return -1;
    }
  syslog (LOG_DEBUG, "%i bytes ('%s') wrote to desc %i", count_with_end,
	  msg_with_end, control_fd);
  free (msg_with_end);
  return 0;
}

int
status_message (int subdevice, char *description)
{
  status_t *st;
  st = &(statutes[subdevice]);
  return devser_dprintf ("M %s %i %s", st->name, st->status, description);
}

/*! 
 * Send status message through channel, log it.
 *
 * @param subdevice 	subdevice index
 * @param description 	message description, could be null
 *
 * @return -1 and set errno on error, 0 otherwise.
 */
int
devser_status_message (int subdevice, char *description)
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
 * Write ending message to fd. 
 *
 * Uses vasprintf to format message.
 * 
 * @param retc 		return code
 * @param msg_format	message to write
 * @param ...		parameters to msg_format
 *
 * @return -1 and set errno on error, 0 otherwise
 */
int
devser_write_command_end (int retc, char *msg_format, ...)
{
  va_list ap;
  int ret, tmper;
  char *msg;

  va_start (ap, msg_format);
  vasprintf (&msg, msg_format, ap);
  va_end (ap);

  ret = devser_dprintf ("%+04i %s", retc, msg);
  tmper = errno;
  free (msg);
  errno = tmper;
  return ret;
}

/*!
 * Decrease thread lock, unlock given thread excutin log
 *
 * @param arg		(pthread_mutex_t *) lock to unlock
 */
void
threads_count_decrease (void *arg)
{
  pthread_mutex_unlock ((pthread_mutex_t *) arg);

  pthread_mutex_lock (&threads_mutex);
  threads_count--;
  if (threads_count == 0)
    pthread_cond_broadcast (&threads_cond);
  pthread_mutex_unlock (&threads_mutex);
}

/*! 
 * Internall call.
 */
void *
thread_wrapper (void *temp)
{
#define TW ((struct thread_wrapper_temp *) temp)	// THREAD WRAPPER
  void *ret;

  pthread_mutex_lock (&threads_mutex);
  threads_count++;
  pthread_mutex_unlock (&threads_mutex);

  pthread_cleanup_push (threads_count_decrease, (void *) (TW->lock));

  if (TW->clean_cancel)
    {
      pthread_cleanup_push (TW->clean_cancel, TW->arg);
      ret = TW->start_routine (TW->arg);
      pthread_cleanup_pop (0);	// of course don't execute that one, end already takes care of it
    }
  else
    ret = TW->start_routine (TW->arg);

  pthread_cleanup_pop (1);	// mutex unlock, threads decrease

  syslog (LOG_DEBUG, "thread successfully ended");

  if (TW->freed)
    free (TW->arg);
  free (temp);
  return ret;
#undef TW
}

/*! 
 * Start new thread.
 * 
 * Inspired by pthread_create LinuxThread calls, add some important things
 * for thread management.
 *
 * All deamons are required to use this function to start new thread for
 * paraller processing of methods, which have to terminate when control over
 * device is seized by other network connection.
 *
 * It's not good idea to use pthread or other similar calls directly from
 * deamon. You should have really solid ground for doing that. 
 *
 * @param start_routine
 * @param arg 		argument
 * @param arg_size	argument size; 0 if no argument
 * @param id 		if not NULL, returns id of thread, which can by used for
 * 			@see pthread_cancel_thread (int)
 * @param clean_success	called on succesfull end of execution to clean-up any lock(s),..; if NULL, ignored
 * @param clean_cancel	called on cancelation, for the same purpose as clean_success; if NULL, ignored
 *
 * @return -1 and set errno on error, 0 otherwise. 
 */
int
devser_thread_create (void *(*start_routine) (void *), void *arg,
		      size_t arg_size, int *id,
		      devser_thread_cleaner_t clean_cancel)
{
  int i;
  for (i = 0; i < MAX_THREADS; i++)
    {
      if (pthread_mutex_trylock (&threads[i].lock) == 0)
	{
	  struct thread_wrapper_temp *temp =
	    (struct thread_wrapper_temp *)
	    malloc (sizeof (struct thread_wrapper_temp));
	  if (!temp)
	    return -1;

	  temp->lock = &threads[i].lock;
	  temp->start_routine = start_routine;
	  if (arg_size)
	    {
	      if (!(temp->arg = malloc (arg_size)))
		return -1;
	      memcpy (temp->arg, arg, arg_size);
	      temp->freed = 1;
	    }
	  else
	    {
	      temp->arg = arg;
	      temp->freed = 0;
	    }
	  if (id)
	    *id = i;

	  temp->clean_cancel = clean_cancel;

	  errno = pthread_create (&threads[i].thread, NULL, thread_wrapper,
				  (void *) temp);
	  return errno ? -1 : 0;
	}
    }
  syslog (LOG_WARNING, "reaches MAX_THREADS");
  errno = EAGAIN;
  return -1;
}

/*! 
 * Stop given thread.
 * 
 * Stops thread with given id.
 *
 * @param id Thread id, as returned by @see devser_thread_create.
 * @retun -1 and set errno on error, 0 otherwise.
 */
int
devser_thread_cancel (int id)
{
  errno = pthread_cancel (threads[id].thread);
  return errno ? -1 : 0;
}

/*! 
 * Stop all threads.
 *
 * @see devser_thread_cancel
 *
 * @return always 0.
 */
int
devser_thread_cancel_all (void)
{
  int i;
  for (i = 0; i < MAX_THREADS; i++)
    {
      pthread_cancel (threads[i].thread);
    }
  return 0;
}

#ifdef DEBUG
/*!
 * Print information about running thread.
 *
 */
int
devser_status ()
{
  int i;
  for (i = 0; i < MAX_THREADS; i++)
    {
      if ((errno = pthread_mutex_trylock (&threads[i].lock)))
	if ((errno = EBUSY))
	  devser_dprintf ("thread %i mutex is locked", i);
	else
	  devser_dprintf ("thread %i error pthread_mutex_trylock %i",
			  i, errno);
      else
	{
	  pthread_mutex_unlock (&threads[i].lock);
	  devser_dprintf ("thread %i unlocked", i);
	}
    }
  devser_dprintf ("threads_count %i", threads_count);

#ifdef DEVDEM_WITH_CLIENT
  devser_dprintf ("client_id %i", client_id);
  devser_dprintf ("priority_client %i", clients_info->priority_client);
  for (i = 0; i < MAX_CLIENT; i++)
    if (clients_info->clients[i].pid)
      devser_dprintf ("client_id_pid %i %i", i, clients_info->clients[i].pid);
#endif /* DEVDEM_WITH_CLIENT */

  return devser_dprintf ("info finished");
}
#endif /* DEBUG */

#ifdef DEVDEM_WITH_CLIENT

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
				"you haven't priority to execute that command");
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

int
client_authorize ()
{
  if (devser_param_test_length (1))
    return -1;
  if (devser_param_next_integer (&client_id))
    return -1;
  if (client_id < 0 || client_id >= MAX_CLIENT)
    {
      devser_write_command_end (DEVDEM_E_SYSTEM, "invalid client number: %i",
				client_id);
      return -1;
    }

  // create message thread
  if (pthread_create (&msg_thread, NULL, msg_receive_thread, NULL) < 0)
    {
      syslog (LOG_ERR, "create message thread: %m");
      return -1;
    }

  clients_info->clients[client_id].pid = devser_child_pid;
  return 0;
}
#endif /* DEVDEM_WITH_CLIENT */

/*! 
 * Handle devser commands.
 *
 * It's solely responsibility of handler to report any errror,
 * which occures during command processing.
 * <p>
 * Errors resulting from wrong command format are reported directly.
 *
 * @param buffer	buffer containing params
 * @return -1 and set errno on network failure|exit command, otherwise 0. 
 */
int
handle_commands (char *buffer)
{
  char *next_command = NULL;
  char *argv;
  size_t argc;
  int ret;
  if ((errno = argz_create_sep (buffer, ';', &argv, &argc)))
    return devser_write_command_end (DEVDEM_E_SYSTEM, "System: ",
				     strerror (errno));
  if (!argv)
    return devser_write_command_end (DEVDEM_E_COMMAND, "empty command!");

  while ((next_command = argz_next (argv, argc, next_command)))
    {
      if ((errno =
	   argz_create_sep (next_command, ' ', &param_argv, &param_argc)))
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM, "argz call error: %s",
				    strerror (errno));
	  ret = -1;
	  goto end;
	}
      if (!param_argv)
	{
	  devser_write_command_end (DEVDEM_E_COMMAND, "empty command!");
	  free (param_argv);
	  ret = -1;
	  goto end;
	}
      syslog (LOG_DEBUG, "handling '%s'", next_command);
      param_processing = param_argv;
      if (strcmp (param_argv, "exit") == 0)
	ret = -2;
#ifdef DEBUG
      else if (strcmp (param_argv, "devser_status") == 0)
	ret = devser_status ();
#endif /* DEBUG */
#ifdef DEVDEM_WITH_CLIENT
      else if (strcmp (param_argv, "auth") == 0)
	ret = client_authorize ();
#endif /* DEVDEM_WITH_CLIENT */
      else
	ret = cmd_handler (param_argv);
      free (param_argv);
      if (ret < 0)
	break;
    }

end:
  free (argv);
  if (ret == -2)
    {
      if (close (control_fd) < 0)
	syslog (LOG_ERR, "close:%m");
      return -1;
    }
  if (!ret)
    devser_write_command_end (0, "Success");
  return 0;
}

/*! 
 * Get next free connection.
 *
 * @return >=0 on succes, -1 and set errno on failure
 */
int
next_free_conn_number ()
{
  int i;
  for (i = 0; i < MAXDATACONS; i++)
    {
      if ((errno = pthread_mutex_trylock (&(data_cons[i].lock))) == 0)
	return i;
    }
  errno = EAGAIN;
  return -1;
}

int
make_data_socket (struct sockaddr_in *server)
{
  int test_port;
  int data_sock;
  /* Create the socket. */
  if ((data_sock = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
      syslog (LOG_ERR, "socket: %m");
      return -1;
    }

  /* Give the socket a name. */
  server->sin_family = AF_INET;
  server->sin_addr.s_addr = htonl (INADDR_ANY);

  for (test_port = MINDATAPORT; test_port < MAXDATAPORT; test_port++)
    {
      server->sin_port = htons (test_port);
      if (bind (data_sock, (struct sockaddr *) server, sizeof (*server)) == 0)
	break;
    }
  if (test_port == MAXDATAPORT)
    {
      syslog (LOG_ERR, "bind: %m");
      return -1;
    }
  return data_sock;
}

void *
send_data_thread (void *arg)
{
  struct timeval send_tout;
  fd_set write_fds;
  data_conn_info_t *data_con;
  int port, ret;
  void *ready_data_ptr, *data_ptr;
  size_t data_size;
  size_t avail_size;

  syslog (LOG_DEBUG, "Sending data thread started.");

  data_con = (data_conn_info_t *) arg;

  port = ntohs (data_con->my_side.sin_port);

  ready_data_ptr = data_ptr = data_con->data_ptr;
  data_size = data_con->data_size;

  while (ready_data_ptr - data_ptr < data_size)
    {
      send_tout.tv_sec = 9000;
      send_tout.tv_usec = 0;
      FD_ZERO (&write_fds);
      FD_SET (data_con->sock, &write_fds);
      if ((ret = select (FD_SETSIZE, NULL, &write_fds, NULL, &send_tout)) < 0)
	{
	  syslog (LOG_ERR, "select: %m port:%i", port);
	  break;
	}
      if (ret == 0)
	{
	  syslog (LOG_ERR, "select timeout port:%i", port);
	  errno = ETIMEDOUT;
	  ret = -1;
	  break;
	}
      avail_size = ((ready_data_ptr + DATA_BLOCK_SIZE) <
		    data_ptr + data_size) ? DATA_BLOCK_SIZE : data_ptr +
	data_size - ready_data_ptr;
      if ((ret = write (data_con->sock, ready_data_ptr, avail_size)) < 0)
	{
	  syslog (LOG_ERR, "write:%m port:%i", port);
	  break;
	}
      syslog (LOG_DEBUG, "on port %i[%i] write %i bytes", port,
	      data_con->sock, ret);
      ready_data_ptr += ret;
    }
  if (close (data_con->sock) < 0)
    syslog (LOG_ERR, "close %i: %m", data_con->sock);

  pthread_mutex_unlock (&data_con->lock);

  if (ret < 0)
    syslog (LOG_ERR, "sending data on port: %i %m", port);
  return NULL;
}

/*! 
 * Connect, initializes and send data to given client.
 * 
 * Quite complex, hopefully quite clever.
 * Handles all task connected with sending data - just past it target address 
 * data to send, and don't care about rest.
 * <p>
 * It sends some messages on control channel, which client should
 * check.
 * 
 * @param in_addr Address to send data; if NULL, we will accept
 * connection request from any address.
 * @param data_ptr Data to send
 * @param data_size Data size
 */
int
devser_send_data (struct in_addr *client_addr, void *data_ptr,
		  size_t data_size)
{
  int port, ret, data_listen_sock;
  struct timeval accept_tout;
  int conn;
  int size;
  data_conn_info_t *data_con;	// actual data connection
  struct sockaddr_in control_socaddr;
  fd_set a_set;

  if ((conn = next_free_conn_number ()) < 0)
    {
      syslog (LOG_INFO, "no new data connection");
      return devser_write_command_end (DEVDEM_E_SYSTEM,
				       "no new data connection");
    }

  data_con = &data_cons[conn];
  data_con->data_ptr = data_ptr;
  data_con->data_size = data_size;

  if ((data_listen_sock = make_data_socket (&data_con->my_side)) < 0)
    return -1;
  port = ntohs (data_con->my_side.sin_port);

  if (listen (data_listen_sock, 1) < 0)
    {
      syslog (LOG_ERR, "listen:%m");
      goto err;
    }

  size = sizeof (control_socaddr);
  if (getsockname (control_fd, (struct sockaddr *) &control_socaddr, &size)
      < 0)
    {
      syslog (LOG_ERR, "getsockaddr:%m");
      goto err;
    }

  if (devser_dprintf
      ("D connect %i %s:%i %i", conn,
       inet_ntoa (control_socaddr.sin_addr), port, data_size) < 0)
    goto err;

  FD_ZERO (&a_set);
  FD_SET (data_listen_sock, &a_set);
  accept_tout.tv_sec = 500;
  accept_tout.tv_usec = 0;

  syslog (LOG_DEBUG, "waiting for connection on port %i", port);
  if ((ret = select (FD_SETSIZE, &a_set, NULL, NULL, &accept_tout)) < 0)
    {
      syslog (LOG_ERR, "select: %m");
      goto err;
    }
  if (ret == 0)
    {
      syslog (LOG_ERR, "select timeout on port %i", port);
      errno = ETIMEDOUT;
      goto err;
    }

  size = sizeof (data_con->their_side);
  if ((data_con->sock =
       accept (data_listen_sock, (struct sockaddr *) &data_con->their_side,
	       &size)) < 0)
    {
      syslog (LOG_ERR, "accept port:%i", port);
      goto err;
    }

  close (data_listen_sock);
  syslog (LOG_INFO, "connection on port %i from %s:%i accepted desc %i",
	  port, inet_ntoa (data_con->their_side.sin_addr),
	  ntohs (data_con->their_side.sin_port), data_con->sock);

  if ((ret =
       pthread_create (&data_con->thread, NULL, send_data_thread, data_con)))
    {
      errno = ret;
      return -1;
    }
  return 0;

err:
  close (data_listen_sock);
  return -1;
}

/*!
 * Create a socket, bind it.
 *
 * Internal function.
 * @param port port number.
 * @return positive socket on success, exit on failure.
 */

int
make_socket (uint16_t port)
{
  int sock;
  struct sockaddr_in name;
  const int so_reuseaddr = 1;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      syslog (LOG_ERR, "socket: %m");
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (setsockopt
      (sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (int)) < 0)
    {
      syslog (LOG_ERR, "setsockopt: %m");
      exit (EXIT_FAILURE);
    }
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      syslog (LOG_ERR, "bind: %m");
      exit (EXIT_FAILURE);
    }
  return sock;
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
devser_status_mask (int subdevice, int mask, int operand, char *message)
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

/*!
 * Attach shared memory data segment.
 *
 * It's size is specified in call to @see devser_run. Exit on any error.
 *
 * @return	pointer to shared data
 */
void *
devser_shm_data_at ()
{
  void *ret = shmat (data_shm, NULL, 0);
  if ((int) ret < 0)
    {
      syslog (LOG_ERR, "devser_shm_data_at shmat: %m");
      exit (EXIT_FAILURE);
    }
  return ret;
}

/*!
 * Lock semaphore for shared data access.
 */
void
devser_shm_data_lock ()
{
  struct sembuf sb;

  sb.sem_num = 0;
  sb.sem_op = -1;
  sb.sem_flg = SEM_UNDO;
  if (semop (data_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "devser_shm_data_lock semphore -1: %m");
      exit (EXIT_FAILURE);
    }
}

/*! 
 * Unlock semaphore for shared data access.
 */
void
devser_shm_data_unlock ()
{
  struct sembuf sb;

  sb.sem_num = 0;
  sb.sem_op = +1;
  sb.sem_flg = SEM_UNDO;
  if (semop (data_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "devser_shm_data_unlock +1: %m");
      exit (EXIT_FAILURE);
    }
}

/*! 
 * Detach shared data memory.
 *
 * Exit on any error.
 */
void
devser_shm_data_dt (void *mem)
{
  if (shmdt (mem) < 0)
    {
      syslog (LOG_ERR, "devser_shm_data_dt: %m");
      exit (EXIT_FAILURE);
    }
}

devser_handle_message_t
devser_msg_set_handler (devser_handle_message_t handler)
{
  devser_handle_message_t old_handler = msg_handler;
  msg_handler = handler;
  return old_handler;
}

/*!
 * Send given IPC message.
 *
 * @see msgsnd.
 *
 * @param msg		pointer to the message buffer to send
 *
 * @return 0 on success, -1 and set errno on error
 */
int
devser_msg_snd (struct devser_msg *msg)
{
  if (msgsnd (msg_id, msg, MSG_SIZE, 0))
    {
      syslog (LOG_ERR, "error devser_msg_snd: %m %i %i", msg_id, msg->mtype);
      return -1;
    }
  // else
  return 0;
}

#ifdef DEVDEM_WITH_CLIENT
void
client_priority_receive ()
{
  status_lock (client_status);

  clients_info->priority_client = client_id;

  devser_status_message (0, "priority received");
}

void
client_priority_lost ()
{
  syslog (LOG_INFO, "calling thread cancel");

  priority_block_start ();
  devser_thread_cancel_all ();
  devdem_priority_block_end ();

  // wait for all threads to finish
  pthread_mutex_lock (&threads_mutex);
  while (threads_count != 0)
    pthread_cond_wait (&threads_cond, &threads_mutex);
  pthread_mutex_unlock (&threads_mutex);

  clients_info->priority_client = -1;

  status_unlock (client_status);

  // inform client
  devser_status_message (0, "priority lost");
}


#endif /* DEVDEM_WITH_CLIENT */

/*!
 * Thread to receive IPC messages.
 *
 * @see devser_thread_create
 *
 * React to some specified messages (priority_lost, priority_receive,
 * ..).
 */
void *
msg_receive_thread (void *arg)
{
  while (1)
    {
      struct devser_msg msg_buf;
      int msg_size;
#ifdef DEVDEM_WITH_CLIENT
      msg_size = msgrcv (msg_id, &msg_buf, MSG_SIZE, client_id + 1, 0);
#else /* DEVDEM_WITH_CLIENT */
      msg_size = msgrcv (msg_id, &msg_buf, MSG_SIZE, devser_child_pid, 0);
#endif /* DEVDEM_WITH_CLINT */
      if (msg_size < 0)
	syslog (LOG_ERR, "devser msg_receive: %m");
      else if (msg_size != MSG_SIZE)
	syslog (LOG_ERR, "invalid message size: %i", msg_size);
      else
	{
	  // testing for null - be cautios, since 
	  // another thread could set null message
	  // handler
	  devser_handle_message_t tmp_handler = msg_handler;

	  msg_buf.mtext[MSG_SIZE - 1] = 0;	// end message (otherwise buffer owerflow could result) 

	  syslog (LOG_DEBUG, "received message %s", msg_buf.mtext);

#ifdef DEVDEM_WITH_CLIENT
	  // some message preprocessing (catchnig important system
	  // messages)
	  if (strcmp (msg_buf.mtext, "priority_receive") == 0)
	    {
	      client_priority_receive ();
	      continue;
	    }

	  if (strcmp (msg_buf.mtext, "priority_lost") == 0)
	    {
	      client_priority_lost ();
	      continue;
	    }
#endif /* DEVDEM_WITH_CLIENT */

	  if (tmp_handler)
	    {
	      syslog (LOG_INFO, "msg received: %s, calling handler",
		      msg_buf.mtext);
	      tmp_handler (msg_buf.mtext);
	    }
	  else
	    {
	      syslog (LOG_WARNING, "ipc message received, null msg handler");
	    }
	}
    }
}

int
devser_param_test_length (int npars)
{
  int actuall_count = argz_count (param_argv, param_argc) - 1;
  if (actuall_count != npars)
    {
      devser_write_command_end (DEVDEM_E_PARAMSNUM,
				"bad nmbr of params: expected %i,got %i",
				npars, actuall_count);
      return -1;
    }
  return 0;
}

/*!
 * Internel to read next string, and report any errors.
 *
 * @return 0 on succes, -1 and set errno on failure.
 */
int
param_next ()
{
  if ((param_processing =
       argz_next (param_argv, param_argc, param_processing)))
    return 0;

  devser_write_command_end (DEVDEM_E_PARAMSVAL,
			    "invalid parameter - got error by argz_next, errno '%i'",
			    errno);
  return -1;
}

int
devser_param_next_string (char **ret)
{
  if (param_next ())
    return -1;

  *ret = param_processing;
  return 0;
}

int
devser_param_next_integer (int *ret)
{
  char *endptr;
  if (param_next ())
    return -1;
  *ret = strtol (param_processing, &endptr, 10);
  if (*endptr)
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"invalid parameter - expected integer, got '%s'",
				param_processing);
      return -1;
    }
  return 0;
}

int
devser_param_next_float (float *ret)
{
  char *endptr;
  if (param_next ())
    return -1;
  *ret = strtof (param_processing, &endptr);
  if (*endptr)
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"invalid parameter - expected integer, got '%s'",
				param_processing);
      return -1;
    }
  return 0;
}

int
devser_param_next_hmsdec (double *ret)
{
  if (param_next ())
    return -1;
  *ret = hmstod (param_processing);
  if (isnan (*ret))
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"expected hms string, got: %s",
				param_processing);
      return -1;
    }
  return 0;
}

int
read_from_client ()
{
  char *buffer;
  int nbytes;
  char *startptr;		// start of message
  char *endptr;			// end of message
  int ret;

  buffer = command_buffer.buf;
  endptr = command_buffer.endptr;
  nbytes = read (control_fd, endptr, MAXMSG - (endptr - buffer));
  if (nbytes < 0)
    {
      /* Read error. */
      syslog (LOG_ERR, "read: %m");
      return -1;
    }
  else if (nbytes == 0)
    /* End-of-file. */
    return -1;
  else
    {
      endptr[nbytes] = 0;	// mark end of message
      startptr = endptr = buffer;
      while (1)
	{
	  if (*endptr == '\r')
	    {
	      *endptr = 0;
	      syslog (LOG_DEBUG, "parsed: '%s'", startptr);
	      if ((ret = handle_commands (startptr)) < 0)
		return ret;
	      endptr++;
	      if (*endptr == '\n')
		endptr++;
	      startptr = endptr;
	    }
	  else if (!*endptr)	// we reach end of message, let's see, if there are some
	    // other bits out there
	    {
	      memmove (buffer, startptr, endptr - startptr + 1);
	      endptr -= startptr - buffer;
	      startptr = buffer;
	      command_buffer.endptr = endptr;
	      return 0;
	    }
	  else
	    endptr++;
	}
    }
}

/*! 
 * On exit handler.
 */
void
devser_on_exit (int err, void *args)
{
  if (getpid () == devser_parent_pid)
    {
#ifdef DEVDEM_WITH_CLIENT
      if (shmdt (statutes))
	syslog (LOG_ERR, "shmdt: %m");
      if ((shmdt (clients_info)))
	syslog (LOG_ERR, "shmdt clients_info: %m");
      if (shmctl (client_shm, IPC_RMID, NULL))
	syslog (LOG_ERR, "IPC_RMID client_shm shmctl: %m");
#endif /* DEVDEM_WITH_CLIENT */
      if (shmctl (status_shm, IPC_RMID, NULL))
	syslog (LOG_ERR, "IPC_RMID status_shm shmctl: %m");
      if (semctl (status_sem, 1, IPC_RMID))
	syslog (LOG_ERR, "IPC_RMID status_sem semctl: %m");
      if (shmctl (data_shm, IPC_RMID, NULL))
	syslog (LOG_ERR, "IPC_RMID data_shm shmctl: %m");
      if (semctl (data_sem, 1, IPC_RMID))
	syslog (LOG_ERR, "IPC_RMID data_sem semctl: %m");
      if (msgctl (msg_id, IPC_RMID, NULL))
	syslog (LOG_ERR, "IPC_RMID msg_id: %m");
    }
#ifdef DEVDEM_WITH_CLIENT
  else if (getpid () == devser_child_pid)
    {
      if (clients_info->priority_client == client_id)	// we have priority and we exit => we must give up priority
	client_priority_lost ();
      clients_info->clients[client_id].pid = 0;
    }
#endif /* DEVDEM_WITH_CLIENT */
  syslog (LOG_INFO, "exiting");
}

/*! 
 * Signal handler.
 */
void
sig_exit (int sig)
{
  syslog (LOG_INFO, "exiting with signal:%i", sig);
  exit (0);
}

#ifdef DEVDEM_WITH_CLIENT

/*!
 *
 */
int
server_command_handler (char *ret)
{
  printf ("returned: %s\n", ret);
  fflush (stdout);
  return 0;
}

/*!
 * Handle message with changing priority, comming from central server.
 *
 * Have to send priority_lost IPC message to currently active devser
 * (if any), wait till all threads are killed, change actual priority 
 * and send priority_receive message to new devser process.
 *
 * @param priority		string with priority setting (as
 * 				received from the served)
 *
 * @return 0 on success, -1 and set errno otherwise
 */
int
server_message_handle_priority (char *priority)
{
  int new_priority_client;
  char *int_end;
  // honour priority request
  priority += 8;		// strlen ("priority");
  while (*priority && *priority == ' ')
    priority++;
  // get new priority client from message
  new_priority_client = strtol (priority, &int_end, 10);
  if (!*int_end || *int_end == ' ')
    {
      struct devser_msg msg;
      if (new_priority_client < 0 || new_priority_client >= MAX_CLIENT)
	{
	  syslog (LOG_ERR, "invalid new priority client %i",
		  new_priority_client);
	  errno = EINVAL;
	  return -1;
	}
      // change priority only if there is such change
      if (clients_info->priority_client != new_priority_client)
	{
	  // no current priority client => priority_client == -1
	  // we would like to stop some client only if some is
	  if (clients_info->priority_client != -1)
	    {
	      // send stop signal to request thread stopping
	      // on given device
	      msg.mtype = clients_info->priority_client + 1;
	      strcpy (msg.mtext, "priority_lost");
	      if (devser_msg_snd (&msg))
		return -1;
	    }
	  msg.mtype = new_priority_client + 1;
	  strcpy (msg.mtext, "priority_receive");
	  if (devser_msg_snd (&msg))
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
  else
    {
      syslog (LOG_ERR,
	      "while processing priority request - invalid number: %s",
	      priority);
      return -1;
    }
}

int
server_message_handler (char *ret)
{
  char *priority;
  if ((priority = strstr (ret, "priority")))
    return server_message_handle_priority (priority);
  printf ("message: %s\n", ret);
  fflush (stdout);
  return 0;
}

/*!
 * Make connection to central server.
 *
 * Fork, connect to central server, register to central server and
 * provide hooks for central server.
 *
 * Exit after connection is sucessfully initialized, or connection cannot be made.
 *
 * @param server_channel	central server channel
 * @param server_address	central server address
 * @param server_port		central server port
 *
 * @return 0 on success, -1 and set errno on error
 */
int
devdem_register (struct devcli_channel *server_channel, char *device_name,
		 char *server_address, int server_port)
{
  char *cmd;
  server_channel->command_handler = server_command_handler;
  server_channel->message_handler = server_message_handler;

  server_channel->data_handler = NULL;

  /* connect to the server */
  if (devcli_connect (server_channel, server_address, server_port) < 0)
    return -1;

  asprintf (&cmd, "register %s", device_name);

  if (devcli_command (server_channel, cmd) < 0)
    {
      free (cmd);
      return -1;
    }

  free (cmd);
  return 0;
}

#endif /* DEVDEM_WITH_CLIENT */

/*! 
 * Run device daemon.
 *
 * This function will run the device deamon on given port, and will
 * call handler for every command it received.
 * 
 * @param port 		port to run device deamon
 * @param in_handler 	address of command handler code
 * @param status_names 	names of status, which are part of process status space
 * @param status_num_in	number of status_names
 * @param shm_data_size	size for shared memory for data; =0 if shared data
 * 			memory is not required
 * 
 * @return 0 on success, -1 and set errno on error
 */
int
devser_run (int port, devser_handle_command_t in_handler,
	    char **status_names, int status_num_in, size_t shm_data_size)
{
  int sock, i;
  struct sockaddr_in clientname;
  size_t size;
  status_t *st;
  union semun sem_un;
  devser_parent_pid = getpid ();

  if (!in_handler)
    {
      errno = EINVAL;
      return -1;
    }
  cmd_handler = in_handler;
  msg_handler = NULL;

#ifdef DEVDEM_WITH_CLIENT
  status_num = status_num_in + 1;
#else /* DEVDEM_WITH_CLIENT */
  status_num = status_num_in;
#endif /* DEVDEM_WITH_CLIENT */
  // creates semaphore for control to shm
  if ((status_sem = semget (IPC_PRIVATE, status_num, 0644)) < 0)
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
#ifdef DEVDEM_WITH_CLIENT
  for (i = 0; i < status_num - 1; i++, st++)
#else /* DEVDEM_WITH_CLIENT */
  for (i = 0; i < status_num; i++, st++)
#endif /* DEVDEM_WITH_CLIENT */
    {
      strncpy (st->name, status_names[i], STATUSNAME + 1);
      st->status = 0;
      semctl (status_sem, i, SETVAL, sem_un);
    }
#ifdef DEVDEM_WITH_CLIENT
  strncpy (st->name, "priority_lock", STATUSNAME + 1);
  st->status = 0;
  semctl (status_sem, i, SETVAL, sem_un);
  // don't detach shm segment, since we will need it
  // in IPC message receivers to set priority flag
#else /* DEVDEM_WITH_CLIENT */
  // detach status shared memory
  if (shmdt (statutes))
    {
      syslog (LOG_ERR, "shmdt: %m");
      return -1;
    }
#endif /* DEVDEM_WITH_CLIENT */

  // initialize data shared memory
  if (shm_data_size > 0)
    {
      void *shmdata;
      if ((data_shm = shmget (IPC_PRIVATE, shm_data_size, 0644)) < 0)
	{
	  syslog (LOG_ERR, "shm_data shmget: %m");
	  return -1;
	}
      // attach shared segment
      shmdata = devser_shm_data_at ();
      // null shared segment
      memset (shmdata, 0, shm_data_size);
      // detach shared segment
      devser_shm_data_dt (shmdata);
    }
  else
    {
      data_shm = -1;
    }

  // creates semaphore for control to data shm
  if ((data_sem = semget (IPC_PRIVATE, 1, 0644)) < 0)
    {
      syslog (LOG_ERR, "data shm semget: %m");
      return -1;
    }

  // initialize data shared memory semaphore
  semctl (data_sem, 0, SETVAL, sem_un);

#ifdef DEVDEM_WITH_CLIENT

  // initializes shared memory
  if ((client_shm =
       shmget (IPC_PRIVATE, sizeof (struct client_info), 0644)) < 0)
    {
      syslog (LOG_ERR, "shmget client_shm: %m");
      return -1;
    }

  // attach
  if ((clients_info = (struct client_info *) shmat (client_shm, NULL, 0)) < 0)
    {
      syslog (LOG_ERR, "shmat clients_info: %m");
      return -1;
    }

  client_status = status_num - 1;

  // init
  memset (clients_info, 0, sizeof (struct client_info));
  clients_info->priority_client = -1;
  // don't detach clients_info - we will require it during
  // IPC message processing (at this process, not at childrens)

#endif /* DEVDEM_WITH_CLIENT */

  // initialize ipc msg que
  if ((msg_id = msgget (IPC_PRIVATE, 0644)) < 0)
    {
      syslog (LOG_ERR, "devser msgget: %m");
      return -1;
    }

  /* register on_exit */
  on_exit (devser_on_exit, NULL);
  signal (SIGTERM, sig_exit);
  signal (SIGQUIT, sig_exit);
  signal (SIGINT, sig_exit);

  /* create the socket and set it up to accept connections. */
  sock = make_socket (port);
  if (listen (sock, 1) < 0)
    {
      syslog (LOG_ERR, "listen: %m");
      return -1;
    }

  syslog (LOG_INFO, "started");

  while (1)
    {
      size = sizeof (clientname);
      if ((control_fd =
	   accept (sock, (struct sockaddr *) &clientname, &size)) < 0)
	{
	  syslog (LOG_ERR, "accept: %m");
	  if (errno != EBADF)
	    return -1;
	}

      if (fork () == 0)
	{
	  // child
	  // (void) dup2 (control_fd, 0);
	  // (void) dup2 (control_fd, 1);
	  close (sock);
	  break;
	}
      // parent 
      close (control_fd);
    }

  // here starts child processing - mind of differences in ipc id's,
  // pointers,..

  command_buffer.endptr = command_buffer.buf;
  syslog (LOG_INFO, "server: connect from host %s, port %hd, desc:%i",
	  inet_ntoa (clientname.sin_addr),
	  ntohs (clientname.sin_port), control_fd);

  devser_child_pid = getpid ();
  // initialize data_cons
  for (sock = 0; sock < MAXDATACONS; sock++)
    {
      pthread_mutex_init (&data_cons[sock].lock, NULL);
    }

  // initialize threads lock
  for (sock = 0; sock < MAX_THREADS; sock++)
    {
      pthread_mutex_init (&threads[sock].lock, NULL);
    }

  // attach shared memory with status
  if ((int) (statutes = (status_t *) shmat (status_shm, NULL, 0)) < 0)
    {
      syslog (LOG_ERR, "shmat: %m");
      return -1;
    }

#ifdef DEVDEM_WITH_CLIENT
  // attach shared memory with clients_info
  if ((int)
      (clients_info = (struct client_info *) shmat (client_shm, NULL, 0)) < 0)
    {
      syslog (LOG_ERR, "shmat: %m");
      return -1;
    }
#endif /* DEVDEM_WITH_CLIENT */

#ifndef DEVDEM_WITH_CLIENT
  // create message thread
  if (pthread_create (&msg_thread, NULL, msg_receive_thread, NULL) < 0)
    {
      syslog (LOG_ERR, "create message thread: %m");
      return -1;
    }
#endif /* !DEVDEM_WITH_CLIENT */

  // null threads count
  threads_count = 0;

  while (1)
    {
      if (read_from_client () < 0)
	{
	  close (control_fd);
	  syslog (LOG_INFO, "connection from desc %d closed", control_fd);
	  exit (EXIT_FAILURE);
	}
    }
  exit (0);
}
