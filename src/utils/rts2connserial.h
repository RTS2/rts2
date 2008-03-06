/* 
 * Generic serial port connection.
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

#include "rts2conn.h"

/**
 * Enum for baud speeds.
 */
typedef enum {BS4800, BS9600, BS115200}
bSpeedT;

/**
 * Enum for data size.
 */
typedef enum {C7, C8}
cSizeT;

/**
 * Enum for parity.
 */
typedef enum {NONE, ODD, EVEN}
parityT;

/**
 * Serial connection class.
 *
 * This class present single interface to set correctly serial port connection with
 * ussual parameters - baud speed, stop bits, and parity. It also have common functions
 * to do I/O on serial port, with proper error reporting throught logStream() mechanism.
 *
 * @ingroup RTS2Block
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConnSerial: public Rts2Conn
{
	private:
		bSpeedT baudSpeed;

		cSizeT cSize;
		parityT parity;

		int vMin;
		int vTime;

		// if we will preint port communication
		bool debugPortComm;

		/**
		 * Returns baud speed as string.
		 */
		const char *getBaudSpeed ();

		int getVMin ()
		{
			return vMin;
		}

		int getVTime ()
		{
			return vTime;
		}
	public:
		/**
		 * Create connection to serial port.
		 *
		 * @param in_devName   Device name (ussually /dev/ttySx)
		 * @param in_master    Controlling block
		 * @param in_baudSpeed Device baud speed
		 * @param in_cSize     Character bits size (7 or 8 bits)
		 * @param in_parity    Device parity.
		 * @param in_vTime     Time to wait for single read before giving up.
		 */
		Rts2ConnSerial (const char *in_devName, Rts2Block * in_master,
			bSpeedT in_baudSpeed = BS9600, cSizeT in_cSize = C8,
			parityT in_parity = NONE, int in_vTime = 40);
		virtual ~Rts2ConnSerial (void);

		/**
		 * Init serial port.
		 */
		virtual int init ();

		/**
		 * Write data to serial port.
		 *
		 * @param buf Buffer which will be written to the port.
		 * @param b_len Lenght of buffer to write on the port.
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		int writePort (const char *wbuf, int b_len);

		/**
		 * Raw read data from serial port.
		 *
		 * @param buf Buffer where data will be readed.
		 * @param b_len Lenght of buffer to read data.
		 *
		 * @return -1 on error, 0 on sucess.
		 */
		int readPort (char *rbuf, int b_len);

		/**
		 * Read full line from serial port.
		 */

		/**
		 * Flush serial port (make sure all data were sended and received).
		 */
		int flushPortIO ();

		/**
		 * Set if debug messages from port communication will be printed.
		 *
		 * @param printDebug  True if all port communication should be written to log.
		 */
		void setDebug (bool printDebug = true)
		{
			debugPortComm = printDebug;
		}

};
