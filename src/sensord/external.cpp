/*
 * Pseudo weather sensor that just exposes meteo variables, to be set from external scripts.
 * Copyright (C) 2018 Sergey Karpov <karpov.sv@gmail.com>
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
#include "value.h"

namespace rts2sensord
{

/**
 * Simple weather sensor meant to be driven by external scripts to update actual values.
 *
 * @author Sergey Karpov <karpov.sv@gmail.com>
 */
class External:public SensorWeather
{
	public:
		External (int argc, char **argv);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * newValue);

	protected:
		virtual int processOption (int opt);
		virtual bool isGoodWeather ();

	private:
		rts2core::ValueTime *updateTime;
		rts2core::ValueBool *goodWeather;
		rts2core::ValueDouble *weatherTimeout;
};

External::External (int argc, char **argv):SensorWeather (argc, argv, 0)
{
	createValue (updateTime, "update_time", "time of last update of device status");

	createValue (goodWeather, "good_weather", "weather quality as set by external script", true, RTS2_VALUE_WRITABLE);
	goodWeather->setValueBool (true);

	createValue (weatherTimeout, "weather_timeout", "bad weather timeout", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	weatherTimeout->setValueDouble (300);

	addOption ('t', "timeout", 1, "Bad weather timeout, seconds");

	addOption ('I', "int", 1, "Create integer variable");
	addOption ('B', "bool", 1, "Create boolean variable");
	addOption ('D', "double", 1, "Create double variable");
	addOption ('S', "string", 1, "Create string variable");

	setWeatherState (true, "switching to GOOD_WEATHER on startup");
}

int External::processOption (int opt)
{
	char *name = optarg;
	char *comment = NULL;

	// Parse comma-separated variable name and comment in optarg, if any
	if (opt == 'I' || opt == 'B' || opt == 'D' || opt == 'S')
	{
		char *p = strchr (optarg, ',');

		if (p != NULL)
		{
			comment = p + 1;
			*p = '\0';
		}
	}

	switch (opt)
	{
		case 't':
			weatherTimeout->setValueCharArr (optarg);
			break;

		case 'I':
		{
			rts2core::ValueInteger *value = NULL;
			createValue (value, name,  comment ? comment : "integer value", true, RTS2_VALUE_WRITABLE);

			break;
		}
		case 'B':
		{
			rts2core::ValueBool *value = NULL;
			createValue (value, name,  comment ? comment : "boolean value", true, RTS2_VALUE_WRITABLE);

			break;
		}
		case 'D':
		{
			rts2core::ValueDouble *value = NULL;
			createValue (value, name,  comment ? comment : "double value", true, RTS2_VALUE_WRITABLE);

			break;
		}
		case 'S':
		{
			rts2core::ValueString *value = NULL;
			createValue (value, name,  comment ? comment : "string value", true, RTS2_VALUE_WRITABLE);

			break;
		}

		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}


int External::setValue (rts2core::Value * old_value, rts2core::Value * newValue)
{
	updateTime->setValueInteger (getNow ());
	sendValueAll (updateTime);

	return SensorWeather::setValue (old_value, newValue);
}

bool External::isGoodWeather ()
{
	if (!goodWeather->getValueBool ())
	{
		setWeatherTimeout (weatherTimeout->getValueDouble (), "Weather is bad");
		return false;
	}

	return SensorWeather::isGoodWeather ();
}

}

using namespace rts2sensord;

int main (int argc, char **argv)
{
	External device (argc, argv);
	return device.run ();
}
