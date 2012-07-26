/* 
 * Connection for GPIB-serial manufactured by Serial.biz
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_CONN_GPIB_SERIAL__
#define __RTS2_CONN_GPIB_SERIAL__

#include "conngpib.h"

#include "block.h"
#include "connection/serial.h"

namespace rts2sensord
{
/**
 * Serial connection emulating GPIB (IEEE-488).
 *
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ConnGpibSerial:public ConnGpib, rts2core::ConnSerial
{
	public:
		ConnGpibSerial (rts2core::Block *_master, const char *_device, rts2core::bSpeedT _baudSpeed = rts2core::BS9600, rts2core::cSizeT _cSize = rts2core::C8, rts2core::parityT _parity = rts2core::NONE, const char *_sep = "\n");
		virtual ~ ConnGpibSerial (void);

		virtual void setDebug (bool _debug = true) { rts2core::ConnSerial::setDebug (_debug); }

		virtual void initGpib ();

		virtual void gpibWriteBuffer (const char *cmd, int _len);
		virtual void gpibRead (void *cmd, int &blen);
		virtual void gpibWriteRead (const char *cmd, char *reply, int blen);

		virtual void gpibWaitSRQ ();

		virtual void devClear () { flushPortIO (); }
		virtual float gettmo () { return timeout; };
		virtual void settmo (float _sec);

		/**
		 * Sets EOT flag - whenever end characters are send.
		 */
		void setEot (int _eot)
		{
			eot = _eot;
		}

	private:
		void readUSBGpib (char *reply, int blen);

		int eot;
		int eos;

		float timeout;

		const char *sep;
		size_t seplen;
};

};
#endif		 /* !__RTS2_CONN_GPIB_SERIAL__ */
