/* 
 * Fram weather sensor.
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

#ifndef __RTS2_FRAM_WEATHER__
#define __RTS2_FRAM_WEATHER__

#include "sensord.h"
#include "framudp.h"

#include <libnova/libnova.h>

namespace rts2sensord
{

/**
 * Class used on FRAM site for UDP based weather transport.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class FramWeather: public SensorWeather
{
	public:
		FramWeather (int args, char **argv);

		virtual int info ();

		/**
		 * Set weather conditions, taken from weather connection.
		 *
		 * @param _windSpeed  Current windspeed.
		 * @param _rain       Rain bit (true if it is raining)
		 * @param _status     Weather connection status (watch or something else)
		 * @param _date       Date and time of last weather measurement.
		 */
		void setWeather (float _windSpeed, bool _rain, const char *_status, struct ln_date *_date);

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

		virtual bool isGoodWeather ();

	private:
		// measured values
		rts2core::ValueFloat *windSpeed;
		rts2core::ValueBool *rain;
		rts2core::ValueSelection *watch;

		rts2core::ValueFloat *maxWindSpeed;
		rts2core::ValueInteger *connUpdateSep;

		rts2core::ValueInteger *timeoutConn;
		rts2core::ValueInteger *timeoutRain;
		rts2core::ValueInteger *timeoutWindspeed;

		ConnFramWeather *weatherConn;
};

}

#endif // !__RTS2_FRAM_WEATHER__
