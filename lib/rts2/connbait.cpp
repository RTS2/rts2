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

#include "connbait.h"

#include <algorithm>
#include <iomanip>
#include <netdb.h>
#include <fcntl.h>
#include <sstream>
#include <sys/socket.h>

using namespace rts2core;

BAIT::BAIT (rts2core::Block *_master, std::string _hostname, int _port):ConnTCP (_master, _hostname.c_str(), _port)
{
}

BAIT::~BAIT ()
{
}

int BAIT::idle ()
{
	return 0;
}

int BAIT::receive (fd_set *fset)
{
	return -1;
}

void BAIT::writeRead (const char *cmd, char *reply, size_t buf_len)
{
	int cmdlen = strlen (cmd) + 1;
	int ret = send (sock, cmd, cmdlen, 0);
	if (ret != cmdlen)
		throw rts2core::Error ("cannot send BAIT command");
	char *buftop = reply;
	size_t bufused = 0;
	while (true)
	{
		cmdlen = recv (sock, buftop, buf_len - bufused, 0);
		if (cmdlen < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			throw rts2core::Error ("cannot receive reply from BAIT");
		}
		if (cmdlen == 0)
			continue;
		buftop += cmdlen;
		bufused += cmdlen;
		if (*(buftop - 1) == '\0')
		{
			std::cout << "buf : " << reply << " len " << bufused << " cmdlen " << cmdlen << std::endl;
			return;
		}
		if (bufused >= buf_len)
			throw rts2core::Error ("too long server reply");
	}
}
