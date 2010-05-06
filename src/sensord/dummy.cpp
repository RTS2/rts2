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

//* Event number for timer event.
#define EVENT_TIMER_TEST     RTS2_LOCAL_EVENT + 5060

/**
 * Simple dummy sensor. It is an excelent example how to use mechanism inside RTS2.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Dummy:public Sensor
{
	private:
		Rts2ValueInteger *testInt;
		Rts2ValueBool *goodWeather;
		Rts2ValueBool *testOnOff;
		Rts2ValueDoubleStat *statTest;
		rts2core::DoubleArray *statContent1;
		rts2core::DoubleArray *statContent2;
		Rts2ValueDoubleStat *statTest5;
		Rts2ValueDoubleMinMax *minMaxTest;
		Rts2ValueBool *hwError;

		Rts2ValueBool *timerEnabled;
		Rts2ValueLong *timerCount;
	protected:
		virtual int init ()
		{
			int ret = Sensor::init ();
			if (ret)
				return ret;
			// initialize timer
			addTimer (5, new Rts2Event (EVENT_TIMER_TEST));
			return 0;
		}
	public:
		Dummy (int argc, char **argv):Sensor (argc, argv)
		{
			createValue (testInt, "TEST_INT", "test integer value", true, RTS2_VALUE_WRITABLE | RTS2_VWHEN_RECORD_CHANGE, 0, false);
			createValue (goodWeather, "good_weather", "if dummy sensor is reporting good weather", true, RTS2_VALUE_WRITABLE);
			createValue (testOnOff, "test_on_off", "test true/false displayed as on/off", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
			goodWeather->setValueBool (false);
			setWeatherState (goodWeather->getValueBool (), "weather state set from goodWeather value");
			createValue (statTest, "test_stat", "test stat value", true);

			createValue (statContent1, "test_content1", "test content 1", true, RTS2_WR_GROUP_NUMBER(0));
			createValue (statContent2, "test_content2", "test content 2", true, RTS2_WR_GROUP_NUMBER(0));

			createValue (statTest5, "test_stat_5", "test stat value with 5 entries", true);
			createValue (minMaxTest, "test_minmax", "test minmax value", true, RTS2_VALUE_WRITABLE);
			createValue (hwError, "hw_error", "device current hardware error", false, RTS2_VALUE_WRITABLE);

			createValue (timerEnabled, "timer_enabled", "enable timer every 5 seconds", false, RTS2_VALUE_WRITABLE);
			timerEnabled->setValueBool (true);
			createValue (timerCount, "timer_count", "timer count - increased every 5 seconds if timer_enabled is true", false, RTS2_VALUE_WRITABLE);
			timerCount->setValueInteger (0);
		}

		/**
		 * Handles event send by RTS2 core. We use it there only to catch timer messages.
		 */
		virtual void postEvent (Rts2Event *event)
		{
			switch (event->getType ())
			{
				case EVENT_TIMER_TEST:
					if (timerEnabled->getValueBool () == true)
					{
						// increas value
						timerCount->inc ();
						// send out value to all connections
						sendValueAll (timerCount);
						// reschedule us and return - ascending up to Sensor::postEvent will delete event object, we would like to avoid this
						addTimer (5, event);
						if (timerCount->getValueLong () % 12 == 0)
							logStream (MESSAGE_INFO) << "60 seconds timer: " << timerCount->getValueLong () << sendLog;
						return;
					}
					break;
			}
			Sensor::postEvent (event);
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
			if (old_value == timerEnabled)
			{
				if (((Rts2ValueBool *) newValue)->getValueBool () == true)
				{
					deleteTimers (EVENT_TIMER_TEST);
					addTimer (5, new Rts2Event (EVENT_TIMER_TEST));
				}
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

				statContent1->addValue (aval);
				statContent2->addValue (aval / 2.0);

				statTest5->addValue (aval, 5);
				statTest5->calculate ();

				infoAll ();
				return 0;
			}
			return Sensor::commandAuthorized (conn);
		}
};

}

using namespace rts2sensord;

int
main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
