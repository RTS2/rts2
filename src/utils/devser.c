/*! 
 * @file Device server skeleton code.
 * 
 * $Id$
 * Implements TCP/IP server, calling hander for every command it
 * receives.
 * <p/>
 * In dev server are implemented all splitings - for ';' and ' '.
 * Code using server should not implement any splits.
 * <p/>
 * Provides too some support functions - status access, thread management.
 *
 * @author petr
 */

#define _GNU_SOURCE

#define IPC_MSG_SHIFT		1000

#include "devser.h"
#include "devconn.h"
#include "param.h"
#include "status.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
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
#include <sys/wait.h>

#include <argz.h>

//! data transmission timout in seconds
#define DATA_TIMEOUT	20

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

//! ip of connected client
struct sockaddr_in server_client_ip;

/*! 
 * server number.
 *
 * Must be unique. Used mainly for IPC message mtype.
 */
int server_id = -1;

//! structure holding information about socket buffer processing.
struct
{
  char buf[MAXMSG + 1];
  char *endptr;
}
command_buffer;

//! handler functions
devser_handle_command_t cmd_handler = NULL;
devser_handle_msg_t msg_handler = NULL;

pid_t devser_parent_pid;	//! pid of parent process - process which forks response children
pid_t devser_child_pid = 0;	//! pid of child process - actual pid of main thread

