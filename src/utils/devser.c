/*! @file Device daemon skeleton code.
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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
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

#include <argz.h>

#define MAXMSG  512

typedef struct
{
  char buf[MAXMSG + 1];
  char *endptr;
}
servinfo;

servinfo *clients[FD_SETSIZE];

/*! Printf to descriptor, log to syslogd
* 
* @param fd file descriptor
* @param format buffer format
* @param ... buffer length
* @return -1 and set errno on error, 0 otherwise
*/
int
devdem_dprintf (int fd, const char *format, ...)
{
  int ret;
  va_list ap;
  char *msg;
  int count;

  va_start (ap, format);
  vasprintf (&msg, format, ap);
  va_end (ap);
  count = strlen (msg);
  syslog (LOG_DEBUG, "Writing '%s' to desc %i", msg, fd);
  if ((ret = write (fd, msg, count)) != count)
    {
      syslog (LOG_ERR, "devdem_write: ret:%i count:%i", ret, count);
      free (msg);
      return -1;
    }
  free (msg);
  return 0;
}

/*! Write ending message to fd.
* 
* @param fd file descriptor to write in
* @param msg message to write
* @param retc return code
* @return -1 and set errno on error, 0 otherwise
*/
int
devdem_write_command_end (int fd, char *msg, int retc)
{
  return devdem_dprintf (fd, "%+04i %s\n", retc, msg);
}


/*! Handle devdem commands
*
* @param buffer Buffer containing params
* @param fd Socket file descriptor
* @return -1 and set errno on network failure, otherwise 0. Any HW
* failure is reported to socket. 
*/
int
devdem_handle_commands (char *buffer, int fd, devdem_handle_command_t handler)
{
  char *next_command = NULL;
  char *argv;
  size_t argc;
  int ret;
  if ((ret = argz_create_sep (buffer, ';', &argv, &argc)))
    {
      errno = ret;
      return devdem_write_command_end (fd, strerror (errno), -errno);
    }
  if (!argv)
    return devdem_write_command_end (fd, "Empty command", -1);

  while ((next_command = argz_next (argv, argc, next_command)))
    {
      if (handler (next_command, fd) < 0)
	goto end;
    }

end:
  return devdem_write_command_end (fd, strerror (errno), -errno);
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
      syslog (LOG_ERR, "socket: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (int))
      < 0)
    {
      syslog (LOG_ERR, "setsockopt: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      syslog (LOG_ERR, "bind: %s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  return sock;
}


int
read_from_client (int fd, devdem_handle_command_t handler, servinfo * client)
{
  char *buffer;
  int nbytes;
  char *startptr;		// start of message
  char *endptr;			// end of message
  int ret;

  buffer = client->buf;
  endptr = client->endptr;
  nbytes = read (fd, endptr, MAXMSG - (endptr - buffer));
  if (nbytes < 0)
    {
      /* Read error. */
      syslog (LOG_ERR, "read: %s", strerror (errno));
      return -1;
    }
  else if (nbytes == 0)
    /* End-of-file. */
    return -1;
  else
    {
      buffer[nbytes] = 0;	// mark end of message
      startptr = endptr = buffer;
      while (1)
	{
	  if (*endptr == '\r')
	    {
	      *endptr = 0;
	      syslog (LOG_DEBUG, "Server: got message: '%s'", buffer);
	      if ((ret = devdem_handle_commands (startptr, fd, handler)) < 0)
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
	      nbytes = read (fd, endptr, MAXMSG - (endptr - startptr));
	      endptr[nbytes] = 0;	// mark end appropriatery
	      client->endptr = endptr;
	      if (nbytes < 0)
		{
		  if (errno == EAGAIN)
		    return 0;
		  else
		    {
		      /* Read error. */
		      syslog (LOG_ERR, "read: %s", strerror (errno));
		      return -1;
		    }
		}
	      else if (nbytes == 0)
		/* End-of-file. */
		return 0;
	    }
	  else
	    endptr++;
	}
    }
}

void
devdem_on_exit (int err, void *args)
{
  syslog (LOG_INFO, "Exiting");
}

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
devdem_run (int port, devdem_handle_command_t handler)
{
  int sock;
  fd_set active_fd_set, read_fd_set;
  int i;
  struct sockaddr_in clientname;
  size_t size;
  /* Register on_exit */
  on_exit (devdem_on_exit, NULL);
  signal (SIGTERM, sig_exit);
  signal (SIGQUIT, sig_exit);
  signal (SIGINT, sig_exit);
  /* Create the socket and set it up to accept connections. */
  sock = make_socket (port);
  if (listen (sock, 1) < 0)
    {
      syslog (LOG_ERR, "listen: %s", strerror (errno));
      return -1;
    }

  /* Initialize the set of active sockets. */
  FD_ZERO (&active_fd_set);
  FD_SET (sock, &active_fd_set);

  syslog (LOG_INFO, "Started");

  while (1)
    {
      /* Block until input arrives on one or more active sockets. */
      read_fd_set = active_fd_set;
      if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
	{
	  syslog (LOG_ERR, "select: %s", strerror (errno));
	  if (errno != EBADF)
	    return -1;
	}

      /* Service all the sockets with status or input pending. */
      for (i = 0; i < FD_SETSIZE; ++i)
	if (FD_ISSET (i, &read_fd_set))
	  {
	    if (i == sock)
	      {
		/* Connection request on original socket. */
		int new;
		size = sizeof (clientname);
		new = accept (sock, (struct sockaddr *) &clientname, &size);
		if (new < 0)
		  {
		    syslog (LOG_ERR, "accept: %s", strerror (errno));
		    return -1;
		  }
		if (fcntl (new, F_SETFL, O_NONBLOCK) < 0)
		  {
		    syslog (LOG_ERR, "fcntl: %s", strerror (errno));
		    break;
		  }
		clients[new] = (servinfo *) malloc (sizeof (servinfo));
		clients[new]->endptr = clients[new]->buf;
		syslog (LOG_INFO,
			"Server: connect from host %s, port %hd, desc:%i",
			inet_ntoa (clientname.sin_addr),
			ntohs (clientname.sin_port), new);
		FD_SET (new, &active_fd_set);
	      }
	    else
	      {
		/* Data arriving on an already-connected socket. */
		if (read_from_client (i, handler, clients[i]) < 0)
		  {
		    free (clients[i]);
		    close (i);
		    syslog (LOG_INFO, "Connection from desc %d closed", i);
		    FD_CLR (i, &active_fd_set);
		  }
	      }
	  }
    }
}
