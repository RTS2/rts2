/* 
 * NI-Motion driver card support (PCI version).
 * Copyright (C) 2011,2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 * Copyright (C) 2012 Christopher Klein, UC Berkeley
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

/*
 * This code was tested on NI-Motion 7332 card with MID-7604 stepper motor
 * driver.
 *
 * The following documents provide information on which functions are
 * available, what is its C signature (the first one), what is its command ID
 * and how parameters are passed (the second one):
 *
 * NI-Motion Function Help
 * http://www.ni.com/pdf/manuals/370538e.zip
 *
 * NI-Motion DDK
 * http://joule.ni.com/nidu/cds/view/p/id/482/lang/en
 * http://ftp.ni.com/support/softlib/motion_control/NI-Motion%20Driver%20Development%20Kit/Driver%20Development%20Kit%206.1.5/MotionDDK615.zip
 *
 * flex_ routines
 * http://www.ni.com/pdf/manuals/321943b.pdf
 *
 * You will need to quess a bit which command ID provides which function, as
 * there isn't any direct mapping. Checking name and parameters usually will
 * lead to finsing the match.
 */
#include "nimotion.h"
#include "sensord.h"

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>

#define MAX_AXIS      2

using namespace rts2sensord;

/**
 * Class to control NImotion drives
 *
 * @author Petr Kubanek <kubanek@fzu.cz>
 */
class NIRatir:public Sensor
{
	public:
		NIRatir (int argc, char **argv);
		virtual ~NIRatir ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int info ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
		virtual int commandAuthorized (rts2core::Connection * conn);

	private:
		const char *boardPCI;

		rts2core::ValueDoubleMinMax *axtarget[MAX_AXIS];
		rts2core::ValueLong *axposition[MAX_AXIS];

		rts2core::ValueLong *axvelocity[MAX_AXIS];
		rts2core::ValueDoubleMinMax *axmaxv[MAX_AXIS];
		rts2core::ValueDoubleMinMax *axbasev[MAX_AXIS];

		rts2core::ValueDoubleMinMax *axacceleration[MAX_AXIS];
		rts2core::ValueDoubleMinMax *axdeceleration[MAX_AXIS];

		rts2core::ValueBool *axenabled[MAX_AXIS];

		rts2core::BoolArray *ports;

		rts2core::IntegerArray *ADCs;

		void moveAbs (int axis, int32_t pos);
		void moveVelocity (int axis, int32_t velocity);
};

