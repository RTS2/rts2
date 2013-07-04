/*
 * Copyright (C) 2013 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define FOCUSER_PORT "/dev/ttyACM0"

#include "focusd.h"
#include "connection/serial.h"

namespace rts2focusd
{

/**
 * Based on documentation kindly provided by Gene Chimahusky (A.K.A Astro Gene).
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class NStep:public Focusd
{
	public:
		NStep (int argc, char **argv);
		~NStep (void);

		virtual int init ();
		virtual int initValues ();
		virtual int info ();
		virtual int setTo (double num);
  		virtual double tcOffset () {return 0.;};

	protected:
		virtual int processOption (int in_opt);
		virtual int isFocusing ();

		virtual bool isAtStartPosition ()
		{
			return false;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

	private:
		char buf[15];
		const char *device_file;
                rts2core::ConnSerial *NSConn; // communication port with microfocuser

		rts2core::ValueBool *coils;
		rts2core::ValueInteger *step_speed;

		rts2core::ValueFloat *temp_change;
		rts2core::ValueInteger *temp_steps;
		rts2core::ValueSelection *temp_mode;
		rts2core::ValueInteger *temp_backlash;
		rts2core::ValueInteger *temp_timer;
};

}

using namespace rts2focusd;

NStep::NStep (int argc, char **argv):Focusd (argc, argv)
{
	device_file = FOCUSER_PORT;
	NSConn = NULL;

	createValue (coils, "coils", "coil on after move", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (step_speed, "step_speed", "max step speed", false, RTS2_VALUE_WRITABLE);

	createValue (temp_change, "temp_change", "[C] temperature change for compensation", false, RTS2_VALUE_WRITABLE);
	createValue (temp_steps, "temp_steps", "temperature compensation move, steps per temperature change", false, RTS2_VALUE_WRITABLE);
	createValue (temp_mode, "temp_mode", "temperature compensation mode", false, RTS2_VALUE_WRITABLE);
	temp_mode->addSelVal ("off");
	temp_mode->addSelVal ("one shot");
	temp_mode->addSelVal ("auto");

	createValue (temp_backlash, "temp_backlash", "temperature compensation backlash", false, RTS2_VALUE_WRITABLE);
	createValue (temp_timer, "temp_timer", "temperature compensation timer", false, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "device file (ussualy /dev/ttyACMx");
}

NStep::~NStep ()
{
  	delete NSConn;
}

int NStep::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;
		default:
			return Focusd::processOption (in_opt);
	}
	return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int NStep::init ()
{
	int ret;

	ret = Focusd::init ();

	if (ret)
		return ret;

	NSConn = new rts2core::ConnSerial (device_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
	NSConn->setDebug (getDebug ());
	ret = NSConn->init ();
	if (ret)
		return ret;

	NSConn->flushPortIO ();

	ret = NSConn->writeRead ("#:RT", 4, buf, 4);
	if (ret != 4 || !(buf[0] == '+' || buf[0] == '-'))
	{
		logStream (MESSAGE_ERROR) << "cannot switch focuser to PC mode" << sendLog;
		return -1;
	}

	if (strncmp (buf, "-888", 4))
	{
		createTemperature ();
	}

	ret = info ();
	if (ret)
		return ret;

	setIdleInfoInterval (60);

	return 0;
}

int NStep::initValues ()
{
	focType = std::string ("GCUSB-nStep");
	return Focusd::initValues ();
}

int NStep::info ()
{
	int ret;
	if (temperature)
	{
		ret = NSConn->writeRead ("#:RT", 4, buf, 4);
		if (ret != 4)
		{
			logStream (MESSAGE_ERROR) << "cannot read temperature" << sendLog;
			return -1;
		}
		buf[4] = '\0';
		temperature->setValueFloat (atof (buf) / 10);
	}

	ret = NSConn->writeRead (":RC", 3, buf, 1);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot read coil state" << sendLog;
		return -1;
	}
	coils->setValueBool (buf[0] == '0');

	ret = NSConn->writeRead (":RS", 3, buf, 3);
	if (ret != 3)
	{
		logStream (MESSAGE_ERROR) << "cannot read current position" << sendLog;
		return -1;
	}
	buf[3] = '\0';
	step_speed->setValueInteger (atoi (buf));

	ret = NSConn->writeRead ("#:RP", 4, buf, 7);
	if (ret != 7)
	{
		logStream (MESSAGE_ERROR) << "cannot read current position" << sendLog;
		return -1;
	}
	buf[7] = '\0';
	position->setValueDouble (atoi (buf));

	ret = NSConn->writeRead (":RA", 3, buf, 4);
	if (ret != 4)
	{
		logStream (MESSAGE_ERROR) << "cannot read temperature change for compensation" << sendLog;
		return -1;
	}
	buf[4] = '\0';
	temp_change->setValueFloat (atof (buf) / 10.0);

	ret = NSConn->writeRead (":RB", 3, buf, 3);
	if (ret != 3)
	{
		logStream (MESSAGE_ERROR) << "cannot read temperature step for compensation" << sendLog;
		return -1;
	}
	buf[3] = '\0';
	temp_steps->setValueInteger (atoi (buf));

	ret = NSConn->writeRead (":RG", 3, buf, 1);
	if (ret != 1)
	{
		logStream (MESSAGE_ERROR) << "cannot read temperature compensation mode" << sendLog;
		return -1;
	}
	buf[1] = '\0';
	temp_mode->setValueInteger (atoi (buf));

	ret = NSConn->writeRead (":RE", 3, buf, 3);
	if (ret != 3)
	{
		logStream (MESSAGE_ERROR) << "cannot read temperature compensation backlash" << sendLog;
		return -1;
	}
	buf[3] = '\0';
	temp_backlash->setValueInteger (atoi (buf));

	ret = NSConn->writeRead (":RH", 3, buf, 2);
	if (ret != 2)
	{
		logStream (MESSAGE_ERROR) << "cannot read temperature compensation timer" << sendLog;
		return -1;
	}
	buf[2] = '\0';
	temp_timer->setValueInteger (atoi (buf));

	return Focusd::info ();
}

int NStep::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == coils)
	{
		sprintf (buf, ":CC%c#", ((rts2core::ValueBool *) newValue)->getValueBool () ? '0' : '1');
		return NSConn->writePort (buf, 5);
	}
	if (oldValue == step_speed)
	{
		sprintf (buf, ":CS%03d#", newValue->getValueInteger ());
		return NSConn->writePort (buf, 7);
	}
	if (oldValue == temp_change)
	{
		sprintf (buf, ":TT%+04d#", int (newValue->getValueFloat () * 10.0));
		return NSConn->writePort (buf, 8);
	}
	if (oldValue == temp_steps)
	{
		sprintf (buf, ":TS%+03d#", newValue->getValueInteger ());
		return NSConn->writePort (buf, 7);
	}
	if (oldValue == temp_mode)
	{
		sprintf (buf, ":TA%1d", newValue->getValueInteger ());
		return NSConn->writePort (buf, 4);
	}
	if (oldValue == temp_backlash)
	{
		sprintf (buf, ":TB%03d#", newValue->getValueInteger ());
		return NSConn->writePort (buf, 7);
	}
	if (oldValue == temp_timer)
	{
		sprintf (buf, ":TB%03d#", newValue->getValueInteger ());
		return NSConn->writePort (buf, 7);
	}
	return Focusd::setValue (oldValue, newValue);
}

int NStep::setTo (double num)
{
	int ret = info ();
	if (ret)
		return -1;

	double diff = num - position->getValueDouble ();
	// ignore sub-step requests
	if (fabs (diff) < 1)
		return 0;
	
	if (fabs (diff) > 999)
	{
		if (diff < 0)
			diff = -999;
		else
			diff = 999;
	}

	// compare as well strings we will send..
	snprintf (buf, 9, ":F%c%d%03d#", (diff > 0) ? '1' : '0', 0, (int) (fabs (diff)));
	return NSConn->writePort (buf, 8);
}

int NStep::isFocusing ()
{
	int ret = info ();
	if (ret)
		return -1;
	sendValueAll (position);

	ret = NSConn->writeRead ("S", 1, buf, 1);
	if (ret != 1)
		return -1;
	if (buf[0] == '1')
		return USEC_SEC;
	
	// check if it reached position..
	ret = info ();
	if (ret)
		return -1;
	
	if (getTarget () == getPosition ())
		return -2;

	return setTo (getTarget ());
}

int main (int argc, char **argv)
{
	NStep device (argc, argv);
	return device.run ();
}
