#ifndef __RTS2_ADDRESS__
#define __RTS2_ADDRESS__

#include <string.h>

#include "status.h"

// CYGWIN workaround
#ifndef HAVE_GETADDRINFO
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "getaddrinfo.h"
#endif

class Rts2Address
{
  char name[DEVICE_NAME_SIZE];
  char *host;
  int port;
  int type;
public:
    Rts2Address (const char *in_name, const char *in_host, int in_port,
		 int in_type);
    virtual ~ Rts2Address (void);
  int update (const char *in_name, const char *new_host, int new_port,
	      int new_type);
  int getSockaddr (struct addrinfo **info);
  char *getName ()
  {
    return name;
  }
  int getType ()
  {
    return type;
  }
  int isAddress (const char *in_name)
  {
    return !strcmp (in_name, name);
  }
};

#endif /* !__RTS2_ADDRESS__ */
