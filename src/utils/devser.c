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

#include "devdem.h"
#include "../status.h"

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

#include "devconn.h"

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

//! Structure holding information about socket buffer processing.
struct
{
  char buf[MAXMSG + 1];
  char *endptr;
}
client_info;

//! Handler functions
devdem_handle_command_t cmd_handler;
devdem_handle_command_t msg_handler;

pid_t devdem_parent_pid;
pid_t devdem_child_pid;

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

//! Filedes of ascii control channel
int control_fd;

#define MAX_THREADS	30

//! Thread management type
struct
{
  pthread_mutex_t lock;		//! lock to lock threads
  pthread_t thread;		//! thread
}
threads[MAX_THREADS];

//! Threads management helper structure
struct thread_wrapper_temp
{
  pthread_mutex_t *lock;
  void *arg;
  void *(*start_routine) (void *);
};

//! Number of status registers
int status_num;

//! Key to shared memory segment, holding status flags.
int status_shm;

//! Key to semaphore for access control to shared memory segment
int status_sem;

//! Data shared memory
int data_shm;

//! Data semaphore to control write access
int data_sem;

//! IPC message que identifier
int msg_id;

//! IPC message receive thread
pthread_t msg_thread;

//! Holds status informations
typedef struct
{
  char name[STATUSNAME + 1];
  int status;
}
status_t;

