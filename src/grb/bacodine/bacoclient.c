#include "../grbc.h"
#include <stdlib.h>
#include <string.h>
#include "../../utils/config.h"

#include <libnova/libnova.h>

process_grb_event_t process_grb;

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
// Login to INTA proxy
int
login (sock)
     int sock;
{
  fd_set read_fd;
  char *login_user = "";
  char *login_password = "";
  char *login_proxy_user = "User:";
  char *login_proxy_pass = "password:";
  char *login_accepted = "Login accepted\r\n";
  char *logi;
  struct timeval to;
  unsigned char c;
  int go_for = 0;

  if (*(get_device_string_default ("bacoclient", "proxy", "Y")) != 'Y')
    return 0;

  printf ("Will login throuht proxy\n");

  login_accepted =
    get_device_string_default ("bacoclient", "proxy_end", login_accepted);

  login_proxy_user =
    get_device_string_default ("bacoclient", "proxy_user", login_proxy_user);

  login_proxy_pass =
    get_device_string_default ("bacoclient", "proxy_pass", login_proxy_pass);

wait_loop:
  to.tv_sec = 50;
  to.tv_usec = 0;

  FD_ZERO (&read_fd);
  FD_SET (sock, &read_fd);

  printf ("Waiting..\n");
  while ((*logi) && select (FD_SETSIZE, &read_fd, NULL, NULL, &to) == 1
	 && read (sock, &c, sizeof (c)) == sizeof (c))
    {
      write (1, &c, sizeof (c));
      if (!go_for)
	{
	  if (c == *login_accepted)
	    {
	      go_for = 1;
	      logi = login_accepted;
	    }
	  else if (c == *login_proxy_user)
	    {
	      go_for = 2;
	      logi = login_proxy_user;
	    }
	}
      if (c == *logi)
	{
	  write (1, "*", 1);
	  logi++;
	}
      else
	{
	  go_for = 0;
	}
      FD_ZERO (&read_fd);
      FD_SET (sock, &read_fd);
    }

  if (go_for == 1 && !*logi)
    return 0;

  if (*logi)
    {
      close (sock);
      return -1;
    }

  login_user = get_device_string_default ("bacoclient", "user", login_user);
  login_password =
    get_device_string_default ("bacoclient", "password", login_password);
  write (sock, login_user, strlen (login_user));
  write (sock, "\r\n", 2);
  logi = login_proxy_pass;

  printf ("Waiting..\n");
  while ((*logi) && select (FD_SETSIZE, &read_fd, NULL, NULL, &to) == 1
	 && read (sock, &c, sizeof (c)) == sizeof (c))
    {
      write (1, &c, sizeof (c));
      if (c == *logi)
	{
	  write (1, "*", 1);
	  logi++;
	}
      else
	{
	  logi = login_proxy_pass;
	}
      FD_ZERO (&read_fd);
      FD_SET (sock, &read_fd);
    }

  if (*logi)
    {
      close (sock);
      return -1;
    }

  write (sock, login_password, strlen (login_password));
  write (sock, "\r\n", 2);

  logi = login_accepted;

  goto wait_loop;
}

/*--------------------------------------------------------------------------*/
int
open_conn (hostname, port)
     char *hostname;		/* Machine host name */
     int port;			/* Port number */
{
  int sock = -1;		/* The connected sock descriptor */
  struct sockaddr_in s_in;	/* Socket structure for inet */
  struct hostent *host_info;

  s_in.sin_family = AF_INET;
  s_in.sin_port = htons (port);
  if (!(host_info = gethostbyname (hostname)))
    {
      serr (sock, "client(): gethostbyname.");
      printf ("hostname : %s\n", hostname);
      return -1;
    }
  s_in.sin_addr = *(struct in_addr *) host_info->h_addr;

/* Initiate the socket connection. */
  if ((sock = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    return (serr (sock, "server(): socket."));

  while (connect (sock, (struct sockaddr *) &s_in, sizeof (s_in)) == -1 ||
	 login (sock) == -1)
    {
      perror ("connect");
      sleep (600);
    }

  printf ("Connection goes on.\n");
  fflush (stdout);

/* Make the connection nonblocking I/O. */
  fcntl (sock, F_SETFL, O_NONBLOCK);

  serr (0, "client(): the client is up.");
  return (sock);
}
