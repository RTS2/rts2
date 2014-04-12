/* 
 * Sitech connection.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

#include "serial.h"

namespace rts2teld
{

/**
 * Sitech protocol connections. Used to control Sitech mounts.
 * 
 * Protocol details should be available at:
 * https://docs.google.com/document/d/10P7L-WyDicIjkYnuNnAuRSgEJ6p6HEzD2aKb2ygj9OI/pub
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnSitech: public rts2core::ConnSerial
{
	public:
		/**
		 * Construct new Sitech connection.
		 *
		 * @param devName device name
		 * @param master reference to master block
		 */
		ConnSitech (const char *devName, rts2core::Block *master);

		/**
		 * Initialize connection. Switch to checksumed mode.
		 */
		virtual int init ();

		/**
		 * Execute command on the axis. Don't expect any return.
		 *
		 * @param axis Command axis (X, Y, T, U, V or W)
		 * @param cmd  Command to execute (see SiTech doc for details)
		 *
		 * @throw rts2core::Error on communication error
		 */
		void siTechCommand (const char axis, const char *cmd);

		/**
		 * Return SiTech value.
		 *
		 * @param axis Axis from which value will be retrieved.
		 * @param val  Value name to retrieve
		 *
		 * @return Value of the given axis/value combination
		 *
		 * @throw rts2core::Error on communication error
		 */
		int32_t getSiTechValue (const char axis, const char *val);

		/**
		 * Executes .XS request and return response.
		 *
		 * @param axis          Axis for which status will be retrieved.
		 * @param address       Returned module address
		 * @param x_pos         X (Alt/Dec) motor position
		 * @param y_pos         Y (Az/RA) motor position
		 * @param x_enc         X (Alt/Dec) encoder readout
		 * @param y_enc         Y (Ax/RA) encoder readout
		 * @param keypad        Keypad status
		 * @param x_bit         XBits
		 * @param y_bit         YBits
		 * @param extra_bit     Extra bits
		 * @param ain_1         Analog input 1
		 * @param ain_2         Analog input 2
		 * @param mclock        Millisecond clock
		 * @param temperature   Temperature (probably CPU)
		 * @param y_worm_phase  Az/RA worm phase
		 * @param x_last        Alt/Dec motor location at last Alt/Dec scope encoder location change
		 * @param y_last        Az/RA motor location at last Az/RA scope encoder location change
		 *
		 * @throw rts2core::Error on communication error
		 */
		void getAxisStatus (char axis, int &address, int32_t &x_pos, int32_t &y_pos, int32_t &x_enc, int32_t &y_enc, char &keypad, char &x_bit, char &y_bit, char &extra_bit, int16_t &ain_1, int16_t &ain_2, uint32_t &mclock, int8_t &temperature, int8_t &y_worm_phase, int32_t &x_last, int32_t &y_last);

		void setSiTechValue (const char axis, const char *val, int value);

	private:
		void writePortChecksumed (const char *cmd, size_t len);

		uint8_t calculateChecksum (const char *buf, size_t len);
};

}