//! Array holding status structures; stored in shared mem, hence just pointer
status_t *statutes;

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
  sb.sem_flg = 0;
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
devdem_dprintf (const char *format, ...)
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
      syslog (LOG_ERR, "devdem_write: ret:%i count:%i desc:%i error:%m", ret,
	      count_with_end, control_fd);
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
  return devdem_dprintf ("M %s %i %s", st->name, st->status, description);
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
devdem_status_message (int subdevice, char *description)
{
  if (subdevice >= status_num || subdevice < 0)
    {
      errno = EINVAL;
      return -1;
    }
  if (status_lock (subdevice) < 0)
    return -1;
  if (status_message (subdevice, description) < 0)
    return -1;
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
devdem_write_command_end (int retc, char *msg_format, ...)
{
  va_list ap;
  int ret, tmper;
  char *msg;

  va_start (ap, msg_format);
  vasprintf (&msg, msg_format, ap);
  va_end (ap);

  ret = devdem_dprintf ("%+04i %s", retc, msg);
  tmper = errno;
  free (msg);
  errno = tmper;
  return ret;
}

/*! 
 * Internall call.
 */
void *
thread_wrapper (void *temp)
{
  void *ret;
  struct thread_wrapper_temp *arg = (struct thread_wrapper_temp *) temp;
  pthread_cleanup_push (pthread_mutex_unlock, (void *) (arg->lock));
  ret =
    ((struct thread_wrapper_temp *) temp)->
    start_routine (((struct thread_wrapper_temp *) temp)->arg);
  pthread_cleanup_pop (1);
  free (((struct thread_wrapper_temp *) temp)->arg);
  free (temp);
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
 * @param arg_size 	size of argument; in bytes; when arg is NULL, arg_size is
 * 			ignored
 * @param id 		if not NULL, returns id of thread, which can by used for 
 * 			@see pthread_cancel_thread (int)
 *
 * @return -1 and set errno on error, 0 otherwise. 
 */
int
devdem_thread_create (void *(*start_routine) (void *), void *arg,
		      size_t arg_size, int *id)
{
  int i;
  for (i = 0; i < MAX_THREADS; i++)
    {
      if (pthread_mutex_trylock (&threads[i].lock) == 0)
	{
	  struct thread_wrapper_temp *temp =
	    (struct thread_wrapper_temp *)
	    malloc (sizeof (struct thread_wrapper_temp));
	  temp->lock = &threads[i].lock;
	  temp->start_routine = start_routine;
	  if (arg)
	    {
	      temp->arg = malloc (arg_size);
	      memcpy (temp->arg, arg, arg_size);
	    }
	  else
	    {
	      temp->arg = NULL;
	    }
	  if (id)
	    *id = i;
	  return pthread_create (&threads[i].thread, NULL, thread_wrapper,
				 (void *) temp);
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
 * @param id Thread id, as returned by @see devdem_thread_create.
 * @retun -1 and set errno on error, 0 otherwise.
 */
int
devdem_thread_cancel (int id)
{
  errno = pthread_cancel (threads[id].thread);
  return errno ? -1 : 0;
}

/*! 
 * Stop all threads.
 *
 * @see devdem_thread_cancel
 *
 * @return always 0.
 */
int
devdem_thread_cancel_all (void)
{
  int i;
  for (i = 0; i < MAX_THREADS; i++)
    {
      pthread_cancel (threads[i].thread);
    }
  return 0;
}

/*! 
 * Handle devdem commands.
 *
 * It's solely responsibility of handler to report any errror,
 * which occures during command processing.
 * <p>
 * Errors resulting from wrong command format are reported directly.
 *
 * @param buffer Buffer containing params
 * @param handler Command handler. 
 * @return -1 and set errno on network failure|exit command, otherwise 0. 
 */
int
devdem_handle_commands (char *buffer, devdem_handle_command_t handler)
{
  char *next_command = NULL;
  char *argv, *cargv;
  size_t argc, cargc;
  int ret;
  if ((ret = argz_create_sep (buffer, ';', &argv, &argc)))
    {
      errno = ret;
      return devdem_write_command_end (DEVDEM_E_SYSTEM, "System: ",
				       strerror (errno));
    }
  if (!argv)
    return devdem_write_command_end (DEVDEM_E_COMMAND, "Empty command!");

  while ((next_command = argz_next (argv, argc, next_command)))
    {
      if ((ret = argz_create_sep (next_command, ' ', &cargv, &cargc)))
	{
	  devdem_write_command_end (DEVDEM_E_SYSTEM, "Argz call error: %s",
				    strerror (ret));
	  goto end;
	}
      if (!cargv)
	{
	  devdem_write_command_end (DEVDEM_E_COMMAND, "Empty command!");
	  free (cargv);
	  goto end;
	}
      syslog (LOG_DEBUG, "Handling '%s'", next_command);
      ret = cmd_handler (cargv, cargc);
      free (cargv);
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
    devdem_write_command_end (0, "Success");
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
devdem_send_data (struct in_addr *client_addr, void *data_ptr,
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
      return devdem_write_command_end (DEVDEM_E_SYSTEM,
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

  if (devdem_dprintf
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
 * @param subdevice subdevice to change.
 * @param mask <code>(*statutes)[subdevice].status &= !mask;</code>
 * @param operand  <code>(*statutes)[subdevice].status |= operand;</code>
 * @param message optional message to transmit
 *
 * @return -1 and set errno on error, 0 otherwise
 */
int
devdem_status_mask (int subdevice, int mask, int operand, char *message)
{
  if (status_lock (subdevice) < 0)
    return -1;
  // critical section begins

  (statutes)[subdevice].status &= !mask;
  (statutes)[subdevice].status |= operand;

  if (status_message (subdevice, message) < 0)
    return -1;

  // critical section ends
  return status_unlock (subdevice);
}

/*!
 * Attach shared memory data segment.
 *
 * It's size is specified in call to @see devdem_run. Exit on any error.
 *
 * @return	pointer to shared data
 */
void *
devdem_shm_data_at ()
{
  void *ret = shmat (data_shm, NULL, 0);
  if ((int) ret < 0)
    {
      syslog (LOG_ERR, "devdem_shm_data_at shmat: %m");
      exit (EXIT_FAILURE);
    }
  return ret;
}

/*!
 * Lock semaphore for shared data access.
 */
void
devdem_shm_data_lock ()
{
  struct sembuf sb;

  sb.sem_num = 0;
  sb.sem_op = -1;
  sb.sem_flg = 0;
  if (semop (data_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "devdem_shm_data_lock semphore -1: %m");
      exit (EXIT_FAILURE);
    }
}

/*! 
 * Unlock semaphore for shared data access.
 */
void
devdem_shm_data_unlock ()
{
  struct sembuf sb;

  sb.sem_num = 0;
  sb.sem_op = +1;
  sb.sem_flg = 0;
  if (semop (data_sem, &sb, 1) < 0)
    {
      syslog (LOG_ERR, "devdem_shm_data_unlock +1: %m");
      exit (EXIT_FAILURE);
    }
}

/*! 
 * Detach shared data memory.
 *
 * Exit on any error.
 */
void
devdem_shm_data_dt (void *mem)
{
  if (shmdt (mem) < 0)
    {
      syslog (LOG_ERR, "devdem_shm_data_dt: %m");
      exit (EXIT_FAILURE);
    }
}

void
devdem_msg_set_handler (devdem_handle_command_t handler)
{
  msg_handler = handler;
}

int
devdem_msg_snd (struct devdem_msg *msg)
{
  return msgsnd (msg_id, msg, MSG_SIZE, 0);
}

void *
msg_receive_thread (void *arg)
{
  while (1)
    {
      struct devdem_msg msg_buf;
      int msg_size;
      msg_size = msgrcv (msg_id, &msg_buf, MSG_SIZE, devdem_child_pid, 0);
      if (msg_size < 0)
	{
	  syslog (LOG_ERR, "devdem msg_receive: %m");
	}
      else
	{
	  // testing for null - be cautios, since 
	  // another thread could set null message
	  // handler
	  devdem_handle_command_t tmp_handler = msg_handler;
	  if (tmp_handler)
	    {
	      syslog (LOG_INFO, "msg received: %50s", msg_buf.mtext);
	      tmp_handler (msg_buf.mtext, msg_size);
	    }
	  else
	    {
	      syslog (LOG_WARNING, "ipc message received, null msg handler");
	    }
	}
    }
}

int
read_from_client ()
{
  char *buffer;
  int nbytes;
  char *startptr;		// start of message
  char *endptr;			// end of message
  int ret;

  buffer = client_info.buf;
  endptr = client_info.endptr;
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
	      if ((ret = devdem_handle_commands (startptr, cmd_handler)) < 0)
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
	      client_info.endptr = endptr;
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
devdem_on_exit (int err, void *args)
{
  syslog (LOG_INFO, "exiting");
}

/*! 
 * Signal handler.
 */
void
sig_exit (int sig)
{
  if (getpid () == devdem_parent_pid)
    {
      union semun sem_un;
      syslog (LOG_INFO, "exiting with signal:%d", sig);
      if (shmctl (status_shm, IPC_RMID, NULL) < 0)
	{
	  syslog (LOG_ERR, "IPC_RMID status_shm shmctl: %m");
	}
      if (semctl (status_sem, status_num, IPC_RMID, sem_un) < 0)
	{
	  syslog (LOG_ERR, "IPC_RMID status_sem semctl: %m");
	}
      if (shmctl (data_shm, IPC_RMID, NULL) < 0)
	{
	  syslog (LOG_ERR, "IPC_RMID data_shm shmctl: %m");
	}
      if (semctl (data_sem, status_num, IPC_RMID, sem_un) < 0)
	{
	  syslog (LOG_ERR, "IPC_RMID data_sem semctl: %m");
	}
      if (msgctl (msg_id, IPC_RMID, NULL) < 0)
	{
	  syslog (LOG_ERR, "IPC_RMID msg_id: %m");
	}
    }
  exit (0);
}

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
 * @return 0 on succes, -1 and set errno on error
 */
int
devdem_run (int port, devdem_handle_command_t in_handler,
	    char **status_names, int status_num_in, size_t shm_data_size)
{
  int sock, i;
  struct sockaddr_in clientname;
  size_t size;
  status_t *st;
  union semun sem_un;
  devdem_parent_pid = getpid ();

  if (!in_handler)
    {
      errno = EINVAL;
      return -1;
    }
  cmd_handler = in_handler;
  msg_handler = NULL;
  status_num = status_num_in;
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

  // initialize status semaphores
  st = statutes;
  sem_un.val = 1;
  for (i = 0; i < status_num; i++, st++)
    {
      strncpy (st->name, status_names[i], STATUSNAME + 1);
      st->status = 0;
      semctl (status_sem, i, SETVAL, sem_un);
    }

  // detach status shared memory
  if (shmdt (statutes) < 0)
    {
      syslog (LOG_ERR, "shmdt: %m");
      return -1;
    }

  // creates semaphore for control to data shm
  if ((data_sem = semget (IPC_PRIVATE, 1, 0644)) < 0)
    {
      syslog (LOG_ERR, "data shm semget: %m");
      return -1;
    }

  // initialize data shared memory
  if (shm_data_size > 0)
    {
      void *shmdata;
      void *dataptr;
      int i;
      if ((data_shm = shmget (IPC_PRIVATE, shm_data_size, 0644)) < 0)
	{
	  syslog (LOG_ERR, "shm_data shmget: %m");
	  return -1;
	}
      // attach shared segment
      shmdata = devdem_shm_data_at ();
      // null shared segment
      dataptr = shmdata;
      for (i = 0; i < shm_data_size; i++)
	{
	  *((char *) dataptr) = 0;
	  dataptr++;
	}
      // detach shared segment
      devdem_shm_data_dt (shmdata);
    }
  else
    {
      data_shm = -1;
    }

  // initialize data shared memory semaphore
  semctl (data_sem, 0, SETVAL, sem_un);

  // initialize ipc msg que
  if ((msg_id = msgget (IPC_PRIVATE, 0644)) < 0)
    {
      syslog (LOG_ERR, "devdem msgget: %m");
      return -1;
    }

  /* register on_exit */
  on_exit (devdem_on_exit, NULL);
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

  client_info.endptr = client_info.buf;
  syslog (LOG_INFO, "server: connect from host %s, port %hd, desc:%i",
	  inet_ntoa (clientname.sin_addr),
	  ntohs (clientname.sin_port), control_fd);

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

  devdem_child_pid = getpid ();
  // create message thread
  if (pthread_create (&msg_thread, NULL, msg_receive_thread, NULL) < 0)
    {
      syslog (LOG_ERR, "create message thread: %m");
      return -1;
    }

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
