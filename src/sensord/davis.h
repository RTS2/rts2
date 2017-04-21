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

namespace rts2sensord
{

/**
 * Davis Vantage (and other) meteo stations.
 * Needs modified Davis driver, which sends data over UDP.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Davis: public SensorWeather
{
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
			if (!std::isnan (maxHumidity->getValueFloat ()) && humidity->getValueFloat () > maxHumidity->getValueFloat ())
			{
				setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT, "raining");
				valueError (humidity);
			}
			else
			{
				valueGood (humidity);
			}
		}
		float getHumidity ()
		{
			return humidity->getValueFloat ();
		}
		void setRain (bool _rain)
		{
			rain->setValueBool (_rain);
			if (_rain)
			{
				setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT, "raining");
				valueError (rain);
			}
			else
			{
				valueGood (rain);
			}
		}
		bool getRain ()
		{
			return rain->getValueBool ();
		}

		void setAvgWindSpeed (float _avgWindSpeed)
		{
			avgWindSpeed->setValueFloat (_avgWindSpeed);
			avgWindSpeedStat->addValue (_avgWindSpeed, 20);
			avgWindSpeedStat->calculate ();
			if (!std::isnan (avgWindSpeedStat->getValueFloat ()) && avgWindSpeedStat->getValueFloat () >= maxWindSpeed->getValueFloat ())
			{
				setWeatherTimeout (BART_BAD_WINDSPEED_TIMEOUT, "high average wind");
				valueError (avgWindSpeed);
				valueError (avgWindSpeedStat);
			}
			else if (!std::isnan (avgWindSpeed->getValueFloat ()) && _avgWindSpeed >= maxWindSpeed->getValueFloat ())
			{
				valueError (avgWindSpeed);
				valueWarning (avgWindSpeedStat);
			}
			else
			{
				valueGood (avgWindSpeed);
				valueGood (avgWindSpeedStat);
			}
		}

		void setPeekWindSpeed (float _peekWindSpeed)
		{
			peekWindSpeed->setValueFloat (_peekWindSpeed);
			if (!std::isnan (maxPeekWindSpeed->getValueFloat ()) && _peekWindSpeed >= maxPeekWindSpeed->getValueFloat ())
			{
				setWeatherTimeout (BART_BAD_WINDSPEED_TIMEOUT, "high peek wind");
				valueError (peekWindSpeed);
			}
			else
			{
				valueGood (peekWindSpeed);
			}
		}

		void setWindDir (float _windDir) { windDir->setValueFloat (_windDir); }
		void setBaroCurr (float _baroCurr) { pressure->setValueFloat (_baroCurr); }

		void setRainRate (float _rainRate)
		{
			rainRate->setValueFloat (_rainRate);
			if (_rainRate > 0)
			{
				setWeatherTimeout (BART_BAD_WEATHER_TIMEOUT, "raining (rainrate)");	
				valueError (rainRate);
			}
			else
			{
				valueGood (rainRate);
			}
		}

		void setWetness (double _wetness);

		void setCloud (double _cloud, double _top, double _bottom);

		float getMaxPeekWindspeed ()
		{
			return maxPeekWindSpeed->getValueFloat ();
		}

		float getMaxWindSpeed ()
		{
			return maxWindSpeed->getValueFloat ();
		}
	protected:
		virtual int processOption (int _opt);
		virtual int init ();

		virtual int idle ();

		virtual int info ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
	private:
		DavisUdp *weatherConn;

		rts2core::ValueInteger *connTimeout;

		rts2core::ValueFloat *temperature;
		rts2core::ValueFloat *humidity;
		rts2core::ValueBool *rain;
		rts2core::ValueFloat *pressure;

		rts2core::ValueFloat *avgWindSpeed;
		rts2core::ValueDoubleStat *avgWindSpeedStat;
		rts2core::ValueFloat *peekWindSpeed;
		rts2core::ValueFloat *windDir;

		rts2core::ValueFloat *rainRate;

		rts2core::ValueDouble *wetness;

		rts2core::ValueDouble *cloud;
		rts2core::ValueDouble *cloudTop;
		rts2core::ValueDouble *cloudBottom;
		rts2core::ValueDouble *cloud_bad;

		rts2core::ValueFloat *maxWindSpeed;
		rts2core::ValueFloat *maxPeekWindSpeed;

		rts2core::ValueFloat *maxHumidity;

		rts2core::ValueInteger *udpPort;
};

}

#endif // !__RTS2_DAVIS__
