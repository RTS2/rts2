/* 
 * Driver to control switches on Bootes 1 through Ford board.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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
#include "../utils/connford.h"

namespace rts2sensord
{

/**
 * Class for sensor representing Ford board for switches switching on Bootes 1.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Bootes1:public Sensor
{
	public:
		Bootes1 (int argc, char **argv);

	protected:
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual int info ();
	
		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
		rts2core::ValueBool *telSwitch;
		rts2core::ValueBool *asmSwitch;

		const char *ford_file;
		rts2core::FordConn *fordConn;
};

typedef enum
{
	PORT_1,
	PORT_2,
	PORT_3,
	PORT_4,
	DOMESWITCH,
	ASM_SWITCH,
	TEL_SWITCH,
	PORT_8,
	// A
	OPEN_END_1,
	CLOSE_END_1,
	CLOSE_END_2,
	OPEN_END_2,
	// 0xy0,
	PORT_13,
	RAIN_SENSOR
} outputs;

}

using namespace rts2sensord;

Bootes1::Bootes1 (int argc, char **argv):Sensor (argc, argv)
{
	ford_file = "/dev/ttyS0";
	fordConn = NULL;
	addOption ('f', NULL, 1, "/dev file for board serial port (default to /dev/ttyS0)");

	createValue (telSwitch, "tel_switch", "switch for telescope", false, RTS2_VALUE_WRITABLE);
	telSwitch->setValueBool (true);

	createValue (asmSwitch, "asm_switch", "switch for ASM", false, RTS2_VALUE_WRITABLE);
	asmSwitch->setValueBool (true);
}

int Bootes1::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			ford_file = optarg;
			break;
		default:
			Sensor::processOption (in_opt);
	}
	return 0;
}

int Bootes1::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == telSwitch)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return fordConn->ZAP(TEL_SWITCH) == 0 ? 0 : -2;
		}
		else
		{
			return fordConn->VYP(TEL_SWITCH) == 0 ? 0 : -2;
		}
	}
	if (old_value == asmSwitch)
	{
		if (((rts2core::ValueBool* )new_value)->getValueBool () == true)
		{
			return fordConn->ZAP(ASM_SWITCH) == 0 ? 0 : -2;
		}
		else
		{
			return fordConn->VYP(ASM_SWITCH) == 0 ? 0 : -2;
		}
	}
	return Sensor::setValue (old_value, new_value);
}



int Bootes1::init ()
{
	int ret;
	ret = Sensor::init ();
	if (ret)
		return ret;

	fordConn = new rts2core::FordConn (ford_file, this, rts2core::BS9600, rts2core::C8, rts2core::NONE, 40);
	ret = fordConn->init ();
	if (ret)
		return ret;

	fordConn->flushPortIO ();

	ret = fordConn->ZAP(TEL_SWITCH);
	if (ret)
		return ret;
	
	ret = fordConn->ZAP(ASM_SWITCH);
	if (ret)
		return ret;

	return 0;
}

int Bootes1::info ()
{
	int ret;
	ret = fordConn->zjisti_stav_portu  ();
	if (ret)
		return -1;

	telSwitch->setValueBool (fordConn->getPortState (TEL_SWITCH));
	asmSwitch->setValueBool (fordConn->getPortState (ASM_SWITCH));

	return Sensor::info ();
}

int main (int argc, char **argv)
{
	Bootes1 device = Bootes1 (argc, argv);
	return device.run ();
}
