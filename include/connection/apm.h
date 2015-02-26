#ifndef __RTS2_CONNECTION_APM__
#define __RTS2_CONNECTION_APM__

#include "connnosend.h"
#include "error.h"

#include <ostream>

/**
 * Class for APM devices connection
 *
 * Provides basic operations on UDP connection - establish it,
 * send and receive data. 
 *
 * @autor Standa Vitek standa@vitkovi.net
 */

namespace rts2core
{

class ConnAPM:public ConnNoSend
{
	public:
		ConnAPM (rts2core::Block *_master, const char *_hostname, int _port);

		virtual int init ();
		int send (const char * in_message, char * out_message, unsigned int length, int noreceive = 0);

	private:
		const char *hostname;
		int port;
		int sock;        
		struct sockaddr_in servaddr, clientaddr;
};

}

#endif // !__RTS2_CONNECTION_APM__
