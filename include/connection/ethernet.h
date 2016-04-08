/* 
 * Simple connection using RAW ethernet (IEEE 802.3).
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net> and Jan Strobl
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

#ifndef __RTS2_CONNETHERNET__
#define __RTS2_CONNETHERNET__

#include "connnosend.h"
#include "error.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>


namespace rts2core
{


/**
 * Class for connection using RAW ethernet (IEEE 802.3).
 * For details, see e.g. http://aschauf.landshut.org/fh/linux/udp_vs_raw/ch01s03.html .
 *
 * @author Jan Strobl
 */
class ConnEthernet:public ConnNoSend
{
	public:
		/**
		 * Create new connection to remote server.
		 *
		 * @param _master	Reference to master holding this connection.
		 * @param _ethLocal 	Identifier (e.g. eth0) of local ethernet device, used for communication.
		 * @param _macRemote 	MAC adress of remote ethernet server, used for communication.
		 */
		ConnEthernet (rts2core::Block *_master, const char *_ethLocal, const unsigned char *_macRemote);
		virtual ~ ConnEthernet (void);

		/**
		 * Init connection to host given at constructor.
		 *
		 * @return -1 on error, 0 on success.
		 */
		virtual int init ();

		/**
		 * Set debugging flag. When debugging is enabled, all data send
		 * to and from the socket will be logged using standard RTS2
		 * logging with MESSAGE_DEBUG type.
		 */
		void setDebug (bool _debug = true) { debug = _debug; }

		/**
		 * Send data, wait for reply.
		 *
		 * @param wbuf    buffer with data to write
		 * @param wlne    length of the write buffer
		 * @param rbuf    buffer where read data will be stored
		 * @param rlen    length of the read buffer
		 * @param wtime   connection timeout
		 *
		 * @return size of data read on success.
		 */
		int writeRead (const void* wbuf, int wlen, void *rbuf, int rlen, int wtime = 1);

	protected:
		bool getDebug () { return debug; }

	private:
		int sockE;
		struct sockaddr_ll socket_address;
		char ethLocal[10];
		unsigned char macLocal[6];
		unsigned char macRemote[6];
		void* ethOutBuffer;
		unsigned char* ethOutHead;
		unsigned char* ethOutData;
		struct ethhdr *eh;
		void* ethInBuffer;
		unsigned char* ethInData;

		bool debug;
};

};

#endif // !__RTS2_CONNETHERNET__
