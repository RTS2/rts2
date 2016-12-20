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

#include "tcp.h"

namespace rts2core
{

/**
 * Class for modbus TPC/IP errors.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ModbusError:public ConnError
{
	public:
		ModbusError (Connection *conn, const char *desc):ConnError (conn, desc)
		{
		}
};

/**
 * Abstract class for Modbus connection. Ancestor of various Modbus implementations.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubánek <kubanek@fzu.cz>
 */
class ConnModbus:public ConnNoSend
{
	public:
		ConnModbus ();

		virtual ~ConnModbus ();

		virtual int init () = 0;

		virtual void setDebug (int d) = 0;

		/**
		 * Call Modbus function.
		 *
		 * @param func       Function index.
		 * @param data       Function data.
		 * @param data_size  Size of input data.
		 * @param reply      Data returned from function call.
		 * @param reply_size Size of return data.
		 *
		 * @throw ConnError if connection error occured.
		 */
		void callFunction (uint8_t slaveId, int8_t func, const unsigned char *data, size_t data_size, unsigned char *reply, size_t reply_size);

		/**
		 * Call modbus function with two unsigned integer parameters and return expected as char array.
		 *
		 * @param func       Function index.
		 * @param p1         First parameter.
		 * @param p2         Second parameter.
		 * @param reply      Data returned from function call.
		 * @param reply_size Number of returned integers expected from the call.
		 *
		 * @throw ConnError if connection error occured.
		 */
		void callFunction (uint8_t slaveId, int8_t func, int16_t p1, int16_t p2, unsigned char *reply, size_t reply_size);

		/**
		 * Call modbus function with two unsigned integer parameters and return expected as array of unsigned interger.
		 *
		 * @param func       Function index.
		 * @param p1         First parameter.
		 * @param p2         Second parameter.
		 * @param reply      Data returned from function call.
		 * @param qty        Number of returned integers expected from the call.
		 */
		void callFunction16 (uint8_t slaveId, int8_t func, int16_t p1, int16_t p2, uint16_t *reply_data, int16_t qty);

		/**
		 * Read Modbus PLC coil states.
		 *
		 * @param start   Coil start address.
		 * @param size    Number of coils to read.
		 *
		 * @throw         ConnError on error.
		 */
		void readCoils (uint8_t slaveId, int16_t start, int16_t size);

		/**
		 * Read Modbus PLC discrete input states.
		 *
		 * @param start   Discrete input start address.
		 * @param size    Quantity of inputs.
		 *
		 * @throw         ConnError on error.
		 */
		void readDiscreteInputs (uint8_t slaveId, int16_t start, int16_t size);

		/**
		 * Read holding registers.
		 *
		 * @param start      Holding register starting address.
		 * @param qty        Quantity of registers.
		 * @param reply_data Returned data, converted to uint16_t (including network endian conversion).
		 *
		 * @throw            ConnError on error.
		 */
		void readHoldingRegisters (uint8_t slaveId, int16_t start, int16_t qty, uint16_t *reply_data);

		/**
		 * Read input registers.
		 *
		 * @param start      Input register starting address.
		 * @param qty        Quantity of registers.
		 * @param reply_data Returned data, converted to uint16_t (including network endian conversion).
		 *
		 * @throw            ConnError on error.
		 */
		void readInputRegisters (uint8_t slaveId, int16_t start, int16_t qty, uint16_t *reply_data);

		/**
		 * Write value to a register.
		 *
		 * @param reg  register address
		 * @param val  new register value
		 *
		 * @throw            ConnError on error.
		 */
		void writeHoldingRegister (uint8_t slaveId, int16_t reg, int16_t val);

		/**
		 * Write value to a register.
		 *
		 * @param start starting register address
		 * @param vals  new register value
		 *
		 * @throw            ConnError on error.
		 */
		void writeHoldingRegisters (uint8_t slaveId, int16_t start, int8_t len, uint16_t *vals);

		/**
		 * Write masked value to a register. Actually read register,
		 * mask out mask values, or with masked val, and write it back.
		 *
		 * @param reg  register address
		 * @param mask mask (only bits with this mask will be written).
		 * @param val  new register value.
		 *
		 * @throw            ConnError on error.
		 */
		void writeHoldingRegisterMask (uint8_t slaveId, int16_t reg, int16_t mask, int16_t val);

	protected:

		/**
		 * Sends data to client, waits for answer and receives reply.
		 * Must be overwritten in descendants, according to underlying transport mechanism.
		 */
		virtual void exchangeData (const void *modbusPayload, size_t payloadSize, void *reply, size_t replySize) = 0;
};

/**
 * Modbus TCP/IP connection class.
 *
 * This class is for TCP/IP connectin to an Modbus enabled device. It provides
 * methods to easy read and write Modbus coils etc..
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnModbusTCP: public ConnTCP, public ConnModbus
{
	public:
		/**
		 * Create connection to TCP/IP modbus server.
		 *
		 * @param _master     Reference to master holding this connection.
		 * @param _hostname   Modbus server IP address or hostname.
		 * @param _port       Modbus server port number (default is 502).
		 */
		ConnModbusTCP (rts2core::Block *_master, const char *_hostname, int _port);

		virtual int init ();

		virtual void setDebug (int d);

	protected:
		virtual void exchangeData (const void *modbusPayload, size_t payloadSize, void *reply, size_t replySize);

	private:
		int16_t transId;
};

class ConnModbusRTUTCP: public ConnTCP, public ConnModbus
{
	public:
		/**
		 * Create connection to TCP/IP modbus server.
		 *
		 * @param _master     Reference to master holding this connection.
		 * @param _hostname   Modbus server IP address or hostname.
		 * @param _port       Modbus server port number (default is 502).
		 */
		ConnModbusRTUTCP (rts2core::Block *_master, const char *_hostname, int _port);

		virtual int init ();

		virtual void setDebug (int d);

	protected:
		virtual void exchangeData (const void *modbusPayload, size_t payloadSize, void *reply, size_t replySize);
};

}
