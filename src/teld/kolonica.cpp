/* 
 * Driver for Chermelin Telescope Mounting at the Kolonica Observatory, Slovak Republic
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "fork.h"

#include "connection/serial.h"

namespace rts2teld
{

/**
 * Driver for Kolonica telescope.
 *
 * @author Petr Kubanek <peter@kubanek.net>
 * @author Marek Chrastina
 */
class Kolonica:public Fork
{
	public:
		Kolonica (int in_argc, char **in_argv);
		virtual ~Kolonica (void);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();


		virtual int startResync ();
		virtual int isMoving ();
		virtual int stopMove();
		virtual int startPark();
		virtual int endPark();
		virtual int updateLimits();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		const char *telDev;
		rts2core::ConnSerial *telConn;

		/**
		 * Convert long integer to binary. Most important byte is at
		 * first place, e.g. it's low endian.
		 * 
		 * @param num Number which will be converted. 
		 * @param buf Buffer where converted string will be printed.
		 * @param len Length of expected binarry buffer.
		 */
		void intToBin (unsigned long num, char *buf, int len);

		/**
		 * Wait for given number of (msec?).
		 *
		 * @param num  Number of units to wait.
		 *
		 * @return -1 on failure, 0 on success.
		 */
		int telWait (long num);

		/**
		 * Change commanding axis. Issue G command to change
		 * commanding axis.
		 *
		 * @param axId -1 for global (all axis) commanding, 1-16 are then
		 * valid axis numbers.
		 *
		 * @return -1 on failure, 0 on success.
		 */
		int changeAxis (int axId);

		/**
		 * Set axis target position to given value.
		 *
		 * @param axId   Id of the axis, -1 for global.
		 * @param value  Value to which axis will be set.
		 * 
		 * @return -1 on error, 0 on sucess.
		 */
		int setAltAxis (long value);

		/**
		 * Set axis target position to given value.
		 *
		 * @param axId   Id of the axis, -1 for global.
		 * @param value  Value to which axis will be set.
		 * 
		 * @return -1 on error, 0 on sucess.
		 */
		int setAzAxis (long value);

		/**
		 * Switch motor on/off.
		 *
		 * @param axId Axis ID.
		 * @param on True if motor shall be set to on.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int setMotor (int axId, bool on);

		rts2core::ValueLong *axAlt;
		rts2core::ValueLong *axAz;

		rts2core::ValueBool *motorAlt;
		rts2core::ValueBool *motorAz;
};

};

using namespace rts2teld;

void Kolonica::intToBin (unsigned long num, char *buf, int len)
{
	int i;
      
        for(i = len - 1 ; i >= 0 ; i--)  {
		buf[i] = num & 0xff;
		num >>= 8;
	}
}

int Kolonica::telWait (long num)
{
	char buf[5];
	buf[0] = 'W';
	intToBin (num, buf + 1, 3);
	buf[4] = '\r';
	return telConn->writePort (buf, 3);
}

int Kolonica::changeAxis (int axId)
{
	char buf[3];
	buf[0] = 'X';
	if (axId < 0)
	{
		buf[1] = 'G';
	}
	else
	{
		intToBin ((unsigned long) axId, buf + 1, 1);
	}
	buf[2] = '\r';
	if (telConn->writePort (buf, 3))
		return -1;
	return telWait (150);	
}

int Kolonica::setAltAxis (long value)
{
	char buf[5];

	if (changeAxis (1))
		return -1;

	long diff = value - axAlt->getValueLong ();
	if (diff < 0)
	{
		buf[0] = 'B';
		diff *= -1;
	}
	else
	{
		buf[0] = 'F';
	}
	intToBin ((unsigned long) diff, buf + 1, 3);
	buf[4] = '\r';
	return telConn->writePort (buf, 5);
}

int Kolonica::setAzAxis (long value)
{
 	char buf[5];

	if (changeAxis (2))
		return -1;

	long diff = value - axAz->getValueLong ();
	if (diff < 0)
	{
		buf[0] = 'B';
		diff *= -1;
	}
	else
	{
		buf[0] = 'F';
	}
	intToBin ((unsigned long) diff, buf + 1, 3);
	buf[4] = '\r';
	return telConn->writePort (buf, 5);
}

int Kolonica::setMotor (int axId, bool on)
{
	char buf[3];

	if (changeAxis (axId))
		return -1;

	buf[0] = on ? 'C' : 'T';
        intToBin (16, buf + 1, 1);
	buf[2] = '\r';
	return telConn->writePort (buf, 3);
}

int Kolonica::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			telDev = optarg;
			break;
		default:
			return Fork::processOption (in_opt);
	}
	return 0;
}

int Kolonica::init ()
{
	int ret;
	ret = Fork::init ();
	if (ret)
		return ret;
	
	telConn = new rts2core::ConnSerial (telDev, this, rts2core::BS4800, rts2core::C8, rts2core::NONE, 50);
	ret = telConn->init ();
	if (ret)
		return ret;

	telConn->setDebug ();
	telConn->flushPortIO ();
	
	// check that telescope is working
	return info ();
}

int Kolonica::startResync ()
{
	return -1;
}

int Kolonica::isMoving ()
{
	return -1;
}

int Kolonica::stopMove ()
{
	return -1;
}

int Kolonica::startPark()
{
	return -1;
}

int Kolonica::endPark()
{
	return -1;
}

int Kolonica::updateLimits()
{
	return -1;
}

int Kolonica::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == axAlt)
	{
		return setAltAxis (new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == axAz)
	{
		return setAzAxis (new_value->getValueLong ()) == 0 ? 0 : -2;
	}
	if (old_value == motorAlt)
	{
		return setMotor (1, ((rts2core::ValueBool *) new_value)->getValueBool ()) == 0 ? 0 : -2;
	}
	if (old_value == motorAz)
	{
		return setMotor (2, ((rts2core::ValueBool *) new_value)->getValueBool ()) == 0 ? 0 : -2;
	}
	return Fork::setValue (old_value, new_value);
}

Kolonica::Kolonica (int in_argc, char **in_argv):Fork (in_argc, in_argv)
{
	telDev = "/dev/ttyS0";
	telConn = NULL;

	createValue (axAlt, "AXALT", "altitude axis", true, RTS2_VALUE_WRITABLE);
	createValue (axAz, "AXAZ", "azimuth axis", true, RTS2_VALUE_WRITABLE);

	createValue (motorAlt, "motor_alt", "altitude motor", false, RTS2_VALUE_WRITABLE);
	createValue (motorAz, "motor_az", "azimuth motor", false, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "telescope device (default to /dev/ttyS0)");
}

Kolonica::~Kolonica (void)
{
	delete telConn;
}

int main (int argc, char **argv)
{
	Kolonica device = Kolonica (argc, argv);
	return device.run ();
}
