#include "grbd.h"
#include "rts2grbfw.h"
#include "grbconst.h"

#include <errno.h>

Rts2GrbForwardConnection::Rts2GrbForwardConnection (Rts2Block * in_master,
						    int in_forwardPort):
Rts2Conn (in_master)
{
  forwardPort = in_forwardPort;
}

int
Rts2GrbForwardConnection::init ()
{
  int ret;
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    {
      syslog (LOG_ERR,
	      "Rts2GrbForwardConnection cannot create listen socket");
      return -1;
    }
  // do some listen stuff..
  const int so_reuseaddr = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
	      sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (forwardPort);
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
  if (ret == -1)
    {
      syslog (LOG_ERR, "Rts2GrbForwardConnection::init bind %m");
      return -1;
    }
  ret = listen (sock, 5);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2GrbForwardConnection::init cannot listen: %m");
      close (sock);
      sock = -1;
      return -1;
    }
  return 0;
}

int
Rts2GrbForwardConnection::receive (fd_set * set)
{
  int new_sock;
  struct sockaddr_in other_side;
  socklen_t addr_size = sizeof (struct sockaddr_in);
  if (FD_ISSET (sock, set))
    {
      new_sock = accept (sock, (struct sockaddr *) &other_side, &addr_size);
      if (new_sock == -1)
	{
	  syslog (LOG_ERR, "Rts2GrbForwardConnection::receive accept %m");
	  return 0;
	}
      syslog (LOG_DEBUG,
	      "Rts2GrbForwardClientConn::accept connection from %s %i",
	      inet_ntoa (other_side.sin_addr), ntohs (other_side.sin_port));
      Rts2GrbForwardClientConn *newConn =
	new Rts2GrbForwardClientConn (new_sock, getMaster ());
      getMaster ()->addConnection (newConn);
    }
  return 0;
}

/**
 * Takes cares of client connections.
 */

Rts2GrbForwardClientConn::Rts2GrbForwardClientConn (int in_sock, Rts2Block * in_master):Rts2ConnNoSend (in_sock,
		in_master)
{
}

void
Rts2GrbForwardClientConn::forwardPacket (long *nbuf)
{
  int ret;
  ret = write (sock, nbuf, SIZ_PKT);
  if (ret != SIZ_PKT)
    {
      syslog (LOG_ERR,
	      "Rts2GrbForwardClientConn::forwardPacket cannot forward %m (%i, %i)",
	      errno, ret);
      connectionError ();
    }
}

void
Rts2GrbForwardClientConn::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case RTS2_EVENT_GRB_PACKET:
      forwardPacket ((long *) event->getArg ());
      break;
    }
  Rts2ConnNoSend::postEvent (event);
}

int
Rts2GrbForwardClientConn::receive (fd_set * set)
{
  static long loc_buf[SIZ_PKT];
  if (sock < 0)
    return -1;

  if (FD_ISSET (sock, set))
    {
      int ret;
      ret = read (sock, loc_buf, SIZ_PKT);
      if (ret != SIZ_PKT)
	{
	  syslog (LOG_ERR, "Rts2GrbForwardClientConn::receive %m (%i, %i)",
		  errno, ret);
	  return -1;
	}
      // get some data back..
      successfullRead ();
      return ret;
    }
  return 0;
}
