/* 
 * Source demonstrating how to use multiple sensors.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "multidev.h"
#include "sensord.h"

using namespace rts2sensord;

#define EVENT_TIMER_TEST     RTS2_LOCAL_EVENT + 5060

class MultiSensor: public SensorWeather
{
	public:
		MultiSensor (int argc, char **argv, const char *sn);

		virtual void postEvent (rts2core::Event *event);
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * newValue);

	protected:
		virtual int initHardware ();
	
	private:
		void sendCriticalMessage ()
		{
			logStream (MESSAGE_CRITICAL) << "critical message generated from timer with count " << timerCount->getValueInteger () << sendLog;
		}

		rts2core::ValueDouble *testDouble;
		rts2core::ValueString *testString;

		rts2core::ValueBool *timerEnabled;
		rts2core::ValueLong *timerCount;

		rts2core::ValueBool *generateCriticalMessages;
};

MultiSensor::MultiSensor (int argc, char **argv, const char *sn):SensorWeather (argc, argv, 120, sn)
{
	createValue (testDouble, "test_double", "test double variable", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (testString, "test_string", "test string variable", true);

	createValue (timerEnabled, "timer_enabled", "enable timer every 5 seconds", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	timerEnabled->setValueBool (true);
	createValue (timerCount, "timer_count", "timer count - increased every 5 seconds if timer_enabled is true", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	timerCount->setValueInteger (0);

	createValue (generateCriticalMessages, "generate_critical", "generate critical messages with timer", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	generateCriticalMessages->setValueBool (false);

	char buf[10];
	random_salt (buf, 9);
	buf[9] = '\0';
	testString->setValueCharArr (buf);
}

void MultiSensor::postEvent (rts2core::Event *event)
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
				// reschedule us and return - ascending up to SensorWeather::postEvent will delete event object, we would like to avoid this
				addTimer (5, event);
				if (timerCount->getValueLong () % 12 == 0)
					logStream (MESSAGE_INFO) << "60 seconds timer: " << timerCount->getValueLong () << sendLog;
				if (generateCriticalMessages->getValueBool ())
					sendCriticalMessage ();
				return;
			}
			break;
	}
}

int MultiSensor::setValue (rts2core::Value * old_value, rts2core::Value * newValue)
{
	if (old_value == timerEnabled)
	{
		if (((rts2core::ValueBool *) newValue)->getValueBool () == true)
		{
			deleteTimers (EVENT_TIMER_TEST);
			addTimer (5, new rts2core::Event (EVENT_TIMER_TEST));
		}
		return 0;
	}
	if (old_value == generateCriticalMessages)
	{
		if (((rts2core::ValueBool *) newValue)->getValueBool () == true)
		{
			sendCriticalMessage ();
		}
		return 0;
	}
	return SensorWeather::setValue (old_value, newValue);
}

int MultiSensor::initHardware ()
{
	// initialize timer
	addTimer (5, new rts2core::Event (EVENT_TIMER_TEST));
	return 0;
}

int main (int argc, char ** argv)
{
	rts2core::MultiBase mb (argc, argv, "SMD");
	mb.addDevice (new MultiSensor (argc, argv, "MS1"));
	mb.addDevice (new MultiSensor (argc, argv, "MS2"));
	return mb.run ();
}
