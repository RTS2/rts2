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
#include "utilsfunc.h"

#include <strings.h>
#include <sys/socket.h>

using namespace rts2core;

ConnModbus::ConnModbus ():ConnNoSend (NULL)
{
}

ConnModbus::~ConnModbus ()
{
}

void ConnModbus::callFunction (uint8_t slaveId, int8_t func, const unsigned char *data, size_t data_size, unsigned char *reply, size_t reply_size)
{
	unsigned char send_data[2 + data_size];
	unsigned char reply_data[2 + reply_size];
	// fill header
	send_data[0] = slaveId;
	send_data[1] = func;
	bcopy (data, send_data + 2, data_size);
	exchangeData (send_data, data_size + 2, reply_data, reply_size + 2);
	if (reply_data[0] != slaveId)
	{
		std::ostringstream _os;
		_os << "invalid reply ID " << reply_data[0] << " " << slaveId;
		throw ModbusError (this, _os.str ().c_str ());
	}
	if (reply_data[1] & 0x80)
	{
		std::ostringstream _os;
		_os << "Error executing function " << func << " error code is: 0x" << std::hex << (int) reply_data[1];
		throw ModbusError (this, _os.str ().c_str ());
	}
	if ((uint8_t) reply_data[1] != func)
	{
		std::ostringstream _os;
		_os << "Invalid reply from modbus read, reply function is 0x" << std::hex << (int) reply_data[1] << ", expected 0x" << std::hex << (int) func;
		throw ModbusError (this, _os.str ().c_str ());
	}
	bcopy (reply_data + 2, reply, reply_size);
}

void ConnModbus::callFunction (uint8_t slaveId, int8_t func, int16_t p1, int16_t p2, unsigned char *reply, size_t reply_size)
{
	int16_t req_data[2];
	req_data[0] = htons (p1);
	req_data[1] = htons (p2);

	callFunction (slaveId, func, (unsigned char*) req_data, 4, reply, reply_size);
}

void ConnModbus::callFunction16 (uint8_t slaveId, int8_t func, int16_t p1, int16_t p2, uint16_t *reply_data, int16_t qty)
{
	int reply_size = 1 + qty * 2;
	unsigned char reply[reply_size];

	callFunction (slaveId, func, p1, p2, reply, reply_size);

	if (qty * 2 != reply[0])
	{
		std::ostringstream _os;
		_os << "invalid lenght in read reply: " << (int) reply[0] << " " << (qty * 2);
		throw ModbusError(this, _os.str ().c_str ());
	}

	unsigned char *rtop = reply + 1;

	for (reply_size = 0; reply_size < qty; reply_size++)
	{
		reply_data[reply_size] = ntohs (*((uint16_t *) rtop));
		rtop += 2;
	}
}

void ConnModbus::readCoils (uint8_t slaveId, int16_t start, int16_t size)
{
	int reply_size = 1 + (size / 8) + ((size % 8) == 0 ? 0 : 1);
	unsigned char reply_data[reply_size];

	callFunction (slaveId, 0x01, start, size, reply_data, reply_size);
}

void ConnModbus::readDiscreteInputs (uint8_t slaveId, int16_t start, int16_t size)
{
	int reply_size = 1 + (size / 8) + ((size % 8) == 0 ? 0 : 1);
	unsigned char reply_data[reply_size];

	callFunction (slaveId, 0x02, start, size, reply_data, reply_size);
}

void ConnModbus::readHoldingRegisters (uint8_t slaveId, int16_t start, int16_t qty, uint16_t *reply_data)
{
	callFunction16 (slaveId, 0x03, start, qty, reply_data, qty);
}

void ConnModbus::readInputRegisters (uint8_t slaveId, int16_t start, int16_t qty, uint16_t *reply_data)
{
	callFunction16 (slaveId, 0x04, start, qty, reply_data, qty);
}

void ConnModbus::writeHoldingRegister (uint8_t slaveId, int16_t reg, int16_t val)
{
	unsigned char reply[4];
	callFunction (slaveId, 0x06, reg, val, reply, 4);
}

void ConnModbus::writeHoldingRegisters (uint8_t slaveId, int16_t start, int8_t len, uint16_t *vals)
{
	unsigned char data[len * 2 + 5];
	unsigned char reply[4];
	*((int16_t *) data) = htons (start);
	*((int16_t *) (data + 2)) = htons ((int16_t) len);
	data[4] = len * 2;
	memcpy (data + 5, vals, len * 2);

	callFunction (slaveId, 0x10, data, len * 2 + 5, reply, 4);
}

void ConnModbus::writeHoldingRegisterMask (uint8_t slaveId, int16_t reg, int16_t mask, int16_t val)
{
	uint16_t old_value;
	readHoldingRegisters (slaveId, reg, 1, &old_value);
	old_value &= ~mask;
	old_value |= (val & mask);
	writeHoldingRegister (slaveId, reg, old_value);
}

ConnModbusTCP::ConnModbusTCP (Block * _master, const char *_hostname, int _port):ConnTCP (_master, _hostname, _port), ConnModbus ()
{
	transId = 1;
}

int ConnModbusTCP::init ()
{
	return ConnTCP::init ();
}

void ConnModbusTCP::setDebug (int d)
{
	return ConnTCP::setDebug (d);
}

void ConnModbusTCP::exchangeData (const void *payloadData, size_t payloadSize, void *reply, size_t replySize)
{
	char send_data[6 + payloadSize];
	// fill header
	*((uint16_t *) send_data) = htons (transId);
	send_data[2] = 0;
	send_data[3] = 0;
	*((uint16_t *) (send_data + 4)) = htons (payloadSize);
	bcopy (payloadData, send_data + 6, payloadSize);
	try
	{
		sendData (send_data, payloadSize + 6);
		replySize += 6;
		char reply_data[replySize];
		receiveData (reply_data, replySize, 50);

		uint16_t replyId = ntohs (*((uint16_t *) reply_data));
		if (replyId != transId)
		{
			std::ostringstream _os;
			_os << "invalid ID in reply data " << replyId << " " << transId;
			throw ModbusError (static_cast<ConnTCP *> (this), _os.str ().c_str ());
		}

		bcopy (reply_data + 6, reply, replySize - 6);
		transId++;
	}
	catch (ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		throw (err);
	}

}

ConnModbusRTUTCP::ConnModbusRTUTCP (Block * _master, const char *_hostname, int _port):ConnTCP (_master, _hostname, _port), ConnModbus ()
{
}

int ConnModbusRTUTCP::init ()
{
	ConnTCP::setReconnectTime (0);
	return ConnTCP::init ();
}

void ConnModbusRTUTCP::setDebug (int d)
{
	return ConnTCP::setDebug (d);
}

void ConnModbusRTUTCP::exchangeData (const void *payloadData, size_t payloadSize, void *reply, size_t replySize)
{
	uint16_t crc16 = htole16 (getMsgBufCRC16 ((char *) payloadData, payloadSize));
	try
	{
		sendData (payloadData, payloadSize);
		sendData (&crc16, 2);
		replySize += 2;
		char reply_data[replySize];
		receiveData (reply_data, replySize, 50);
		crc16 = htole16 (getMsgBufCRC16 (reply_data, replySize - 2));
		if (crc16 != *((uint16_t*) &(reply_data[replySize - 2])))
		{
			throw ModbusError (static_cast<ConnTCP *> (this), "invalid CRC");
		}
		bcopy (reply_data, reply, replySize - 2);
	}
	catch (ConnError err)
	{
		logStream (MESSAGE_ERROR) << err << sendLog;
		throw (err);
	}
}
