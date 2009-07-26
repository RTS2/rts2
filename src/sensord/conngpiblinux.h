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

#ifndef __RTS2_CONN_GPIB_LINUX__
#define __RTS2_CONN_GPIB_LINUX__

#include "conngpib.h"
#include "../utils/error.h"

#include <gpib/ib.h>
#include <sstream>

namespace rts2sensord
{

class GpibLinuxError: public rts2core::Error
{
	public:
		GpibLinuxError (const char *_msg, const char *_buf, int _ret): rts2core::Error ()
		{
			std::ostringstream _os;
			_os << _msg << " buffer " << _buf << " GPIB error " << gpib_error_string (_ret) << " " << _ret;
			setMsg (_os.str ());
		}
};

class ConnGpibLinux:public ConnGpib
{
	private:
		int minor;
		int pad;

		int gpib_dev;
	public:
		virtual void gpibWrite (const char *_buf);
		virtual void gpibRead (void *_buf, int &blen);
		virtual void gpibWriteRead (const char *_buf, char *val, int blen = 50);

		virtual void gpibWaitSRQ ();

		virtual void initGpib ();

		ConnGpibLinux (int _minor, int _pad);
		virtual ~ ConnGpibLinux (void);
};

};
#endif		 /* !__RTS2_CONN_GPIB_LINUX__ */
