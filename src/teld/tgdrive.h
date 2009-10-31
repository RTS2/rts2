/* 
 * RS-232 TGDrives interface.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/connserial.h"

// interesting addresses

/** Servo control mode. */
#define TGA_MODE                         0x01EF

/** Digital torque mode */
#define TGA_MODE_DT                      0x1001

/** Digital speed mode */
#define TGA_MODE_DS                      0x2002

/** Analog torque mode */
#define TGA_MODE_AT                      0x8001

/** Analog speed mode */
#define TGA_MODE_AS                     0x10001

/** Stepper motor mode */
#define TGA_MODE_SM                     0x20004

/** Position absolute mode */
#define TGA_MODE_PA                     0x4004

/** Position relative (PG) */
#define TGA_MODE_PR_PG                 0x40004

/** Position relative (resolver) */
#define TGA_MODE_PR_RS                 0x80004

/** Encoder mode */
#define TGA_MODE_ENCODER              0x100004

/** Trajecory CAN mode */
#define TGA_MODE_CAN                  0x200004

/** Setting after reset */
#define TGA_AFTER_RESET                 0x01EE

/** After reset disabled */
#define TGA_AFTER_RESET_DISABLED        1

/** After reset enabled */
#define TGA_AFTER_RESET_ENABLED         2

/**
 * Target position */
#define TGA_TARPOS                      0x0192

/** Current positon */
#define TGA_CURRPOS                     0x0196

namespace rts2teld
{

/**
 * Error reported by TGDrive.
 */
class TGDriveError
{
	private:
		char status;
	public:
		TGDriveError (char _status)
		{
			status = _status;
		}
};

/**
 * Class for RS-232 connection to TGDrives motors. See http://www.tgdrives.cz fro details.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class TGDrive: public rts2core::ConnSerial
{
	private:
		// escape, checksum and write..
		void ecWrite (char *msg);
		// read and test checksum, with expected length
		void ecRead (char *msg, int len);

		void writeMsg (char op, int16_t address);
		void writeMsg (char op, int16_t address, char *data, int len);

		void readStatus ();

	public:
		/**
		 * Create connection to TGDrive motor.
		 *
		 * @param _devName  Device name (ussually /dev/ttySx)
		 * @param _master   Controlling block
		 */
		TGDrive (const char *_devName, Rts2Block *_master);

		/**
		 * Read word (2 bytes) from interface.
		 *
		 * @param address Address.
		 * @return Read data.
		 *
		 * @throw TGDriveError on error.
		 */
		int16_t read2b (int16_t address);

		/**
		 * Write word (2 bytes) to interface.
		 *
		 * @param address Address.
		 * @return data Data.
		 *
		 * @throw TGDriveError on error.
		 */
		void write2b (int16_t address, int16_t data);

		/**
		 * Read long (4 bytes) from interface.
		 *
		 * @param address Address.
		 * @return Read data.
		 *
		 * @throw TGDriveError on error.
		 */
		int32_t read4b (int16_t address);

		/**
		 * Write word (2 bytes) to interface.
		 *
		 * @param address Address.
		 * @return data Data.
		 *
		 * @throw TGDriveError on error.
		 */
		void write4b (int16_t address, int32_t data);
};

}
