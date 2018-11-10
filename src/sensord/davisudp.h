/* 
 * Infrastructure for Davis UDP connection.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DAVISUDP__
#define __RTS2_DAVISUDP__

#include "connnosend.h"

namespace rts2sensord
{

class Davis;

class WeatherVal
{
	private:
		const char *name;
	public:
		float value;

		WeatherVal (const char *in_name, float in_value)
		{
			name = in_name;
			value = in_value;
		}

		bool isValue (const char *in_name)
		{
			return !strcmp (name, in_name);
		}
};

class WeatherBuf
{
	private:
		std::vector < WeatherVal > values;
	public:
		WeatherBuf ();
		virtual ~ WeatherBuf (void);

		int parse (char *buf);
		void getValue (const char *name, float &val, int &status);
};

class DavisUdp:public rts2core::ConnNoSend
{
	public:
		DavisUdp (int _weather_port, int _weather_timeout, int _conn_timeout, int _bad_weather_timeout, Davis * _master);

		virtual int init ();

		virtual int receive (rts2core::Block *block);

		void setConnTimeout (int _ct) { conn_timeout = _ct; }
	protected:
		void setWeatherTimeout (time_t wait_time, const char *msg);

		virtual void connectionError (__attribute__ ((unused)) int last_data_size)
		{
			// do NOT call Rts2Conn::connectionError. Weather connection must be kept even when error occurs.
			return;
		}
	private:
		Davis *master;
		int weather_port;
		int weather_timeout;
		int conn_timeout;
		int bad_weather_timeout;

		int rain;
		float peekwindspeed;
		float avgWindSpeed;
		time_t lastWeatherStatus;
		time_t lastBadWeather;
};

}

#endif // !__RTS2_DAVISUDP__
