/* 
 * Dummy sensor for testing.
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

//* rts2core::Event for timer event.
#define EVENT_TIMER_TEST     RTS2_LOCAL_EVENT + 5060

//* rts2core::Event for updating random value
#define EVENT_TIMER_RU       RTS2_LOCAL_EVENT + 5061

/**
 * Simple dummy sensor. It is an excelent example how to use mechanism inside RTS2.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Dummy:public SensorWeather
{
	public:
		Dummy (int argc, char **argv):SensorWeather (argc, argv)
		{
			createValue (testInt, "TEST_INT", "test integer value", true, RTS2_VALUE_WRITABLE | RTS2_VWHEN_RECORD_CHANGE | RTS2_VALUE_AUTOSAVE, 0);
			createValue (testDouble, "TEST_DOUBLE", "test double value", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
			createValue (testDoubleLimit, "test_limit", "test value for double; if < TEST_DOUBLE, weather will be swicthed to bad", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
			createValue (randomDouble, "random_double", "random double value", false);
			createValue (randomInterval, "random_interval", "[s] time interval between generating two random doubles", false, RTS2_VALUE_WRITABLE);
			randomInterval->setValueFloat (1.0);

			createValue (openClose, "open_close", "open/close state", false, RTS2_VALUE_WRITABLE);
			openClose->addSelVal ("CLOSED");
			openClose->addSelVal ("opening");
			openClose->addSelVal ("OPEN");
			openClose->addSelVal ("closing");

			createValue (testTime, "TEST_TIME", "test time value", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
			createValue (goodWeather, "good_weather", "if dummy sensor is reporting good weather", true, RTS2_VALUE_WRITABLE);
			createValue (wrRain, "wr_rain", "weather reason rain", true, RTS2_VALUE_WRITABLE);
			createValue (wrWind, "wr_wind", "weather reason wind", true, RTS2_VALUE_WRITABLE);
			createValue (wrHumidity, "wr_humidity", "weather reason humidity", true, RTS2_VALUE_WRITABLE);
			createValue (wrCloud, "wr_cloud", "weather reason cloud", true, RTS2_VALUE_WRITABLE);
			createValue (stopMove, "stop_move", "if dummy sensor should stop any movement", false, RTS2_VALUE_WRITABLE);
			createValue (stopMoveErr, "stop_move_err", "error state of stop_move", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_DEBUG);
			stopMoveErr->addSelVal ("good");
			stopMoveErr->addSelVal ("warning");
			stopMoveErr->addSelVal ("error");

			createValue (testOnOff, "test_on_off", "test true/false displayed as on/off", false, RTS2_VALUE_WRITABLE | RTS2_VWHEN_TRIGGERED | RTS2_DT_ONOFF);

			goodWeather->setValueBool (false);
			stopMove->setValueBool (false);
			setWeatherState (goodWeather->getValueBool (), "weather state set from goodWeather value");
			createValue (statTest, "test_stat", "test stat value", true);

			createValue (statContent1, "test_content1", "test content 1", true, RTS2_VWHEN_TRIGGERED | RTS2_WR_GROUP_NUMBER(0) | RTS2_VALUE_WRITABLE);
			createValue (statContent2, "test_content2", "test content 2", true, RTS2_VWHEN_TRIGGERED | RTS2_WR_GROUP_NUMBER(0));
			createValue (statContent3, "test_content3", "test content 3", true, RTS2_VALUE_WRITABLE);
			createValue (boolArray, "bools", "tests of boolean array", true, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF | RTS2_VALUE_DEBUG);
			boolArray->addValue (false);
			boolArray->addValue (true);
			boolArray->addValue (true);

			createValue (timeArray, "times", "tests of time array", true, RTS2_VALUE_WRITABLE);

			createValue (statTest5, "test_stat_5", "test statiscal value", true);
			createValue (timeserieTest6, "test_timeserie_6", "test timeserie value (with trending)", true);
			createValue (minMaxTest, "test_minmax", "test minmax value", true, RTS2_VALUE_WRITABLE);
			createValue (degMinMax, "deg_minmax", "[deg] degrees test minmax value", true, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
			createValue (hwError, "hw_error", "device current hardware error", false, RTS2_VALUE_WRITABLE);

			createValue (timerEnabled, "timer_enabled", "enable timer every 5 seconds", false, RTS2_VALUE_WRITABLE);
			timerEnabled->setValueBool (true);
			createValue (timerCount, "timer_count", "timer count - increased every 5 seconds if timer_enabled is true", false, RTS2_VALUE_WRITABLE);
			timerCount->setValueInteger (0);

			createValue (generateCriticalMessages, "generate_critical", "generate critical messages with timer", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
			generateCriticalMessages->setValueBool (false);

			createValue (constValue, "const_value", "test constant read-only autosave value", false, RTS2_VALUE_AUTOSAVE);

			maskState (DEVICE_BLOCK_OPEN | DEVICE_BLOCK_CLOSE, DEVICE_BLOCK_OPEN);
		}

		/**
		 * Handles events send by RTS2 core. We use it there only to catch timer messages.
		 */
		virtual void postEvent (rts2core::Event *event);


		/** Handles logic of BOP_WILL_EXPOSE and BOP_TRIG_EXPOSE. */
		virtual void setFullBopState (rts2_status_t new_state);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * newValue)
		{
			if (old_value == openClose)
			{
				switch (newValue->getValueInteger ())
				{
					case 0:
					case 1:
						maskState (DEVICE_BLOCK_OPEN | DEVICE_BLOCK_CLOSE, DEVICE_BLOCK_OPEN);
						break;
					case 2:
					case 3:
						maskState (DEVICE_BLOCK_OPEN | DEVICE_BLOCK_CLOSE, DEVICE_BLOCK_CLOSE);
						break;
				}
				return 0;
			}
		  	if (old_value == hwError)
			{
				maskState (DEVICE_ERROR_MASK, ((rts2core::ValueBool *) newValue)->getValueBool () ? DEVICE_ERROR_HW : 0);
			}
			if (old_value == wrRain)
			{
				maskState (WR_RAIN, ((rts2core::ValueBool *) newValue)->getValueBool () ? WR_RAIN : 0);
			}
			if (old_value == wrWind)
			{
				maskState (WR_WIND, ((rts2core::ValueBool *) newValue)->getValueBool () ? WR_WIND : 0);
			}
			if (old_value == wrHumidity)
			{
				maskState (WR_HUMIDITY, ((rts2core::ValueBool *) newValue)->getValueBool () ? WR_HUMIDITY : 0);
			}
			if (old_value == wrCloud)
			{
				maskState (WR_CLOUD, ((rts2core::ValueBool *) newValue)->getValueBool () ? WR_CLOUD : 0);
			}
			if (old_value == stopMove)
			{
				setStopState (((rts2core::ValueBool *)newValue)->getValueBool (), "move state set from stop_move value");
			}
			if (old_value == stopMoveErr)
			{
				switch (newValue->getValueInteger ())
				{
					case 0:
						valueGood (stopMove);
						break;
					case 1:
						valueWarning (stopMove);
						break;
					case 2:
						valueError (stopMove);
						break;
				}
				return 0;
			}
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

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int initHardware ();
		virtual bool isGoodWeather ();
	private:
		rts2core::ValueInteger *testInt;
		rts2core::ValueDouble *testDouble;
		rts2core::ValueDouble *testDoubleLimit;
		rts2core::ValueDouble *randomDouble;
		rts2core::ValueFloat *randomInterval;
		rts2core::ValueSelection *openClose;
		rts2core::ValueTime *testTime;
		rts2core::ValueBool *goodWeather;
		rts2core::ValueBool *wrRain;
		rts2core::ValueBool *wrWind;
		rts2core::ValueBool *wrHumidity;
		rts2core::ValueBool *wrCloud;
		rts2core::ValueBool *stopMove;
		rts2core::ValueSelection *stopMoveErr;
		rts2core::ValueBool *testOnOff;
		rts2core::ValueDoubleStat *statTest;
		rts2core::DoubleArray *statContent1;
		rts2core::DoubleArray *statContent2;
		rts2core::IntegerArray *statContent3;
		rts2core::BoolArray *boolArray;
		rts2core::TimeArray *timeArray;
		rts2core::ValueDoubleStat *statTest5;
		rts2core::ValueDoubleTimeserie *timeserieTest6;
		rts2core::ValueDoubleMinMax *minMaxTest;
		rts2core::ValueDoubleMinMax *degMinMax;
		rts2core::ValueBool *hwError;

		rts2core::ValueBool *timerEnabled;
		rts2core::ValueLong *timerCount;
		
		rts2core::ValueBool *generateCriticalMessages;

		rts2core::ValueInteger *constValue;

		void sendCriticalMessage ()
		{
			logStream (MESSAGE_CRITICAL) << "critical message generated from timer with count " << timerCount->getValueInteger () << sendLog;
		}
};

}