//! struct for data connection
typedef struct
{
  pthread_mutex_t lock;		//! locking mutex - we allow only one connection

  pthread_mutex_t que_lock;	//! lock for ava
  pthread_cond_t write_cond;	//! condition for write, eg. put operation
  pthread_cond_t read_cond;	//! condiction for read, eg. get operation

  pthread_t thread;

  struct sockaddr_in my_side;	//! my side of connection
  struct sockaddr_in their_side;	//! the other side of connection

  void *buffer;			//! point to data buffer; needs to be malloced and freeded
  size_t buffer_size;		//! buffer size
  size_t data_size;		//! total data size
  size_t data_readed;		//! size of data readed

  size_t available;		//! count of putted datas 
  size_t data_write;		//! start of writing - we operate in cyclic buffer
  size_t data_read;		//! start of reading

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

//! structure to hold info about all my childs
struct child
{
  pid_t child_pid;		// ! pid of the child
  struct child *next;
}
 *childrens = NULL;

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

//! data shared memory
int data_shm;

//! data semaphore to control write access
int data_sem;

//! IPC message que identifier
int msg_id;

//! IPC message receive thread
pthread_t msg_thread;

//! Server socket - needed to be global for exit processing
int server_socket;

//! parameter processing status
struct param_status *command_params;

//! devser_2dever message typ
struct devser_msg
{
  long mtype;
  char mtext[MSG_SIZE];
};

void *msg_receive_thread (void *arg);

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
devser_write_command_end (int retc, const char *msg_format, ...)
{
  va_list ap;
  int ret;
  char *msg;

  va_start (ap, msg_format);

  if (vasprintf (&msg, msg_format, ap) < 0)
    return -1;

  va_end (ap);

  ret = devser_dprintf ("%+04i %s", retc, msg);
  free (msg);
  return ret;
}

/*!
 * Decrease thread lock, unlock given thread excutin log
 *
 * @param arg		(pthread_mutex_t *) lock to unlock
 */
void
threads_count_decrease (void *temp)
{
#define TW ((struct thread_wrapper_temp *) temp)	// THREAD WRAPPER
  pthread_mutex_unlock (TW->lock);

  pthread_mutex_lock (&threads_mutex);
  threads_count--;
  if (threads_count == 0)
    pthread_cond_broadcast (&threads_cond);
  pthread_mutex_unlock (&threads_mutex);
  if (TW->freed)
    free (TW->arg);
  free (temp);
  syslog (LOG_DEBUG, "freed temp");
}

/*! 
 * Internall call.
 */
void *
thread_wrapper (void *temp)
{
  void *ret;

  pthread_mutex_lock (&threads_mutex);
  threads_count++;
  pthread_mutex_unlock (&threads_mutex);

  pthread_cleanup_push (threads_count_decrease, (void *) TW);

  if (TW->clean_cancel)
    {
      pthread_cleanup_push (TW->clean_cancel, TW->arg);
      ret = TW->start_routine (TW->arg);
      pthread_cleanup_pop (0);	// of course don't execute that one, end already takes care of it
    }
  else
    ret = TW->start_routine (TW->arg);

  pthread_cleanup_pop (1);	// mutex unlock, threads decrease
#undef TW

  if (ret == 0)
    syslog (LOG_DEBUG, "thread successfully ended");
  else
    syslog (LOG_ERR, "thread ended, %i returned", *(int *) ret);
  return ret;
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
	  if (errno)
	    {
	      if (temp->freed)
		free (temp->arg);
	      free (temp);
	      return -1;
	    }
	  return 0;
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

int
devser_thread_wait (void)
{
  // wait for all threads to finish
  pthread_mutex_lock (&threads_mutex);
  while (threads_count != 0)
    pthread_cond_wait (&threads_cond, &threads_mutex);
  pthread_mutex_unlock (&threads_mutex);

  return 0;
}

#ifdef DEBUG
/*!
 * Print information about running threads.
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

  return devser_dprintf ("server_id %i", server_id);
}
#endif /* DEBUG */

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
  char *command = NULL;

  char *argv;
  size_t argc;
  int ret;

  if ((errno = argz_create_sep (buffer, ';', &argv, &argc)))
    return devser_write_command_end (DEVDEM_E_SYSTEM, "system: ",
				     strerror (errno));
  if (!argv)
    return devser_write_command_end (DEVDEM_E_COMMAND, "empty command!");

  while ((next_command = argz_next (argv, argc, next_command)))
    {
      syslog (LOG_DEBUG, "handling '%s'", next_command);

      if (param_init (&command_params, next_command, ' '))
	{
	  devser_write_command_end (DEVDEM_E_SYSTEM, "argz call error: %s",
				    strerror (errno));
	  ret = -1;
	  goto end;
	}
      if (param_is_empty (command_params))
	{
	  devser_write_command_end (DEVDEM_E_COMMAND, "empty command!");
	  param_done (command_params);
	  ret = -1;
	  goto end;
	}

      command = command_params->param_argv;
      if (strcmp (command, "exit") == 0)
	ret = -2;
#ifdef DEBUG
      else if (strcmp (command, "devser_status") == 0)
	ret = devser_status ();
#endif /* DEBUG */
      else
	ret = cmd_handler (command);

      param_done (command_params);
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

int data_get (int id, void *data, size_t data_size, size_t * size);

void *
send_data_thread (void *arg)
{
#define ID	*((int *) arg)
  struct timeval send_tout;
  fd_set write_fds;
  data_conn_info_t *data_con;
  int port, ret;
  char data[DATA_BLOCK_SIZE];
  size_t size;
  size_t sended = 0;

  syslog (LOG_DEBUG, "Sending data thread started.");

  data_con = &data_cons[ID];

  port = ntohs (data_con->my_side.sin_port);

  while (sended < data_con->data_size
	 && !data_get (ID, data, DATA_BLOCK_SIZE, &size))
    {
      send_tout.tv_sec = DATA_TIMEOUT;
      send_tout.tv_usec = 0;
      FD_ZERO (&write_fds);
      FD_SET (data_con->sock, &write_fds);
      ret = select (FD_SETSIZE, NULL, &write_fds, NULL, &send_tout);
      if (FD_ISSET (data_con->sock, &write_fds))
	{
	  if ((ret = write (data_con->sock, data, size)) != size)
	    {
	      syslog (LOG_ERR, "devser write:%m port:%i ret:%i size:%i", port,
		      ret, size);
	      break;
	    }
	}
      else
	{
	  syslog (LOG_DEBUG, "send_data_thread bad select, ret: %i", ret);
	  data_con->available = -1;
	  // devser_data_invalidate (ID);
	  break;
	}
      sended += size;
    }
  devser_data_done (ID);
  pthread_mutex_unlock (&data_con->lock);

  return NULL;
#undef ID
}

/*! 
 * Initializes connection and its data buffer.
 * 
 * Quite complex, hopefully quite clever. 
 * Handles only part of tasks connected task connected with sending data.
 *
 * @return -1 and set errno on failure, 0 otherwise
 */
int
devser_data_init (size_t buffer_size, size_t data_size, int *id)
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
  data_con->data_size = data_size;
  data_con->data_readed = 0;

  if (data_con->buffer)
    free (data_con->buffer);

  if (!(data_con->buffer = malloc (buffer_size)))
    {
      syslog (LOG_ERR, "devser_data_init data_con->buffer malloc: %m");
      goto err_without_socket;
    }

  data_con->buffer_size = buffer_size;

  data_con->data_read = 0;
  data_con->data_write = 0;

  data_con->available = 0;

  if ((data_listen_sock = make_data_socket (&data_con->my_side)) < 0)
    goto err_without_socket;

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
  accept_tout.tv_sec = DATA_TIMEOUT;
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

  *id = conn;
  if ((errno =
       pthread_create (&(data_con->thread), NULL, send_data_thread, id)))
    {
      syslog (LOG_ERR, "devser_data_init pthread_create: %m");
      goto err;
    }

  return 0;

err:
  data_con->sock = 0;
  close (data_listen_sock);
err_without_socket:
  pthread_mutex_unlock (&(data_con->lock));
  devser_write_command_end (DEVDEM_E_SYSTEM,
			    "error on creating readout-thread");
  return -1;
}

void
devser_data_done (int id)
{
  data_conn_info_t *data_con;
  data_con = &data_cons[id];

  syslog (LOG_DEBUG, "devser_data_done");

  free (data_con->buffer);
  data_con->buffer = NULL;

  close (data_con->sock);
  data_con->sock = 0;
}

/*!
 * Put given data amounts on data buffer.
 *
 * Data must be smaller than buffer size. It isn't thread safe, should
 * be called from only one thread.
 *
 * @param id			data connection id
 * @param data			data to write on que
 * @param size			data size
 *
 * @return -1 and set errno on failure, 0 otherwise
 */
int
devser_data_put (int id, void *data, size_t size)
{
  data_conn_info_t *data_con;
  int overload;			// for cyclic buffer

  data_con = &data_cons[id];

  if (size > data_con->buffer_size)
    {
      errno = EINVAL;
      return -1;
    }

  if ((errno = pthread_mutex_lock (&(data_con->que_lock))))
    {
      syslog (LOG_ERR, "devser_data_put pthread_mutex_lock: %m");
      return -1;
    }

  while (data_con->available > data_con->buffer_size - size)
    pthread_cond_wait (&data_con->write_cond, &data_con->que_lock);

  if (data_con->available == -1)
    {
      pthread_mutex_unlock (&data_con->que_lock);
      errno = EPIPE;
      return -1;
    }

  overload = (data_con->data_write + size) - data_con->buffer_size;

  if (overload < 0)
    overload = 0;

  memcpy (data_con->buffer + data_con->data_write, data, size - overload);
  memcpy (data_con->buffer, data + size - overload, overload);	// mostly overload = 0

  data_con->available += size;

  // move data_write pointer to next free data
  if (overload)
    data_con->data_write = overload;
  else
    data_con->data_write += size;

  pthread_cond_signal (&(data_con->read_cond));
  pthread_cond_signal (&(data_con->write_cond));	// if there is some other thread waiting..

  pthread_mutex_unlock (&data_con->que_lock);
  return 0;
}

/*!
 * Read data from que.
 *
 * It isn't thread safe.  
 *
 * @param id			data connection id
 * @param data			data to read from que
 * @param max_size		maximal data size
 * @param size			actually readed data size	
 *
 * @return -1 and set errno on failure, 0 otherwise
 */
int
data_get (int id, void *data, size_t max_size, size_t * size)
{
  data_conn_info_t *data_con;
  int overload;			// for cyclic buffer

  data_con = &data_cons[id];

  if (max_size > data_con->buffer_size)
    max_size = data_con->buffer_size;

  if ((errno = pthread_mutex_lock (&(data_con->que_lock))))
    {
      syslog (LOG_ERR, "devser_data_put pthread_mutex_lock: %m");
      return -1;
    }

  while (data_con->available == 0)
    pthread_cond_wait (&data_con->read_cond, &data_con->que_lock);

  if (data_con->available == -1)
    {
      // invalidate que
      pthread_mutex_unlock (&data_con->que_lock);
      errno = EPIPE;
      return -1;
    }

  if (data_con->available < max_size)
    max_size = data_con->available;	// min from max_size and data_con->available

  overload = (data_con->data_read + max_size) - data_con->buffer_size;

  if (overload < 0)
    overload = 0;

  memcpy (data, data_con->buffer + data_con->data_read, max_size - overload);
  memcpy (data + max_size - overload, data_con->buffer, overload);	// mostly overload = 0

  data_con->available -= max_size;
  data_con->data_readed += max_size;

  *size = max_size;

  // move data_read pointer
  if (overload)
    data_con->data_read = overload;
  else
    data_con->data_read += max_size;

  pthread_cond_signal (&(data_con->write_cond));
  pthread_cond_signal (&(data_con->read_cond));	// if there is some other thread waiting

  pthread_mutex_unlock (&(data_con->que_lock));

  return 0;
}

/*!
 * Invalidate data que.
 *
 * @param id		data connection id
 *
 * @return -1 and set errno on error, 0 otherwise 
 */
int
devser_data_invalidate (int id)
{
  data_conn_info_t *data_con;
  data_con = &data_cons[id];

  if ((errno = pthread_mutex_lock (&(data_con->que_lock))))
    {
      syslog (LOG_ERR, "devser_data_invalidate data_con lock: %m");
      return -1;
    }
  syslog (LOG_DEBUG, "devser_data_invalidate");
  if (data_con->data_readed + data_con->available < data_con->data_size)
    {
      syslog (LOG_DEBUG, "devser_data_invalidate");
      data_con->available = -1;
      pthread_cond_signal (&(data_con->write_cond));
      pthread_cond_signal (&(data_con->read_cond));	// if there is some other thread waiting
    }

  pthread_mutex_unlock (&(data_con->que_lock));

  return 0;
}

/*!
 * Create a socket, bind it.
 *
 * Internal function.
 *
 * @param port 		port number.
 * @return positive 	socket on success, exit on failure.
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
  if (shmctl (data_shm, IPC_RMID, NULL))
    syslog (LOG_ERR, "IPC_RMID data_shm shmctl: %m");

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

/*!
 * Send message to another devser server.
 *
 * @see msgsnd.
 *
 * @param recv_id		id server to send
 * @param message		message to send, at most MSG_SIZE bytes length
 * 
 * @return 0 on success, -1 and set errno on error
 */
int
devser_2devser_message (int recv_id, void *message)
{
  struct devser_msg msg;

  msg.mtype = recv_id + IPC_MSG_SHIFT;
  memcpy (msg.mtext, message, MSG_SIZE);

  if (msgsnd (msg_id, &msg, MSG_SIZE, IPC_NOWAIT))
    {
      syslog (LOG_ERR, "error devser_2devser_message: %m %i %li", msg_id,
	      msg.mtype);
      return -1;
    }
  // else
  return 0;
}

/*!
 * Send message to given device. Use ap to format format string.
 *
 * @param recv_id	message receiver id
 * @param format	format
 * @param ap		va list
 *
 * @return 0 on success, -1 and set errno otherwise
 */
int
devser_2devser_message_va (int recv_id, char *format, va_list ap)
{
  char *mtext;
  int ret;

  vasprintf (&mtext, format, ap);
  ret = devser_2devser_message (recv_id, mtext);

  free (mtext);
  return ret;
}


/*!
 * Send message to given device. Use format string to format
 * message.
 *
 * @param recv_id	message receiver id
 * @param format	message format string
 *
 * @return 0 on success, -1 and set errno otherwise
 */
int
devser_2devser_message_format (int recv_id, char *format, ...)
{
  va_list ap;
  int ret;

  va_start (ap, format);
  ret = devser_2devser_message_va (recv_id, format, ap);
  va_end (ap);

  return ret;
}

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
      msg_size =
	msgrcv (msg_id, &msg_buf, MSG_SIZE, server_id + IPC_MSG_SHIFT, 0);
      if (msg_size < 0)
	{
	  syslog (LOG_ERR, "devser msg_receive: %m %i %i %i", errno, msg_size,
		  server_id);
	  // if (errno == EIDRM)
	  return NULL;
	}
      else if (msg_size != MSG_SIZE)
	syslog (LOG_ERR, "invalid message size: %i", msg_size);
      else
	{
	  // testing for null - be cautios, since 
	  // another thread could set null message
	  // handler
	  devser_handle_msg_t tmp_handler = msg_handler;

	  msg_buf.mtext[MSG_SIZE - 1] = 0;	// end message (otherwise buffer owerflow could result) 

	  if (tmp_handler)
	    {
	      syslog (LOG_INFO, "msg received: %s, calling handler",
		      msg_buf.mtext);
	      if (tmp_handler (msg_buf.mtext))
		syslog (LOG_ERR, "uncatched msg: %s", msg_buf.mtext);
	    }
	  else
	    {
	      syslog (LOG_WARNING,
		      "ipc message '%s' received, null msg handler",
		      msg_buf.mtext);
	    }
	}
    }
}