NIRatir::NIRatir (int argc, char **argv):Sensor (argc, argv)
{
	for (int i = 0; i < MAX_AXIS; i++)
	{
		std::ostringstream pr;
		pr << "AX" << (i + 1) << "_";
		std::string prs = pr.str ();
		createValue (axtarget[i], (prs + "TAR").c_str (), "axis target position", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
		// axtarget[i]->setValueLong (0); // commented out when changed to ValueDoubleMinMax

		createValue (axposition[i], (prs + "POS").c_str (), "current axis position", true, RTS2_VALUE_AUTOSAVE);

		createValue (axvelocity[i], (prs + "VEL").c_str (), "axis current velocity", false);

		createValue (axmaxv[i], (prs + "MAX_VEL").c_str (), "axis maximal velocity", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
		createValue (axbasev[i], (prs + "BASE_VEL").c_str (), "axis base velocity", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

		axmaxv[i]->setValueInteger (1500);
		axbasev[i]->setValueInteger (0);

		createValue (axacceleration[i], (prs + "ACC").c_str (), "axis acceleration", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
		createValue (axdeceleration[i], (prs + "DEC").c_str (), "axis deceleration", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

		axacceleration[i]->setValueInteger (1500);
		axdeceleration[i]->setValueInteger (1500);

		createValue (axenabled[i], (prs + "ENB").c_str (), "axis enabled", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
		axenabled[i]->setValueBool (true);
	}

	int i;
	createValue (ports, "PORTS", "I/O ports status", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	for (i = 0; i < 8; i++)
		ports->addValue (false);

	createValue (ADCs, "ADCs", "I/O ADC values", false);
	for (i = 0; i < 8; i++)
		ADCs->addValue (0);

	boardPCI = NULL;
	addOption ('b', NULL, 1, "NI Motion board /proc entry");
}

NIRatir::~NIRatir ()
{
}

int NIRatir::processOption (int opt)
{
	switch (opt)
	{
		case 'b':
			boardPCI = optarg;
			break;
		default:
			return Sensor::processOption (opt);
	}
	return 0;
}

int NIRatir::initHardware ()
{
	if (boardPCI == NULL)
	{
		logStream (MESSAGE_ERROR) << "board path (-b option) was not specified" << sendLog;
		return -1;
	}
	if (initMotion (boardPCI) < 0)
	{
		logStream (MESSAGE_ERROR) << "cannot init device on " << boardPCI << sendLog;
		return -1;
	}


	flex_clear_pu_status ();
	while (flex_read_csr_rtn () & NIMC_POWER_UP_RESET)
	{
		usleep (1000);
	}

	flex_enable_axis (0, 0);
	for (int i = 0; i < MAX_AXIS; i++)
		flex_config_axis (NIMC_AXIS1 + i, 0, 0, NIMC_STEP_OUTPUT1 + i, 0);
		
	flex_enable_axis (0x1e, NIMC_PID_RATE_250);
	logStream (MESSAGE_DEBUG) << "called flex enable with 0x1e" << sendLog;

	for (int i = 0; i < MAX_AXIS; i++)
	{
		flex_configure_stepper_output (NIMC_AXIS1 + i, NIMC_STEP_AND_DIRECTION, NIMC_ACTIVE_HIGH, 0);
		flex_load_counts_steps_rev (NIMC_AXIS1 + i, NIMC_STEPS, 24);
		flex_config_inhibit_output (NIMC_AXIS1 + i, 0, 0, 0);

		flex_load_base_vel (NIMC_AXIS1 + i, axbasev[i]->getValueInteger ());
		flex_load_velocity (NIMC_AXIS1 + i, axmaxv[i]->getValueInteger (), 0xff);

		flex_load_acceleration (NIMC_AXIS1 + i, NIMC_ACCELERATION, axacceleration[i]->getValueLong (), 0xff);
		flex_load_acceleration (NIMC_AXIS1 + i, NIMC_DECELERATION, axdeceleration[i]->getValueLong (), 0xff);
	}

	// enable first 8 ADCs
	//std::cout << "entering adcs" << std::endl;
	//flex_enable_adcs (0xf0);
	//std::cout << "exiting adcs" << std::endl;
	
	//flex_set_port_direction (NIMC_IO_PORT1, 0);
    
	return 0;	
}

int NIRatir::info ()
{
	int32_t cp;
	for (int i = 0; i < MAX_AXIS; i++)
	{
		flex_read_position_rtn (NIMC_AXIS1 + i, &cp);
		axposition[i]->setValueLong (cp);

		flex_read_velocity_rtn (NIMC_AXIS1 + i, &cp);
		axvelocity[i]->setValueLong (cp);
	}

	uint16_t portS;
	int i;

	flex_read_port_rtn (NIMC_IO_PORT1, &portS);
	for (i = 0; i < 8; i++)
	{
		ports->setValueBool (i, portS & 0x01);
		portS = portS >> 1;
	}

/*	for (i = 4; i < 8; i++)
	{
		int32_t v;
		flex_read_adc16_rtn (NIMC_ADC1 + i, &v);
		ADCs->setValueInteger (i, v);
	} */

	return Sensor::info ();
}

int NIRatir::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	for (int i = 0; i < MAX_AXIS; i++)
	{
		if (old_value == axtarget[i])
		{
			moveAbs (NIMC_AXIS1 + i, new_value->getValueLong ());
			return 0;
		}
		else if (old_value == axposition[i])
		{
			flex_reset_pos (NIMC_AXIS1 + i, new_value->getValueLong (), 0, 0xff);
			return 0;
		}
		else if (old_value == axbasev[i])
		{
			flex_load_base_vel (NIMC_AXIS1 + i, new_value->getValueLong ());
			return 0;
		}
		else if (old_value == axmaxv[i])
		{
			flex_load_velocity (NIMC_AXIS1 + i, new_value->getValueLong (), 0xff);
			return 0;
		}
		else if (old_value == axacceleration[i])
		{
			flex_load_acceleration (NIMC_AXIS1 + i, NIMC_ACCELERATION, new_value->getValueLong (), 0xff);
			return 0;
		}
		else if (old_value == axdeceleration[i])
		{
			flex_load_acceleration (NIMC_AXIS1 + i, NIMC_DECELERATION, new_value->getValueLong (), 0xff);
			return 0;
		}
		else if (old_value == axenabled[i])
		{
			int16_t enable_map = 0;
			for (int j = 0; j < MAX_AXIS; j++)
			{
				if (j != i && axenabled[j]->getValueBool ())
					enable_map |= (0x02 << j);
			}
			if (((rts2core::ValueBool *) new_value)->getValueBool ())
				enable_map |= (0x02 << i);
			//flex_config_inhibit_output (NIMC_AXIS1 + i, 0, 0, 0);
			flex_enable_axis (enable_map, NIMC_PID_RATE_250);
			flex_config_inhibit_output (NIMC_AXIS1 + i, 0, 0, 0);
			logStream (MESSAGE_DEBUG) << "called flex_enable with 0x" << std::hex << std::setw (2) << enable_map << sendLog;
			if (((rts2core::ValueBool *) new_value)->getValueBool () == false)
			{
				logStream (MESSAGE_DEBUG) << "call stop motion with kill_stop" << sendLog;
				flex_config_inhibit_output (NIMC_AXIS1 + i, 1, 1, 0);
				// flex_stop_motion (NIMC_AXIS1 + i, NIMC_KILL_STOP, 0x02);
			}
			return 0;
		}
	}
	if (old_value == ports)
	{
		rts2core::BoolArray *nv = (rts2core::BoolArray *) new_value;
		uint8_t mon = 0;
		uint8_t moff = 0;
		for (int i = 8; i >= 0; i--)
		{
			mon <<= 1;
			moff <<= 1;

			if ((*nv)[i])
				mon |= 0x01;
			else
				moff |= 0x01;
		}
		flex_set_port (NIMC_IO_PORT1, mon, moff);
		return 0;
	}
	return Sensor::setValue (old_value, new_value);
}

int NIRatir::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("reset"))
	{
		for (int i = 0; i < MAX_AXIS; i++)
			flex_reset_pos (NIMC_AXIS1 + i, 0, 0, 0xff);
		return 0;
	}
	return Sensor::commandAuthorized (conn);
}

void NIRatir::moveAbs (int axis, int32_t pos)
{
	logStream (MESSAGE_INFO) << "moveAbs axis num " << std::hex << axis << " to " << pos << sendLog;
	flex_stop_motion (axis, NIMC_DECEL_STOP, 0x02);
	flex_set_op_mode (axis, NIMC_ABSOLUTE_POSITION);
	flex_load_target_pos (axis, pos);
	flex_start (axis, 0x02);
}

void NIRatir::moveVelocity (int axis, int32_t velocity)
{
	flex_stop_motion (axis, NIMC_DECEL_STOP, 0x02);
	flex_set_op_mode (axis, NIMC_VELOCITY);
	flex_load_velocity (axis, velocity, 0xff);
	flex_start (axis, 0x02);
}

int main (int argc, char **argv)
{
	NIRatir device (argc, argv);
	return device.run ();
}
