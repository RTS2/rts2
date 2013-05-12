/* 
 * Connection for SCPI-raw devices.
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_CONN_SCPI__
#define __RTS2_CONN_SCPI__

#include "conngpib.h"

namespace rts2core
{

/**
 * Perform communication with SCPI devices over raw socket.
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 * TODO Move SCPI commands to own class, keep GPIB as separate class.
 */
class ConnSCPI:public ConnGpib
{
	public:
		ConnSCPI (const char *_device, char _endDelimiter = '\n'):ConnGpib ()
		{
			device = _device;
			devfile = -1;

			timeout = USEC_SEC * 10;
			debug = false;

			endDelimiter = _endDelimiter;
		}

		virtual ~ConnSCPI () {};

		virtual void gpibWriteBuffer (const char *cmd, int len);

		virtual void gpibRead (void *reply, int &blen);

		virtual void gpibWriteRead (const char *cmd, char *reply, int blen);

		virtual void gpibWaitSRQ () {}

		virtual void initGpib ();

		virtual void devClear () {};

		virtual float gettmo () { return timeout / USEC_SEC; };

		virtual void settmo (float _sec) { timeout = _sec * USEC_SEC; }

		virtual void setDebug (bool _debug = true) { debug = _debug; }

		virtual bool isSerial () { return true; }
	
	private:
		const char *device;
		int devfile;

		long long timeout;
		bool debug;

		char endDelimiter;
};

}

#endif //! __RTS2_CONN_SCPI__