int
devser_param_test_length (int npars)
{
  int actuall_count = param_get_length (command_params);
  if (actuall_count != npars)
    {
      devser_write_command_end (DEVDEM_E_PARAMSNUM,
				"bad nmbr of params: expected %i,got %i",
				npars, actuall_count);
      return -1;
    }
  return 0;
}

int
devser_param_next_string (char **ret)
{
  if (param_next_string (command_params, ret))
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"expected string, got '%s'", ret);
      return -1;
    }
  return 0;
}

int
devser_param_next_integer (int *ret)
{
  if (param_next_integer (command_params, ret))
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"invalid parameter - expected integer, got '%s'",
				command_params->param_processing);
      return -1;
    }
  return 0;
}

int
devser_param_next_float (float *ret)
{
  if (param_next_float (command_params, ret))
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"invalid parameter - expected integer, got '%s'",
				command_params->param_processing);
      return -1;
    }
  return 0;
}

int
devser_param_next_hmsdec (double *ret)
{
  if (param_next_hmsdec (command_params, ret))
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL,
				"expected hms string, got: %s",
				command_params->param_processing);
      return -1;
    }
  return 0;
}

int
devser_param_next_ip_address (char **hostname, unsigned int *port)
{
  if (param_next_ip_address (command_params, hostname, port))
    {
      devser_write_command_end (DEVDEM_E_PARAMSVAL, "expected uri, got: %s",
				command_params->param_processing);
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

  fd_set read_set;

  FD_ZERO (&read_set);
  if (control_fd < 0)
    {
#ifdef DEBUG
      printf ("control_fd: %i\n", control_fd);
#endif /* DEBUG */
      return -1;
    }
  FD_SET (control_fd, &read_set);

  buffer = command_buffer.buf;
  endptr = command_buffer.endptr;

  ret = select (FD_SETSIZE, &read_set, NULL, NULL, NULL);

  if (!FD_ISSET (control_fd, &read_set))
    return -1;
  nbytes = read (control_fd, endptr, MAXMSG - (endptr - buffer));
  if (nbytes < 0)
    {
      /* Read error. */
      syslog (LOG_ERR, "read_from_client read: %m");
      return -1;
    }
  else if (nbytes == 0)
    {
      syslog (LOG_ERR, "read_from_client read 0 bytes");
      return -1;
    }
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
 * Return client ip
 */
void
devser_get_client_ip (struct sockaddr_in *client_ip)
{
  memcpy (client_ip, &server_client_ip, sizeof (server_client_ip));
}

/*! 
 * Signal handler.
 */
void
ser_sig_exit_main (int sig)
{
#ifdef DEBUG
  printf ("[%i] devser exiting with signal:%i\n", getpid (), sig);
#endif /* DEBUG */
  close (server_socket);
  while (childrens)
    {
#ifdef DEBUG
      printf ("kill %i\n", childrens->child_pid);
#endif /* DEBUG */
      kill (childrens->child_pid, SIGQUIT);
      childrens = childrens->next;
    }
#ifdef DEBUG
  printf ("devser removing IPC\n");
#endif /* DEBUG */
  if (semctl (data_sem, 1, IPC_RMID))
    {
      perror ("IPC_RMID data_sem semctl");
    }
  if (msgctl (msg_id, IPC_RMID, NULL))
    {
      perror ("IPC_RMID msg_id");
    }
}

void
ser_sig_exit_child (int sig)
{
#ifdef DEBUG
  printf ("[%i] exiting with signal: %i\n", getpid (), sig);
#endif /* DEBUG */
  close (control_fd);
}

void
ser_child_sig_exit (int sig)
{
  pid_t child_pid;
  child_pid = waitpid (-1, NULL, WNOHANG);
  if (child_pid < 0)
    syslog (LOG_ERR, "ser_child_sig_exit waitpid: %m");
  else
    {
      struct child *a_child, *prev = NULL;
      a_child = childrens;
      while (a_child && a_child->child_pid != child_pid)
	{
	  prev = a_child;
	  a_child = a_child->next;
	}
      if (prev)
	{
	  if (a_child)
	    {
	      prev->next = a_child->next;
	      free (a_child);
	    }
	}
      else if (childrens)
	{
	  prev = childrens;
	  childrens = childrens->next;
	  free (prev);
	}
    }
}

/*!
 * Set server id, and create thread to receive IPC messages.
 *
 * @param server_id_in		server_id
 * @param msg_handler_in	msg handler function
 *
 * @return 0 on sucess, -1 and set errno on failure
 */
int
devser_set_server_id (int server_id_in, devser_handle_msg_t msg_handler_in)
{
  if (server_id == -1)
    {
      if (server_id_in < 0)
	{
	  syslog (LOG_ERR, "devser_set_server_id invalid id: %i",
		  server_id_in);
	  devser_write_command_end (DEVDEM_E_SYSTEM, "invalid server_id %i",
				    server_id_in);
	  return -1;
	}
      server_id = server_id_in;
      msg_handler = msg_handler_in;
      if (pthread_create (&msg_thread, NULL, msg_receive_thread, NULL) < 0)
	{
	  syslog (LOG_ERR, "create message thread: %m");
	  devser_write_command_end (DEVDEM_E_SYSTEM,
				    "error creating message thread");
	  return -1;
	}
    }
  else
    {
      syslog (LOG_ERR, "server_id already set!");
      devser_write_command_end (DEVDEM_E_SYSTEM, "server_id already set");
      exit (EXIT_FAILURE);
    }
  syslog (LOG_DEBUG, "server_id set to %i", server_id);
  return 0;
}

/*!
 * Initalizes server, prepare it to run.
 *
 * Create all IPC stuff, init shared memory segments, set
 * semaphores...
 * 
 * @param shm_data_size	size for shared memory for data; =0 if shared data
 * 			memory is not required
 * 
 * @return 0 on success, -1 and set errno on error.
 */
int
devser_init (size_t shm_data_size)
{
  union semun sem_un;

  /* register on_exit */
  signal (SIGCHLD, ser_child_sig_exit);

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
      // hey, we will detach shmdata at exit, so don't care about them now
    }
  else
    {
      data_shm = -1;
    }

  // creates semaphore for guarding data_shm
  if ((data_sem = semget (IPC_PRIVATE, 1, 0644)) < 0)
    {
      syslog (LOG_ERR, "data shm semget: %m");
      return -1;
    }

  sem_un.val = 1;

  // initialize data shared memory semaphore
  if (semctl (data_sem, 0, SETVAL, sem_un))
    {
      syslog (LOG_ERR, "data sem # 0 init: %m");
      return -1;
    }

  // initialize ipc msg que
  if ((msg_id = msgget (IPC_PRIVATE, 0644)) < 0)
    {
      syslog (LOG_ERR, "devser msgget: %m");
      return -1;
    }

  return 0;
}

/*! 
 * Run server daemon.
 *
 * This function will run the device deamon on given port, and will
 * call handler for every command it received.
 * 
 * @param port 		port to run device deamon
 * @param in_handler 	reference to command handler code
 * 
 * @return 0 on success, -1 and set errno on error
 */
int
devser_run (int port, devser_handle_command_t in_handler)
{
  int i;
  size_t size;
  pid_t child = 0;

  signal (SIGTERM, ser_sig_exit_main);
  signal (SIGQUIT, ser_sig_exit_main);
  signal (SIGINT, ser_sig_exit_main);

  devser_parent_pid = getpid ();

  if (!in_handler)
    {
      errno = EINVAL;
      return -1;
    }
  cmd_handler = in_handler;

  /* create the socket and set it up to accept connections */
  server_socket = make_socket (port);
  if (listen (server_socket, 1) < 0)
    {
      syslog (LOG_ERR, "listen: %m");
      return -1;
    }

  syslog (LOG_INFO, "started");

  while (1)
    {
      size = sizeof (server_client_ip);
      if ((control_fd =
	   accept (server_socket, (struct sockaddr *) &server_client_ip,
		   &size)) < 0)
	{
	  syslog (LOG_ERR, "accept: %m");
	  return -1;
	}

      child = fork ();
      if (child < 0)
	{
	  syslog (LOG_ERR, "fork: %m");
	  return -1;
	}

      if (child == 0)
	{
	  // child
	  // (void) dup2 (control_fd, 0);
	  // (void) dup2 (control_fd, 1);
	  close (server_socket);
	  break;
	}

      // parent 
      close (control_fd);
      {
	struct child *an_child;
	if (childrens == NULL)
	  {
	    childrens = (struct child *) malloc (sizeof (struct child));
	    an_child = childrens;
	  }
	else
	  {
	    // find childrens tail
	    an_child = childrens;
	    while (an_child->next)
	      an_child = an_child->next;
	    an_child->next = (struct child *) malloc (sizeof (struct child));
	    an_child = an_child->next;
	  }
	an_child->next = NULL;
	an_child->child_pid = child;
      }
    }

  // here starts child processing - mind of differences in ipc id's,
  // pointers,..
  childrens = NULL;		// we don't need that

  signal (SIGQUIT, ser_sig_exit_child);

  server_id = -1;

  command_buffer.endptr = command_buffer.buf;
  syslog (LOG_INFO, "server: connect from host %s, port %hd, desc:%i",
	  inet_ntoa (server_client_ip.sin_addr),
	  ntohs (server_client_ip.sin_port), control_fd);

  devser_child_pid = getpid ();

  // initialize data_cons
  for (i = 0; i < MAXDATACONS; i++)
    {
      pthread_mutex_init (&data_cons[i].lock, NULL);
      pthread_mutex_init (&data_cons[i].que_lock, NULL);

      pthread_cond_init (&data_cons[i].write_cond, NULL);
      pthread_cond_init (&data_cons[i].read_cond, NULL);

      data_cons[i].sock = 0;
      data_cons[i].buffer = NULL;
    }

  // initialize threads lock
  for (i = 0; i < MAX_THREADS; i++)
    {
      pthread_mutex_init (&threads[i].lock, NULL);
    }


  // null threads count
  threads_count = 0;

  while (1)
    {
      if (read_from_client () < 0)
	{
	  close (control_fd);
	  syslog (LOG_INFO, "connection from desc %d closed", control_fd);
	  devser_thread_cancel_all ();
	  return -1;
	}
    }
}
