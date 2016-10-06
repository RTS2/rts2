/* 
 * BAIT interface.
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_CONNBAIT__
#define __RTS2_CONNBAIT__

#include "tcp.h"

#include <string>
#include <ostream>

namespace rts2core
{

/**
 * Class for BAIT protocol.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class BAIT: public ConnTCP
{
	
	public:
		BAIT (rts2core::Block *_master, std::string _host_name, int _port);
		virtual ~BAIT ();

		virtual int idle ();
		
		virtual int receive (Block *block);

		/**
		 * Sends command, wait for reply.
		 */
		void writeRead (const char *cmd, char *reply, size_t buf_len);
};

}

#endif  /* !__RTS2_CONNOPENTPL__ */
