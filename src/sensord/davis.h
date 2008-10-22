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
		double cloud_bad;

		Rts2ValueFloat *temperature;
		Rts2ValueFloat *humidity;
		Rts2ValueBool *rain;
		Rts2ValueFloat *windspeed;
		Rts2ValueDouble *cloud;

		Rts2ValueInteger *udpPort;

	protected:
		virtual int processOption (int _opt);
		virtual int init ();

	public:
		Davis (int argc, char **argv);

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
		virtual void setRainWeather (int in_rain)
		{
			setRain (in_rain);
		}
		bool getRain ()
		{
			return rain->getValueBool ();
		}
		void setWindSpeed (float in_windpseed)
		{
			windspeed->setValueFloat (in_windpseed);
		}
		float getWindSpeed ()
		{
			return windspeed->getValueFloat ();
		}
		void setCloud (double in_cloud)
		{
			cloud->setValueDouble (in_cloud);
		}
		double getCloud ()
		{
			return cloud->getValueDouble ();
		}

		int getMaxPeekWindspeed ()
		{
			return 50;
		}

		int getMaxWindSpeed ()
		{
			return 50;
		}
};

}

#endif // !__RTS2_DAVIS__
