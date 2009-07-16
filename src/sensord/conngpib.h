/* 
 * Connection for GPIB bus.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONN_GPIB__
#define __RTS2_CONN_GPIB__

#include "../utils/rts2connnosend.h"

#include <gpib/ib.h>

namespace rts2sensord
{

/**
 * Connection to a single GPIB device.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnGpib:public Rts2ConnNoSend
{
	private:
		int minor;
		int pad;

		int gpib_dev;
	public:
		int gpibWrite (const char *_buf);

		/*
		 * Read data from GPIB device to a buffer.
		 *
		 * @param _buf  Buffer where data will be stored.
		 * @param blne  Buffer length in bytes.
		 * @return -1 on error, number of bytes readed on success.
		 */
		int gpibRead (void *_buf, int blen);
		int gpibWriteRead (const char *_buf, char *val, int blen = 50);

		int gpibWaitSRQ ();

		virtual int init ();

		void setMinor (int _minor) { minor = _minor; }
		void setPad (int _pad) { pad = _pad; }

		ConnGpib (Rts2Block *_master);
		virtual ~ ConnGpib (void);
};

};
#endif		 /* !__RTS2_CONN_GPIB__ */
