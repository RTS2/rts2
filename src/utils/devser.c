/*! 
 * @file Device daemon skeleton code.
 * $Id$
 * Implements simple TCP/IP server, calling hander for every command it
 * receives.
 *
 * Command string is defined as follow:
 * <ul>
 * 	<li>commands ::= com | com + ';' + commands
 * 	<li>com ::= name + ' '\+ + params
 * 	<li>params ::= '' | par + ' '\+ + params
 * 	<li>par ::= hms | decimal | integer
 * </ul>
 * In dev demon is implemented only split for ';', all other splits
 * must be implemented in a device driver handler routine.
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

#include <argz.h>

#include "devconn.h"

typedef struct
{
  char buf[MAXMSG + 1];
  char *endptr;
}
servinfo_t;

//! Information structure, for each open socket one
servinfo_t client_info;
//! Handler function
devdem_handle_command_t handler;

pid_t devdem_parent_pid;

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

//! Filedes of asci control channel
int control_fd;

/*! Printf to descriptor, log to syslogd
 * 
 * @param fd file descriptor
 * @param format buffer format
 * @param ... thinks to print 
 * @return -1 and set errno on error, 0 otherwise
 */
int
devdem_dprintf (const char *format, ...)
{
  int ret;
  va_list ap;
  char *msg;
  int count;

  va_start (ap, format);
  vasprintf (&msg, format, ap);
  va_end (ap);

  count = strlen (msg);
  syslog (LOG_DEBUG, "Writing '%s' to desc %i", msg, control_fd);
  if ((ret = write (control_fd, msg, count)) != count)
    {
      syslog (LOG_ERR, "devdem_write: ret:%i count:%i desc:%i", ret, count,
	      control_fd);
      free (msg);
      return -1;
    }
  free (msg);
  return 0;
}

/*! Write ending message to fd.
 * 
 * @param fd file descriptor to write in
 * @param retc return code
 * @param msg_format message to write
 * @param 
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

  ret = devdem_dprintf ("%+04i %s\r\n", retc, msg);
  tmper = errno;
  free (msg);
  errno = tmper;
  return ret;
}


/*! Handle devdem commands
 *
 * It's solely responsibility of handler to report any errror.
 *
 * @param buffer Buffer containing params
 * @param fd Socket file descriptor
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
      ret = handler (cargv, cargc);
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

/*! Get next free connection
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
      syslog (LOG_DEBUG, "On port %i[%i] write %i bytes", port,
	      data_con->sock, ret);
      ready_data_ptr += ret;
    }
  if (close (data_con->sock) < 0)
    syslog (LOG_ERR, "close %i: %m", data_con->sock);

  pthread_mutex_unlock (&data_con->lock);

  if (ret < 0)
    syslog (LOG_ERR, "Sending data on port: %i %m", port);
  return NULL;
}

/*! Connect, initializes and send data to given client
 * Quite complex, hopefully quite clever.
 * Handles all task connected with sending data - just past it your
 * address, data to send, and don't care about rest.
 *
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
      syslog (LOG_INFO, "No new connection");
      return devdem_write_command_end (DEVDEM_E_SYSTEM,
				       "No new data connection");
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
      ("connect %i %s:%i %i\r\n", conn,
       inet_ntoa (control_socaddr.sin_addr), port, data_size) < 0)
    goto err;

  FD_ZERO (&a_set);
  FD_SET (data_listen_sock, &a_set);
  accept_tout.tv_sec = 500;
  accept_tout.tv_usec = 0;

  syslog (LOG_DEBUG, "Waiting for connection on port %i", port);
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
  syslog (LOG_INFO, "Connection on port %i from %s:%i accepted desc %i",
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
	      syslog (LOG_DEBUG, "Server: got message: '%s'", buffer);
	      if ((ret = devdem_handle_commands (startptr, handler)) < 0)
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

/*! On exit handler
 */
void
devdem_on_exit (int err, void *args)
{
  syslog (LOG_INFO, "Exiting");
}

/*! Signal handler
 */
void
sig_exit (int sig)
{
  syslog (LOG_INFO, "Exiting with signal:%d", sig);
  exit (0);
}

/*! Run device daemon
 * This function will run the device deamon on given port, and will
 * call handler for every command it received.
 * 
 * @param port Port to run device deamon
 * @param handler Address of handler code
 * @return 0 on succes, -1 and set errno on error
 */
int
devdem_run (int port, devdem_handle_command_t in_handler)
{
  int sock;
  struct sockaddr_in clientname;
  size_t size;
  devdem_parent_pid = getpid ();

  if (!in_handler)
    {
      errno = EINVAL;
      return -1;
    }
  handler = in_handler;
  /* Register on_exit */
  on_exit (devdem_on_exit, NULL);
  signal (SIGTERM, sig_exit);
  signal (SIGQUIT, sig_exit);
  signal (SIGINT, sig_exit);
  /* Create the socket and set it up to accept connections. */
  sock = make_socket (port);
  if (listen (sock, 1) < 0)
    {
      syslog (LOG_ERR, "listen: %m");
      return -1;
    }

  syslog (LOG_INFO, "Started");

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
  client_info.endptr = client_info.buf;
  syslog (LOG_INFO, "Server: connect from host %s, port %hd, desc:%i",
	  inet_ntoa (clientname.sin_addr),
	  ntohs (clientname.sin_port), control_fd);
  // initialize data_cons
  for (sock = 0; sock < MAXDATACONS; sock++)
    {
      pthread_mutex_init (&data_cons[sock].lock, NULL);
    }
  while (1)
    {
      if (read_from_client () < 0)
	{
	  close (control_fd);
	  syslog (LOG_INFO, "Connection from desc %d closed", control_fd);
	  exit (EXIT_FAILURE);
	}
    }
  exit (0);
}
