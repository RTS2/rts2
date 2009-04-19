/* 
 * Davis weather sensor.
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

// how long after weather was bad can weather be good again; in
// seconds
#define BART_BAD_WEATHER_TIMEOUT    1200
#define BART_BAD_WINDSPEED_TIMEOUT  1200
#define BART_CONN_TIMEOUT           1200

#ifndef __RTS2_DAVIS__
#define __RTS2_DAVIS__

#include "sensord.h"
#include "davisudp.h"

namespace rts2sensor
{

/**
 * Davis Vantage (and other) meteo stations.
 * Needs modified Davis driver, which sends data over UDP.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Davis: public SensorWeather
{
	private:
		DavisUdp *weatherConn;

		Rts2ValueFloat *temperature;
		Rts2ValueFloat *humidity;
		Rts2ValueBool *rain;

		Rts2ValueFloat *avgWindSpeed;
		Rts2ValueFloat *peekWindSpeed;

		Rts2ValueFloat *rainRate;

		Rts2ValueDouble *cloud;
		Rts2ValueDouble *cloud_bad;

		Rts2ValueFloat *maxWindSpeed;
		Rts2ValueFloat *maxPeekWindSpeed;

		Rts2ValueInteger *udpPort;

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);

		virtual int idle ();

	public:
		Davis (int argc, char **argv);

		virtual int info ();

		void setTemperature (float in_temp)
		{
			temperature->setValueFloat (in_temp);
		}
		float getTemperature ()
		{
			return temperature->getValueFloat ();
		}
		void setHumidity (float in_humidity)
		{
			humidity->setValueFloat (in_humidity);
		}
		float getHumidity ()
		{
			return humidity->getValueFloat ();
		}
		void setRain (bool _rain)
		{
			rain->setValueBool (_rain);
		}
		bool getRain ()
		{
			return rain->getValueBool ();
		}

		void setAvgWindSpeed (float _avgWindSpeed)
		{
			avgWindSpeed->setValueFloat (_avgWindSpeed);
			if (!isnan (avgWindSpeed->getValueFloat ()) && _avgWindSpeed >= maxWindSpeed->getValueFloat ())
				setWeatherTimeout (BART_BAD_WINDSPEED_TIMEOUT);
		}

		void setPeekWindSpeed (float _peekWindSpeed)
		{
			peekWindSpeed->setValueFloat (_peekWindSpeed);
			if (!isnan (maxPeekWindSpeed->getValueFloat ()) && _peekWindSpeed >= maxPeekWindSpeed->getValueFloat ())
				setWeatherTimeout (BART_BAD_WINDSPEED_TIMEOUT);
		}

		void setRainRate (float _rainRate)
		{
			rainRate->setValueFloat (_rainRate);
			if (_rainRate > 0)
				setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT);	
		}

		void setCloud (double in_cloud);

		float getMaxPeekWindspeed ()
		{
			return maxPeekWindSpeed->getValueFloat ();
		}

		float getMaxWindSpeed ()
		{
			return maxWindSpeed->getValueFloat ();
		}
};

}

#endif // !__RTS2_DAVIS__
