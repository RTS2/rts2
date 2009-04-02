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

#include "conntcp.h"

namespace rts2core
{

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
class ConnModbus: public ConnTCP
{
	private:
		bool debugModbusComm;

		int16_t transId;
		char unitId;

	public:
		/**
		 * Create connection to TCP/IP modbus server.
		 *
		 * @param _master     Reference to master holding this connection.
		 * @param _hostname   Modbus server IP address or hostname.
		 * @param _port       Modbus server port number (default is 502).
		 */
		ConnModbus (Rts2Block *_master, const char *_hostname, int _port);


		/**
		 * Set if debug messages from port communication will be printed.
		 *
		 * @param printDebug  True if all port communication should be written to log.
		 */
		void setDebug (bool printDebug = true)
		{
			debugModbusComm = printDebug;
		}

		/**
		 * Call Modbus function.
		 *
		 * @param func       Function index.
		 * @param data       Function data.
		 * @param data_size  Size of input data.
		 * @param reply      Data returned from function call.
		 * @param reply_size Size of return data.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int callFunction (char func, const void *data, size_t data_size, void *reply, size_t reply_size);

		/**
		 * Call modbus function with two unsigned integer parameters and return expected as char array.
		 *
		 * @param func       Function index.
		 * @param p1         First parameter.
		 * @param p2         Second parameter.
		 * @param reply      Data returned from function call.
		 * @param reply_size Number of returned integers expected from the call.
		 */
		int callFunction (char func, int16_t p1, int16_t p2, void *reply, size_t reply_size);

		/**
		 * Call modbus function with two unsigned integer parameters and return expected as array of unsigned interger.
		 *
		 * @param func       Function index.
		 * @param p1         First parameter.
		 * @param p2         Second parameter.
		 * @param reply      Data returned from function call.
		 * @param qty        Number of returned integers expected from the call.
		 */
		int callFunction (char func, int16_t p1, int16_t p2, uint16_t *reply_data, int16_t qty);

		/**
		 * Read Modbus PLC coil states.
		 *
		 * @param start   Coil start address.
		 * @param size    Number of coils to read.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int readCoils (int16_t start, int16_t size);

		/**
		 * Read Modbus PLC discrete input states.
		 *
		 * @param start   Discrete input start address.
		 * @param size    Quantity of inputs.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int readDiscreteInputs (int16_t start, int16_t size);

		/**
		 * Read holding registers.
		 *
		 * @param start      Holding register starting address.
		 * @param qty        Quantity of registers.
		 * @param reply_data Returned data, converted to uint16_t (including network endian conversion).
		 *
		 * @return -1 on error, 0 on success.
		 */
		int readHoldingRegisters (int16_t start, int16_t qty, uint16_t *reply_data);

		/**
		 * Read input registers.
		 *
		 * @param start      Input register starting address.
		 * @param qty        Quantity of registers.
		 * @param reply_data Returned data, converted to uint16_t (including network endian conversion).
		 *
		 * @return -1 on error, 0 on success.
		 */
		int readInputRegisters (int16_t start, int16_t qty, uint16_t *reply_data);

		/**
		 * Write value to a register.
		 *
		 * @param reg  Register address.
		 * @param val  New register value.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int writeHoldingRegister (int16_t reg, int16_t val);
};

}
