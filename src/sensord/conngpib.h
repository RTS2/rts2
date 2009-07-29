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

namespace rts2sensord
{

/**
 * Class for connections to GPIB devices.
 *
 * This is an abstract class, which provides interface to GPIB. It provides
 * methods to communicate with IEEE-488 GPIB bus. It is subclassed by
 * ConnGpibLinux and ConnGpibEnet.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnGpib
{
	public:
		virtual void gpibWrite (const char *_buf) = 0;

		/*
		 * Read data from GPIB device to a buffer.
		 *
		 * @param _buf  Buffer where data will be stored.
		 * @param blne  Buffer length in bytes.
		 *
		 * @throw rts2core::Error and its descendants
		 */
		virtual void gpibRead (void *_buf, int &blen) = 0;
		virtual void gpibWriteRead (const char *_buf, char *val, int blen = 50) = 0;

		virtual void gpibWaitSRQ () = 0;

		virtual void initGpib () = 0;

		ConnGpib ()
		{
		}

		virtual ~ ConnGpib (void)
		{
		}
};

};
#endif		 /* !__RTS2_CONN_GPIB__ */
