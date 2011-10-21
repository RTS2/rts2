/* 
 * NI-Motion driver card support (PCI version).
 * Copyright (C) 2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

	private:
		const char *boardPCI;

		rts2core::ValueInteger *ax1target;
		rts2core::ValueInteger *ax1position;

		rts2core::ValueInteger *ax1velocity;
		rts2core::ValueLong *ax1rpm;

		void moveAbs (int axis, int32_t pos);
		void moveVelocity (int axis, int32_t velocity);
};

NIRatir::NIRatir (int argc, char **argv):Sensor (argc, argv)
{
	createValue (ax1target, "AX1_TAR", "1st axis target position", false, RTS2_VALUE_WRITABLE);
	ax1target->setValueInteger (0);

	createValue (ax1position, "AX1_POS", "current 1st axis position", false, RTS2_VALUE_WRITABLE);

	createValue (ax1velocity, "AX1_VEL", "1st axis velocity", false, RTS2_VALUE_WRITABLE);

	createValue (ax1rpm, "AX1_RPM", "1st axis maximal velocity in RPM", false, RTS2_VALUE_WRITABLE);

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
	initMotion (boardPCI);

	flex_clear_pu_status ();
	while (flex_read_csr_rtn () & NIMC_POWER_UP_RESET)
	{
		usleep (1000);
	}

	flex_enable_axis (0, 0);
	flex_config_axis (NIMC_AXIS1, 0, 0, NIMC_STEP_OUTPUT1, 0);
	flex_enable_axis (0x02, NIMC_PID_RATE_250);
	flex_configure_stepper_output (NIMC_AXIS1, NIMC_STEP_AND_DIRECTION, NIMC_ACTIVE_HIGH, 0);
	flex_load_counts_steps_rev (NIMC_AXIS1, NIMC_STEPS, 24);
	flex_config_inhibit_output (NIMC_AXIS1, 0, 0, 0);

	return 0;	
}

int NIRatir::info ()
{
	int32_t cp;
	flex_read_position_rtn (NIMC_AXIS1, &cp);
	ax1position->setValueInteger (cp);

	flex_read_velocity_rtn (NIMC_AXIS1, &cp);
	ax1velocity->setValueInteger (cp);

	int64_t cpp;
	flex_read_rpm (NIMC_AXIS1, &cpp);
	ax1rpm->setValueLong (cpp);

	return Sensor::info ();
}

int NIRatir::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == ax1target)
	{
		moveAbs (NIMC_AXIS1, new_value->getValueInteger ());
		return 0;
	}
	else if (old_value == ax1velocity)
	{
		flex_load_velocity (NIMC_AXIS1, new_value->getValueInteger (), 0xff);
		return 0;
	}
	else if (old_value == ax1rpm)
	{
		flex_load_rpm (NIMC_AXIS1, new_value->getValueLong (), 0xff);
		return 0;
	}
	return Sensor::setValue (old_value, new_value);
}

void NIRatir::moveAbs (int axis, int32_t pos)
{
	flex_stop_motion (NIMC_NOAXIS, NIMC_DECEL_STOP, 0x02);
	flex_set_op_mode (axis, NIMC_RELATIVE_POSITION);
	flex_load_target_pos (axis, pos);
	flex_start (axis, 0x02);
}

void NIRatir::moveVelocity (int axis, int32_t velocity)
{
	flex_stop_motion (NIMC_NOAXIS, NIMC_DECEL_STOP, 0x02);
	flex_set_op_mode (axis, NIMC_VELOCITY);
	flex_load_velocity (axis, velocity, 0xff);
	flex_start (axis, 0x02);
}

int main (int argc, char **argv)
{
	NIRatir device (argc, argv);
	return device.run ();
}
