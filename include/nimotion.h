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

#include <sys/types.h>
#include <stdint.h>

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
#define NIMC_NOAXIS            0x00
#define NIMC_AXIS1             0x01

//Load Counts/Steps per Revolution constants
#define NIMC_COUNTS  0
#define NIMC_STEPS   1

#define NIMC_DAC1              0x31
#define NIMC_COMMAND_IN_PROGRESS 0x08     //Waiting for more data from host to complete the command

#define NIMC_STEP_OUTPUT1      0x41

// CSR values
#define NIMC_READY_TO_RECEIVE    0x01     //Ready to receive
#define NIMC_DATA_IN_RDB         0x02     //Data in return data buffer
#define NIMC_COMMAND_IN_PROGRESS 0x08     //Waiting for more data from host to complete the command
#define NIMC_PACKET_ERROR        0x10     //Packet error
#define NIMC_E_STOP              0x10     //E Stop status
#define NIMC_POWER_UP_RESET      0x20     //Power up Reset
#define NIMC_MODAL_ERROR_MSG     0x40     //Modal error message
#define NIMC_HARDWARE_FAIL       0x80     //Hardware Fail bit

// These constants are used by flex_enable_axes to set the PID rate.
#define NIMC_PID_RATE_62_5    0
#define NIMC_PID_RATE_125     1

#define NIMC_PID_RATE_188     2
#define NIMC_PID_RATE_250     3
#define NIMC_PID_RATE_313     4
#define NIMC_PID_RATE_375     5
#define NIMC_PID_RATE_438     6
#define NIMC_PID_RATE_500     7

// Stepper Output Mode
#define NIMC_CLOCKWISE_COUNTERCLOCKWISE 0
#define NIMC_STEP_AND_DIRECTION         1

// Singal Polarities
#define NIMC_ACTIVE_HIGH   0
#define NIMC_ACTIVE_LOW    1

// Output Drive Mode
#define NIMC_OPEN_COLLECTOR      0
#define NIMC_TOTEM_POLE          1

// Polarity
#define NON_INVERTING            0
#define INVERTING                1

// Operation Modes
#define  NIMC_ABSOLUTE_POSITION     0
#define  NIMC_RELATIVE_POSITION     1
#define  NIMC_VELOCITY              2
#define  NIMC_RELATIVE_TO_CAPTURE   3
#define  NIMC_MODULUS_POSITION      4
#define  NIMC_ABSOLUTE_CONTOURING   5
#define  NIMC_RELATIVE_CONTOURING   6

// stop motion types
#define  NIMC_DECEL_STOP 0
#define  NIMC_HALT_STOP 1
#define  NIMC_KILL_STOP 2

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 * Read error code.
 */
void flex_read_error_msg_rtn (int16_t *command, int16_t *resource, int32_t *error);

/**
 * Read command status register.
 */
uint16_t flex_read_csr_rtn ();

void flex_clear_pu_status ();

void flex_config_axis (uint8_t axis, uint8_t p_feedback, uint8_t s_feedback, uint8_t p_out, uint8_t s_out);

void flex_enable_axis (uint16_t axisMap, uint16_t PIDrate);

void flex_configure_stepper_output (uint8_t resource, uint8_t driveMode, uint8_t polarity, uint8_t outputMode);

void flex_load_counts_steps_rev (uint8_t resource, int16_t type, uint32_t val);

void flex_load_velocity (uint8_t axis, int32_t velocity, uint8_t inputVector);

void flex_load_target_pos (uint8_t axis, uint32_t position);

void flex_config_inhibit_output (uint8_t resource, uint16_t enable, uint16_t polarity,uint16_t driveMode);

void flex_read_velocity_rtn (uint8_t axis, int32_t *velocity);

void flex_read_position_rtn (uint8_t axis, int32_t *position);

void flex_set_op_mode (uint8_t resource, uint16_t operationMode);

void flex_start (uint8_t resource, uint16_t map);

void flex_stop_motion (uint8_t resource, uint16_t stopType, uint16_t map);

void initMotion ();

#ifdef __cplusplus
}
#endif
