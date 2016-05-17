/* 
 * Infrastructure for UDP connection.
 * Copyright (C) 2005-2015 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef __RTS2_CONNUDP__
#define __RTS2_CONNUDP__

#include <sys/types.h>
#include <time.h>
#include <arpa/inet.h>

#include "connnosend.h"

namespace rts2core
{

/**
 * UDP connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @autor Standa Vitek <standa@vitkovi.net>
 */
class ConnUDP:public ConnNoSend
{
	public:
		/**
		 * Configure UDP connection.
		 *
		 * @param hostname   host to which data will be send, can be left NULL
		 */
		ConnUDP (int _port, rts2core::Block * _master, const char* hostname = NULL, size_t _maxSize = 500);
		virtual int init ();
		virtual int receive (Block *block);
		/**
		 * @param rectimeout receive timeout in seconds
		 */
		int sendReceive (const char * in_message, char * out_message, unsigned int length, int noreceive = 0, float rectimeout=10);
	protected:
		/**
		 * Process received data. Data are stored in buf member variable.
		 *
		 * @param len   received data length
		 * @param from  address of the source (originator) of data
		 */
		virtual int process (size_t len, struct sockaddr_in &from) = 0;
		virtual void connectionError (int last_data_size)
		{
			// do NOT call Connection::connectionError, UDP connection must run forewer.
			return;
		}
	private:
		const char *hostname;
		size_t maxSize;
		struct sockaddr_in servaddr, clientaddr;
};

}

#endif // !__RTS2_CONNUDP__
