/* 
 * Generic Modbus TCP/IP connection.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "connmodbus.h"

#include <strings.h>
#include <sys/socket.h>

using namespace rts2core;

ConnModbus::ConnModbus (Rts2Block * _master, const char *_hostname, int _port)
:ConnTCP (_master, _hostname, _port)
{
	transId = 1;
	unitId = 0;
}


int
ConnModbus::callFunction (char func, const void *data, size_t data_size, void *reply, size_t reply_size)
{
	int ret;
	char send_data[8 + data_size];
	// fill header
	*((uint16_t *) send_data) = htons (transId);
	send_data[2] = 0;
	send_data[3] = 0;
	*((uint16_t *) (send_data + 4)) = htons (data_size + 2);
	send_data[6] = unitId;
	send_data[7] = func;
	bcopy (data, send_data + 8, data_size);
	data_size += 8;
	ret = send (sock, send_data, data_size, 0);

	if (debugModbusComm)
	{
		Rts2LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "send ";
		ls.logArrAsHex (send_data, data_size);
		ls << sendLog;
	}

	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "cannot send data to socket, error: " << strerror (errno) << sendLog;
		connectionError (-1);
		return -1;
	}
	else if (ret != (int) (data_size))
	{
		logStream (MESSAGE_ERROR) << "cannot send all data - strange, continuing. Send " <<
			ret << ", expected " << data_size << ". Continuuing" << sendLog;
	}

	fd_set read_set;

	struct timeval read_tout;
	read_tout.tv_sec = 2;
	read_tout.tv_usec = 0;

	FD_ZERO (&read_set);

	FD_SET (sock, &read_set);

	ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "error calling select function for modbus, error: " << strerror (errno)
			<< sendLog;
		connectionError (-1);
		return -1;
	}
	else if (ret == 0)
	{
	  	logStream (MESSAGE_ERROR) << "Timeout during call to select function. Calling connection error." << sendLog;
		connectionError (-1);
		return -1;
	}

	// read from descriptor
	reply_size += 8;
	char reply_data[reply_size];
	ret = recv (sock, reply_data, reply_size, 0);
	if (ret < 0)
	{
		logStream (MESSAGE_ERROR) << "Cannot read from Modbus socket, error " << strerror (errno) << sendLog;
		connectionError (-1);
		return -1;
	}

	if (debugModbusComm)
	{
		Rts2LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "recv ";
		ls.logArrAsHex (reply_data, reply_size);
		ls << sendLog;
	}

	if (reply_data[7] & 0x80)
	{
		logStream (MESSAGE_ERROR) << "Error executiong function " << func 
			<< " error code is: 0x" << std::hex << (int) reply_data[8] << sendLog;
		return -1;
	}
	else if (reply_data[7] != func)
	{
		logStream (MESSAGE_ERROR) << "Invalid reply from modbus read, reply function is 0x" << std::hex << (int) reply_data[7]
			<< ", expected 0x" << std::hex << (int) func << "." << sendLog;
		return -1;
	}
	else if (ret != (int) reply_size)
	{
		logStream (MESSAGE_ERROR) << "Cannot receive all reply data, received " << ret << ", expected " << reply_size << "." << sendLog;
		connectionError (-1);
		return -1;
	}
	bcopy (reply_data + 8, reply, reply_size - 8);
	transId++;
	return 0;
}


int
ConnModbus::callFunction (char func, int16_t p1, int16_t p2, void *reply, size_t reply_size)
{
	int16_t req_data[2];
	req_data[0] = htons (p1);
	req_data[1] = htons (p2);

	int ret = callFunction (func, req_data, 4, reply, reply_size);
	if (ret)
		return ret;
	return 0;
}


int
ConnModbus::callFunction (char func, int16_t p1, int16_t p2, uint16_t *reply_data, int16_t qty)
{
	int reply_size = 1 + qty * 2;
	char reply[reply_size];

	int ret = callFunction (func, p1, p2, (void *) reply, reply_size);
	if (ret)
		return ret;

	if (reply[0] != qty * 2)
	{
	 	logStream (MESSAGE_ERROR) << "Invalid quantity in reply packet!" << sendLog;
		return ret;
	}

	char *rtop = reply + 1;

	for (reply_size = 0; reply_size < qty; reply_size++)
	{
		reply_data[reply_size] = ntohs (*((uint16_t *) rtop));
		rtop += 2;
	}
	return 0;

}


int
ConnModbus::readCoils (int16_t start, int16_t size)
{
	int reply_size = 1 + (size / 8) + ((size % 8) == 0 ? 0 : 1);
	char reply_data[reply_size];

	int ret;
	ret = callFunction (0x01, start, size, reply_data, reply_size);

	return 0;
}


int
ConnModbus::readDiscreteInputs (int16_t start, int16_t size)
{
	int reply_size = 1 + (size / 8) + ((size % 8) == 0 ? 0 : 1);
	char reply_data[reply_size];

	int ret;
	ret = callFunction (0x02, start, size, reply_data, reply_size);

	return 0;
}


int
ConnModbus::readHoldingRegisters (int16_t start, int16_t qty, uint16_t *reply_data)
{
	return callFunction (0x03, start, qty, reply_data, qty);
}

int
ConnModbus::readInputRegisters (int16_t start, int16_t qty, uint16_t *reply_data)
{
	return callFunction (0x04, start, qty, reply_data, qty);
}

int
ConnModbus::writeHoldingRegister (int16_t reg, int16_t val)
{
  	int16_t reply[2];
	return callFunction (0x06, reg, val, reply, 4);
}
