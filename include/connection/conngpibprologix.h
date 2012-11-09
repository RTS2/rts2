/* 
 * Connection for GPIB-serial manufactured by Prologix.biz
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

#ifndef __RTS2_CONN_GPIB_PROLOGIX__
#define __RTS2_CONN_GPIB_PROLOGIX__

#include "connection/conngpib.h"

#include "block.h"
#include "connection/serial.h"


namespace rts2core
{
/**
 * Prologix GPIB-USB convertor.
 *
 * Manual at http://prologix.biz/getfile?attachment_id=2
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class ConnGpibPrologix:public ConnGpib, rts2core::ConnSerial
{
	public:
		ConnGpibPrologix (rts2core::Block *_master, const char *_device, int _pad);
		virtual ~ ConnGpibPrologix (void);

		virtual void setDebug (bool _debug = true) { rts2core::ConnSerial::setDebug (_debug); }

		virtual void initGpib ();

		virtual void gpibWriteBuffer (const char *cmd, int _len);
		virtual void gpibRead (void *cmd, int &blen);
		virtual void gpibWriteRead (const char *cmd, char *reply, int blen);

		virtual void gpibWaitSRQ ();

		virtual void devClear ();

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

		int pad;
		int eot;
		int eos;

		float timeout;

		uint16_t len;
};

};
#endif		 /* !__RTS2_CONN_GPIB_PROLOGIX__ */
