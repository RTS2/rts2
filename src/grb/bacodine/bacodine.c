#include <stdio.h>		/* Standard i/o header file */
#include <sys/types.h>		/* File typedefs header file */
#include <sys/socket.h>		/* Socket structure header file */
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <strings.h>

/*--------------------------------------------------------------------------*/
int
open_server (hostname, port)
     char *hostname;		/* Machine host name */
     int port;			/* Port number */
{
  int sock;			/* The connected sock descriptor */
  int sd = -1;			/* The offerred sock descriptor */
  int temp;			/* Dummy variable */
  int saddrlen;			/* Socket address length + 2 */
  struct sockaddr saddr;	/* Socket structure for UNIX */
  struct sockaddr *psaddr;	/* Pointer to sin */
  struct sockaddr_in s_in;	/* Socket structure for inet */
  const int so_reuseaddr = 1;


  temp = 0;
/* The socket is for inet connection, set up the s_in structure. */
  bzero ((char *) &s_in, sizeof (s_in));
  s_in.sin_family = AF_INET;
  s_in.sin_addr.s_addr = htonl (INADDR_ANY);
  s_in.sin_port = htons ((unsigned short) port);
  psaddr = (struct sockaddr *) &s_in;
  saddrlen = sizeof (s_in);

  printf ("server(): type=%i  (int)type=%i\n", AF_INET, AF_INET);	/* Debug */

/* Initiate the socket connection. */
  if ((sd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    return (serr (sd, "server(): socket."));
  if (setsockopt
      (sd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof (int)) < 0)
    perror ("setsockopt");
/* Bind the name to the socket. */
  if (bind (sd, psaddr, saddrlen) == -1)
    {
      printf ("bind() errno=%i\n", errno);
      perror ("bind()");
      if (errno == 98)
	sleep (200);
      return (serr (sd, "server(): bind."));
    }

  printf ("Listening..\n");

/* Listen for the socket connection from the GCN machine. */
  if (listen (sd, 5))
    return (serr (sd, "server(): listen."));

  printf ("Accepting..\n");

/* Accept the socket connection from the GCN machine (the client). */
  if ((sock = accept (sd, &saddr, &temp)) < 0)
    return (serr (sd, "server(): accept."));

  close (sd);

/* Make the connection nonblocking I/O. */
//  if (ioctl (sock, FIONBIO, &on, sizeof (on)) < 0)
//    return (serr (sock, "server(): ioctl."));

  serr (0, "server(): the server is up.");
  return (sock);
}
