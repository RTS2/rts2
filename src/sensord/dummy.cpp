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

class Rts2DevSensorDummy:public Rts2DevSensor
{
	private:
		Rts2ValueInteger *testInt;
		Rts2ValueBool *goodWeather;
		Rts2ValueDoubleStat *statTest;
		Rts2ValueDoubleStat *statTest5;
		Rts2ValueDoubleMinMax *minMaxTest;
	public:
		Rts2DevSensorDummy (int argc, char **argv):Rts2DevSensor (argc, argv)
		{
			createValue (testInt, "TEST_INT", "test integer value", true, RTS2_VWHEN_RECORD_CHANGE, 0, false);
			createValue (goodWeather, "good_weather", "if dummy sensor is reporting good weather", true);
			goodWeather->setValueBool (false);
			setWeatherState (goodWeather->getValueBool ());
			createValue (statTest, "test_stat", "test stat value", true);
			createValue (statTest5, "test_stat_5", "test stat value with 5 entries", true);
			createValue (minMaxTest, "test_minmax", "test minmax value", true);
		}

		virtual int setValue (Rts2Value * old_value, Rts2Value * newValue)
		{
			if (old_value == minMaxTest
				|| old_value == testInt)
				return 0;
			if (old_value == goodWeather)
			{
			  	setWeatherState (((Rts2ValueBool *)newValue)->getValueBool ());
				return 0;
			}
			return Rts2DevSensor::setValue (old_value, newValue);
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

				statTest->addValue (aval);
				statTest->calculate ();

				infoAll ();
				return 0;
			}
			return Rts2DevSensor::commandAuthorized (conn);
		}
};

int
main (int argc, char **argv)
{
	Rts2DevSensorDummy device = Rts2DevSensorDummy (argc, argv);
	return device.run ();
}
