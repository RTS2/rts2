#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include "rts2dataconn.h"

Rts2ClientTCPDataConn::Rts2ClientTCPDataConn (Rts2Block * in_master,
					      Rts2Conn * in_owner_conn,
					      char *hostname, int in_port,
					      int in_totalSize):
Rts2ConnNoSend (in_master)
{
  struct addrinfo hints;
  struct addrinfo *in_addr;
  char *s_port;
  int ret;

  // delet us if construction of socket fails
  setConnState (CONN_DELETE);
  ownerConnection = in_owner_conn;
  setConnTimeout (140);

  data = NULL;
  dataTop = NULL;
  receivedSize = 0;
  totalSize = in_totalSize;
  // try to resolve hostname..
  hints.ai_flags = 0;
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  asprintf (&s_port, "%i", in_port);
  ret = getaddrinfo (hostname, s_port, &hints, &in_addr);
  free (s_port);
  if (ret)
    {
      syslog (LOG_ERR,
	      "Rts2ClientTCPDataConn::Rts2ClientTCPDataConn getaddrinfo: %m");
      freeaddrinfo (in_addr);
      sock = -1;
      return;
    }
  sock =
    socket (in_addr->ai_family, in_addr->ai_socktype, in_addr->ai_protocol);
  if (sock == -1)
    {
      syslog (LOG_ERR,
	      "Rts2ClientTCPDataConn::Rts2ClientTCPDataConn socket: %m");
      freeaddrinfo (in_addr);
      return;
    }
  ret = fcntl (sock, F_SETFL, O_NONBLOCK);
  if (ret)
    {
      syslog (LOG_ERR,
	      "Rts2ClientTCPDataConn::Rts2ClientTCPDataConn cannot set socket non-blocking: %m");
    }
  // try to connect
  ret = connect (sock, in_addr->ai_addr, in_addr->ai_addrlen);
  freeaddrinfo (in_addr);
  data = new char[totalSize];
  dataTop = data;
  if (ret == -1)
    {
      if (errno == EINPROGRESS)
	{
	  setConnState (CONN_CONNECTING);
	  return;
	}
      return;
    }
  setConnState (CONN_CONNECTED);
}

Rts2ClientTCPDataConn::~Rts2ClientTCPDataConn (void)
{
  if (data)
    delete[]data;
}

int
Rts2ClientTCPDataConn::receive (fd_set * set)
{
  size_t data_size = 0;
  if (isConnState (CONN_DELETE))
    return -1;
  if ((sock >= 0) && FD_ISSET (sock, set))
    {
      data_size = read (sock, dataTop, totalSize - receivedSize);
      if (data_size <= 0)
	{
	  connectionError (data_size);
	  return -1;
	}
      successfullRead ();
      receivedSize += data_size;
      dataTop += data_size;
    }
  if (receivedSize == totalSize)
    {
      dataReceived ();
      endConnection ();
    }
  return data_size;
}

int
Rts2ClientTCPDataConn::idle ()
{
  if (isConnState (CONN_CONNECTING))
    {
      int err = 0;
      int ret;
      socklen_t len = sizeof (err);

      ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
      if (ret)
	{
#ifdef DEBUG_EXTRA
	  syslog (LOG_ERR, "Rts2ConnClient::idle getsockopt %m");
#endif
	  connectionError (-1);
	}
      else if (err)
	{
#ifdef DEBUG_EXTRA
	  syslog (LOG_ERR, "Rts2ConnClient::idle getsockopt %s",
		  strerror (err));
#endif
	  if (err == EINPROGRESS)
	    {
	      if (!reachedSendTimeout ())
		return 0;
	    }
	  connectionError (-1);
	}
      else
	{
	  setConnState (CONN_CONNECTED);
	}
    }
  if (reachedSendTimeout ())
    {
      connectionError (-1);
    }
  return 0;			// we don't want Rts2Conn to take care of our timeouts
}

void
Rts2ClientTCPDataConn::dataReceived ()
{
  ownerConnection->dataReceived (this);
}
