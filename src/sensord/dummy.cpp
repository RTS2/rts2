/* 
 * Dummy sensor for testing.
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

class Dummy:public Sensor
{
	private:
		Rts2ValueInteger *testInt;
		Rts2ValueBool *goodWeather;
		Rts2ValueDoubleStat *statTest;
		rts2core::DoubleArray *statContent;
		Rts2ValueDoubleStat *statTest5;
		Rts2ValueDoubleMinMax *minMaxTest;
		Rts2ValueBool *hwError;
	public:
		Dummy (int argc, char **argv):Sensor (argc, argv)
		{
			createValue (testInt, "TEST_INT", "test integer value", true, RTS2_VALUE_WRITABLE | RTS2_VWHEN_RECORD_CHANGE, 0, false);
			createValue (goodWeather, "good_weather", "if dummy sensor is reporting good weather", true, RTS2_VALUE_WRITABLE);
			goodWeather->setValueBool (false);
			setWeatherState (goodWeather->getValueBool (), "weather state set from goodWeather value");
			createValue (statTest, "test_stat", "test stat value", true);

			createValue (statContent, "test_content", "test content", true);
			createValue (statTest5, "test_stat_5", "test stat value with 5 entries", true);
			createValue (minMaxTest, "test_minmax", "test minmax value", true, RTS2_VALUE_WRITABLE);
			createValue (hwError, "hw_error", "device current hardware error", false, RTS2_VALUE_WRITABLE);
		}

		virtual int setValue (Rts2Value * old_value, Rts2Value * newValue)
		{
		  	if (old_value == hwError)
			{
				maskState (DEVICE_ERROR_MASK, ((Rts2ValueBool *) newValue)->getValueBool () ? DEVICE_ERROR_HW : 0);
				return 0;
			}
			if (old_value == goodWeather)
			{
			  	setWeatherState (((Rts2ValueBool *)newValue)->getValueBool (), "weather state set from goodWeather value");
				return 0;
			}
			return Sensor::setValue (old_value, newValue);
		}

		virtual int commandAuthorized (Rts2Conn * conn)
		{
			if (conn->isCommand ("add"))
			{
				double aval;
				if (conn->paramNextDouble (&aval) || !conn->paramEnd ())
					return -2;
				statTest->addValue (aval);
				statTest->calculate ();

				statContent->addValue (aval);

				statTest5->addValue (aval, 5);
				statTest5->calculate ();

				infoAll ();
				return 0;
			}
			return Sensor::commandAuthorized (conn);
		}
};

};

using namespace rts2sensord;

int
main (int argc, char **argv)
{
	Dummy device = Dummy (argc, argv);
	return device.run ();
}
