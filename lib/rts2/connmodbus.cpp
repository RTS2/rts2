/* 
 * Generic Modbus TCP/IP connection.
 * Copyright (C) 2008-2009 Petr Kubanek <petr@kubanek.net>
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

#include "connection/modbus.h"

#include <strings.h>
#include <sys/socket.h>

using namespace rts2core;

ConnModbus::ConnModbus (Block * _master, const char *_hostname, int _port):ConnTCP (_master, _hostname, _port)
{
	transId = 1;
	unitId = 0;
}

void ConnModbus::callFunction (char func, const void *data, size_t data_size, void *reply, size_t reply_size)
{
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
	try
	{
		sendData (send_data, data_size);
		reply_size += 8;
		char reply_data[reply_size];
		receiveData (reply_data, reply_size, 50);

		if (reply_data[7] & 0x80)
		{
			std::ostringstream _os;
			_os << "Error executiong function " << func 
				<< " error code is: 0x" << std::hex << (int) reply_data[8];
			throw ModbusError (this, _os.str ().c_str ());
		}
		else if (reply_data[7] != func)
		{
		  	std::ostringstream _os;
			_os << "Invalid reply from modbus read, reply function is 0x" << std::hex << (int) reply_data[7]
				<< ", expected 0x" << std::hex << (int) func;
			throw ModbusError (this, _os.str ().c_str ());
		}
		bcopy (reply_data + 8, reply, reply_size - 8);
		transId++;
	}
	catch (ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		throw (err);
	}
}

void ConnModbus::callFunction (char func, int16_t p1, int16_t p2, void *reply, size_t reply_size)
{
	int16_t req_data[2];
	req_data[0] = htons (p1);
	req_data[1] = htons (p2);

	callFunction (func, req_data, 4, reply, reply_size);
}

void ConnModbus::callFunction (char func, int16_t p1, int16_t p2, uint16_t *reply_data, int16_t qty)
{
	int reply_size = 1 + qty * 2;
	char reply[reply_size];

	callFunction (func, p1, p2, (void *) reply, reply_size);

	if (reply[0] != qty * 2)
	{
		throw ModbusError (this, "Invalid quantity in reply packet!");
	}

	char *rtop = reply + 1;

	for (reply_size = 0; reply_size < qty; reply_size++)
	{
		reply_data[reply_size] = ntohs (*((uint16_t *) rtop));
		rtop += 2;
	}
}

void ConnModbus::readCoils (int16_t start, int16_t size)
{
	int reply_size = 1 + (size / 8) + ((size % 8) == 0 ? 0 : 1);
	char reply_data[reply_size];

	callFunction (0x01, start, size, reply_data, reply_size);
}

void ConnModbus::readDiscreteInputs (int16_t start, int16_t size)
{
	int reply_size = 1 + (size / 8) + ((size % 8) == 0 ? 0 : 1);
	char reply_data[reply_size];

	callFunction (0x02, start, size, reply_data, reply_size);
}

void ConnModbus::readHoldingRegisters (int16_t start, int16_t qty, uint16_t *reply_data)
{
	callFunction (0x03, start, qty, reply_data, qty);
}

void ConnModbus::readInputRegisters (int16_t start, int16_t qty, uint16_t *reply_data)
{
	callFunction (0x04, start, qty, reply_data, qty);
}

void ConnModbus::writeHoldingRegister (int16_t reg, int16_t val)
{
  	int16_t reply[2];
	callFunction (0x06, reg, val, reply, 4);
}

void ConnModbus::writeHoldingRegisterMask (int16_t reg, int16_t mask, int16_t val)
{
	uint16_t old_value;
	readHoldingRegisters (reg, 1, &old_value);
	old_value &= ~mask;
	old_value |= (val & mask);
	writeHoldingRegister (reg, old_value);
}
