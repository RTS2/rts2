/* 
 * Forwarding GRB connection.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_GRBFW__
#define __RTS2_GRBFW__

#include "block.h"
#include "connnosend.h"

/**
 * Defines forward connection.
 *
 * Enables GRBd to forward incoming GCN packets to other destinations.
 *
 * @author petr
 */
class Rts2GrbForwardConnection:public rts2core::ConnNoSend
{
	private:
		int forwardPort;
	public:
		Rts2GrbForwardConnection (rts2core::Block * in_master, int in_forwardPort);
		virtual int init ();
		// span new GRBFw connection
		virtual int receive (rts2core::Block *block);
};

class Rts2GrbForwardClientConn:public rts2core::ConnNoSend
{
	private:
		void forwardPacket (int32_t *nbuf);
	public:
		Rts2GrbForwardClientConn (int in_sock, rts2core::Block * in_master);
		virtual void postEvent (rts2core::Event * event);

		virtual int receive (rts2core::Block *block);
};
#endif							 /* !__RTS2_GRBFW__ */
