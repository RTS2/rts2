/* 
 * Class which contains routines to resolve IP address from hostname.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_ADDRESS__
#define __RTS2_ADDRESS__

#include <string.h>

#include "status.h"
#include <config.h>

// CYGWIN workaround
#ifndef HAVE_GETADDRINFO
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "getaddrinfo.h"
#endif


/**
 * Class representing device TCP/IP address. It is ussed to hold device
 * informations obtained from central server.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2Address
{
	private:
		char name[DEVICE_NAME_SIZE];
		char *host;
		int port;
		int type;
	public:
		/**
		 * Construct Rts2Address object.
		 *
		 * @param in_name  Block name.
		 * @param in_host  Hostname of the host running the block.
		 * @param in_port  Port on which block listen for incoming requests.
		 * @param in_type  Block type.
		 */
		Rts2Address (const char *in_name, const char *in_host, int in_port, int in_type);
		virtual ~ Rts2Address (void);
		int update (const char *in_name, const char *new_host, int new_port, int new_type);
		int getSockaddr (struct addrinfo **info);

		/**
		 * Return block name.
		 *
		 * @return Block name.
		 */
		char *getName ()
		{
			return name;
		}

		/**
		 * Return block type.
		 *
		 * @return Block type.
		 */
		int getType ()
		{
			return type;
		}

		/**
		 * Check if the block is for given name.
		 *
		 * @return True if it is block address for given name, false otherwise.
		 */
		bool isAddress (const char *in_name)
		{
			return !strcmp (in_name, name);
		}
};
#endif							 /* !__RTS2_ADDRESS__ */
