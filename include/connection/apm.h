#ifndef __RTS2_CONNECTION_APM__
#define __RTS2_CONNECTION_APM__

#include "udp.h"
#include "error.h"

#include <ostream>

/**
 * Class for APM devices connection
 *
 * Provides basic operations on UDP connection - establish it,
 * send and receive data. 
 *
 * @autor Standa Vitek <standa@vitkovi.net>
 */

namespace rts2core
{

class ConnAPM:public ConnUDP
{
	public:
		ConnAPM (int _port, rts2core::Block *_master, const char *_hostname);

	protected:
		virtual int process (size_t len, struct sockaddr_in &from);
};

}

#endif // !__RTS2_CONNECTION_APM__
