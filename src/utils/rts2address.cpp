#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <malloc.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>

#include "rts2address.h"

Rts2Address::Rts2Address (const char *in_name, const char *in_host,
			  int in_port, int in_type)
{
  strncpy (name, in_name, DEVICE_NAME_SIZE);
  name[DEVICE_NAME_SIZE - 1] = '\0';
  host = new char[strlen (in_host) + 1];
  strcpy (host, in_host);
  port = in_port;
  type = in_type;
}

Rts2Address::~Rts2Address (void)
{
  delete[]host;
}

int
Rts2Address::update (const char *in_name, const char *new_host, int new_port,
		     int new_type)
{
  if (!isAddress (in_name))
    return -1;
  delete[]host;
  host = new char[strlen (new_host) + 1];
  strcpy (host, new_host);
  port = new_port;
  type = new_type;
  return 0;
}

int
Rts2Address::getSockaddr (struct addrinfo **info)
{
  int ret;
  char *s_port;
  struct addrinfo hints;
  hints.ai_flags = 0;
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  asprintf (&s_port, "%i", port);
  ret = getaddrinfo (host, s_port, &hints, info);
  free (s_port);
  if (ret)
    {
      syslog (LOG_ERR, "Rts2Address::getAddress getaddrinfor: %s",
	      gai_strerror (ret));
      return -1;
    }
  return 0;
}
