/*
 * Pseudo weather sensor that just exposes meteo variables, to be set from external scripts.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "sensord.h"

namespace rts2sensord
{

/**
 * Simple weather sensor meant to be driven by external scripts to update actual values.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class External:public SensorWeather
{
	public:
		External (int argc, char **argv):SensorWeather (argc, argv)
		{
			createValue (updateTime, "update_time", "time of last update of weather status");

			createValue (goodWeather, "good_weather", "weather quality as set by external script", true, RTS2_VALUE_WRITABLE);
			createValue (wrRain, "rain", "rain condition as set by external script", true, RTS2_VALUE_WRITABLE);
			createValue (wrTemperature, "temperature", "temperature as set by external script", true, RTS2_VALUE_WRITABLE);
			createValue (wrWind, "windspeed", "wind speed as set by external script", true, RTS2_VALUE_WRITABLE);
			createValue (wrHumidity, "humidity", "humidity as set by external script", true, RTS2_VALUE_WRITABLE);
			createValue (wrPressure, "pressure", "pressure as set by external script", true, RTS2_VALUE_WRITABLE);

			goodWeather->setValueBool (false);
			wrRain->setValueBool (false);
			wrTemperature->setValueDouble (0.0);
			wrWind->setValueDouble (0.0);
			wrHumidity->setValueDouble (0.0);
			wrPressure->setValueDouble (0.0);

			setWeatherState (goodWeather->getValueBool (), "weather state set from goodWeather value");

			maskState (DEVICE_BLOCK_OPEN | DEVICE_BLOCK_CLOSE, DEVICE_BLOCK_OPEN);
		}

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * newValue)
		{
			updateTime->setValueInteger (getNow ());
			sendValueAll (updateTime);

			if (old_value == goodWeather)
			{
				setWeatherState (((rts2core::ValueBool *)newValue)->getValueBool (), "weather state set from goodWeather value");
			}

			return SensorWeather::setValue (old_value, newValue);
		}

	protected:
		virtual bool isGoodWeather ()
			{
				return goodWeather->getValueBool ();
			}
	private:
		rts2core::ValueTime *updateTime;
		rts2core::ValueBool *goodWeather;
		rts2core::ValueBool *wrRain;
		rts2core::ValueDouble *wrTemperature;
		rts2core::ValueDouble *wrWind;
		rts2core::ValueDouble *wrHumidity;
		rts2core::ValueDouble *wrPressure;
};

}

using namespace rts2sensord;

int main (int argc, char **argv)
{
	External device (argc, argv);
	return device.run ();
}