using namespace rts2sensord;

void Dummy::postEvent (rts2core::Event *event)
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
		case EVENT_TIMER_RU:
			randomDouble->setValueDouble ((double) random () / RAND_MAX);
			sendValueAll (randomDouble);
			addTimer (randomInterval->getValueFloat (), event);
			return;
	}
	SensorWeather::postEvent (event);
}

void Dummy::setFullBopState (rts2_status_t new_state)
{
	if ((new_state & BOP_WILL_EXPOSE) && !(getDeviceBopState () & BOP_WILL_EXPOSE))
	{
		maskState (BOP_TRIG_EXPOSE, 0, "enabled exposure");
		sleep (10);
	}
	if (new_state & BOP_TEL_MOVE)
	{
		maskState (BOP_TRIG_EXPOSE, BOP_TRIG_EXPOSE, "waiting for next exposure");
	}
 	SensorWeather::setFullBopState (new_state);
}

int Dummy::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("add"))
	{
		double aval;
		double now = getNow ();
		if (conn->paramNextDouble (&aval) || !conn->paramEnd ())
			return -2;
		statTest->addValue (aval);
		statTest->calculate ();

		statContent1->addValue (aval);
		statContent2->addValue (aval / 2.0);
		statContent3->addValue ((int) aval * 2);

		statTest5->addValue (aval, 5);
		statTest5->calculate ();
		timeserieTest6->addValue (aval, now);
		timeserieTest6->calculate ();
		timeArray->addValue (now);

		constValue->setValueInteger (aval);

		infoAll ();
		return 0;
	}
	else if (conn->isCommand ("now"))
	{
		testTime->setValueDouble (getNow ());
		sendValueAll (testTime);
		return 0;
	}
	else if (conn->isCommand (COMMAND_OPEN))
	{
		openClose->setValueInteger (1);
		sendValueAll (openClose);
		return 0;
	}
	else if (conn->isCommand (COMMAND_CLOSE))
	{
		openClose->setValueInteger (3);
		sendValueAll (openClose);
		return 0;
	}
	return SensorWeather::commandAuthorized (conn);
}

int Dummy::initHardware ()
{
	// initialize timer
	addTimer (5, new rts2core::Event (EVENT_TIMER_TEST));
	addTimer (1, new rts2core::Event (EVENT_TIMER_RU));
	maskState (BOP_TRIG_EXPOSE, BOP_TRIG_EXPOSE, "block exposure, waits for sensor");
	return 0;
}

bool Dummy::isGoodWeather ()
{
	if (testDouble->getValueDouble () < testDoubleLimit->getValueDouble ())
	{
		std::ostringstream os;
		os << "test value " << testDouble->getValueDouble () << " below limit " << testDoubleLimit->getValueDouble ();
		setWeatherTimeout (60, os.str ().c_str ());
		return false;
	}
	else if (goodWeather->getValueBool () == false)
	{
		valueError (goodWeather);
		setWeatherTimeout (60, "waiting for next good weather");
		return false;
	}

	valueGood (goodWeather);
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
