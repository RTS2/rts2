#include "connection/apm.h"

#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace rts2core;

ConnAPM::ConnAPM (rts2core::Block *_master, const char *_hostname, int _port):ConnNoSend (_master), hostname (_hostname)
{
	port = _port;
	hostname = _hostname;
}

int ConnAPM::init ()
{
	sock = socket (AF_INET, SOCK_DGRAM, 0);

        bzero (&servaddr, sizeof (servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr (hostname);
        servaddr.sin_port = htons (port);

        bzero (&clientaddr, sizeof (clientaddr));
        clientaddr.sin_family = AF_INET;
        clientaddr.sin_port = htons (port);
        clientaddr.sin_addr.s_addr = htonl (INADDR_ANY);

        if (bind (sock , (struct sockaddr*)&clientaddr, sizeof(clientaddr) ) == -1)
        {
                logStream (MESSAGE_ERROR) << "Unable to bind socket to port" << sendLog;
                return 1;
        }
}
