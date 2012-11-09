/* 
 * Connection for GPIB bus.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONN_GPIB_ENET__
#define __RTS2_CONN_GPIB_ENET__

#include "conngpib.h"

#include "block.h"
#include "connection/tcp.h"


namespace rts2core
{

class ConnGpibEnet:public ConnGpib, public rts2core::ConnTCP
{
	public:
		virtual void gpibWriteBuffer (const char *cmd, int _len);
		virtual void gpibRead (void *cmd, int &blen);
		virtual void gpibWriteRead (const char *cmd, char *reply, int blen);

		virtual void gpibWaitSRQ ();

		virtual void initGpib ();

		virtual void devClear ();

		virtual float gettmo () { return timeout; };
		virtual void settmo (float _sec);

		/**
		 * Sets EOT flag - whenever end characters are send.
		 */
		void setEot (int _eot)
		{
			eot = _eot;
		}

		ConnGpibEnet (rts2core::Block *_master, const char *_address, int _port, int _pad);
		virtual ~ ConnGpibEnet (void);

		virtual void setDebug (bool _debug = true) { rts2core::ConnTCP::setDebug (_debug); }

	private:
		int sad;
		int pad;
		int eot;
		int eos;

		float timeout;

		uint16_t sta;
		uint16_t err;
		uint32_t cnt;

		uint16_t flags;
		uint16_t len;

		/**
		 * Read header and data from socket.
		 *
		 * @param ret_buf Buffer which will be filled with returned
		 * string. It will be allocated by this method, and must be
		 * freed by caller.
		 */
		void sread (char **ret_buf);

		/**
		 * Wait for and read socket response. Process reply header, which
		 * contains reply data. Returns full reply buffer, which must be deleted 
		 * - unless ret_buf is NULL.
		 */
		void sresp (char **ret_buf = NULL);
};

};
#endif		 /* !__RTS2_CONN_GPIB_ENET__ */
