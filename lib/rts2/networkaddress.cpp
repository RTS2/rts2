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

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>

#include "networkaddress.h"

using namespace rts2core;

NetworkAddress::NetworkAddress (int _host_num, int _centrald_num, int _centrald_id, const char *_name, const char *_host, int _port, int _type)
{
  	host_num = _host_num;
	centrald_num = _centrald_num;
	centrald_id = _centrald_id;
	strncpy (name, _name, DEVICE_NAME_SIZE);
	name[DEVICE_NAME_SIZE - 1] = '\0';
	host = new char[strlen (_host) + 1];
	strcpy (host, _host);
	port = _port;
	type = _type;
}

NetworkAddress::~NetworkAddress (void)
{
	delete[]host;
}

int NetworkAddress::update (int _centrald_num, const char *_name, const char *new_host, int new_port, int new_type)
{
	if (!isAddress (_centrald_num, _name))
		return -1;
	delete[]host;
	host = new char[strlen (new_host) + 1];
	strcpy (host, new_host);
	port = new_port;
	type = new_type;
	return 0;
}

int NetworkAddress::getSockaddr (struct addrinfo **info)
{
	char s_port[10];
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = 0;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	sprintf (s_port, "%i", port);
	return getaddrinfo (host, s_port, &hints, info);
}
