/*
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "filterd.h"
#include "connection/thorlabs.h"

using namespace rts2filterd;

/**
 * Throlabs FW102c filter wheel.
 *
 * Documentation at
 * http://www.thorlabs.us/Thorcat/20200/FW102C-Manual.pdf
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class FW102c:public Filterd
{
	public:
		FW102c (int argc, char **argv);
		virtual ~FW102c ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		virtual int getFilterNum ();
		virtual int setFilterNum (int new_filter);
	
	private:
		// FW102c is communicationg over GPIB-style serial
		rts2core::ConnThorLabs *fwConn;
		const char *fwDev;

		rts2core::ValueBool *sensorMode;
};

FW102c::FW102c (int argc, char **argv):Filterd (argc, argv)
{
	fwConn = NULL;
	fwDev = NULL;

	createValue (sensorMode, "sensors", "sensor mode", true, RTS2_VALUE_WRITABLE);

	addOption ('f', NULL, 1, "device serial port");
}

FW102c::~FW102c ()
{
	delete fwConn;
}

int FW102c::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			fwDev = optarg;
			break;
		default:
			return Filterd::processOption (opt);
	}
	return 0;
}

int FW102c::initHardware ()
{
	fwConn = new rts2core::ConnThorLabs (fwDev, this, rts2core::FW);
	fwConn->setDebug (getDebug ());
	if (fwConn->init ())
		return -1;
	return 0;
}

int FW102c::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == sensorMode)
		return fwConn->setValue ("sensors", new_value) ? -2 : 0;
	return Filterd::setValue (old_value, new_value);
}

int FW102c::getFilterNum ()
{
	int val;
	if (fwConn->getInt ("pcount", val))
		return -1;
	return val;
}

int FW102c::setFilterNum (int new_filter)
{
	return fwConn->setInt ("pos", new_filter);
}

int main (int argc, char **argv)
{
	FW102c device (argc, argv);
	return device.run ();
}
