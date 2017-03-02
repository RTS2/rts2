/* 
 * Infrastructure for Pierre Auger UDP connection.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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


#ifndef __RTS2_FRAMUDP__
#define __RTS2_FRAMUDP__

#include <sys/types.h>
#include <time.h>

#include "connnosend.h"

namespace rts2sensord
{

class FramWeather;

class ConnFramWeather:public rts2core::ConnNoSend
{
	private:
		FramWeather * master;

	protected:
		int weather_port;

		virtual void connectionError (int last_data_size)
		{
			// do NOT call Rts2Conn::connectionError. Weather connection must be kept even when error occurs.
			return;
		}
	public:
		ConnFramWeather (int _weather_port, int _weather_timeout, FramWeather * _master);
		virtual int init ();
		virtual int receive (rts2core::Block *block);
};

}

#endif // !__RTS2_FRAMUDP__
