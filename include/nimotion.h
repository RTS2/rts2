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
#define NIMC_AXIS2             0x02
#define NIMC_AXIS3             0x03
#define NIMC_AXIS4             0x04
#define NIMC_AXIS5             0x05
#define NIMC_AXIS6             0x06
#define NIMC_AXIS7             0x07
#define NIMC_AXIS8             0x08
#define NIMC_AXIS9             0x09
#define NIMC_AXIS10            0x0a
#define NIMC_AXIS11            0x0b
#define NIMC_AXIS12            0x0c
#define NIMC_AXIS13            0x0d
#define NIMC_AXIS14            0x0e
#define NIMC_AXIS15            0x0f

#define NIMC_IO_PORT1          0x01
#define NIMC_IO_PORT2          0x02
#define NIMC_IO_PORT3          0x03
#define NIMC_IO_PORT4          0x04
#define NIMC_IO_PORT5          0x05
#define NIMC_IO_PORT6          0x06
#define NIMC_IO_PORT7          0x07
#define NIMC_IO_PORT8          0x08

// ADC Channles
#define NIMC_ADC1              0x51
#define NIMC_ADC2              0x52
#define NIMC_ADC3              0x53
#define NIMC_ADC4              0x54
#define NIMC_ADC5              0x55
#define NIMC_ADC6              0x56
#define NIMC_ADC7              0x57
#define NIMC_ADC8              0x58
#define NIMC_ADC9              0x59
#define NIMC_ADC10             0x5A
#define NIMC_ADC11             0x5B
#define NIMC_ADC12             0x5C
#define NIMC_ADC13             0x5D
#define NIMC_ADC14             0x5E
#define NIMC_ADC15             0x5F
#define NIMC_ADC16             0xF1
#define NIMC_ADC17             0xF2
#define NIMC_ADC18             0xF3
#define NIMC_ADC19             0xF4
#define NIMC_ADC20             0xF5
#define NIMC_ADC21             0xF6
#define NIMC_ADC22             0xF7
#define NIMC_ADC23             0xF8
#define NIMC_ADC24             0xF9
#define NIMC_ADC25             0xFA
#define NIMC_ADC26             0xFB
#define NIMC_ADC27             0xFC
#define NIMC_ADC28             0xFD
#define NIMC_ADC29             0xFE
#define NIMC_ADC30             0xFF

// Load Counts/Steps per Revolution constants
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

// Acceleration/deceleration
#define  NIMC_BOTH                  0
#define  NIMC_ACCELERATION          1
#define  NIMC_DECELERATION          2

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

/**
 * Load base velocity, e.g. velocity where move starts.
 *
 * @param axis            axis to control
 * @param baseVelocity    base velocity for the stepper axis in steps/seconds
 */
void flex_load_base_vel (uint8_t axis, uint16_t baseVelocity);

void flex_load_velocity (uint8_t axis, int32_t velocity, uint8_t inputVector);

void flex_load_rpm (uint8_t axis, int64_t rpm, uint8_t inputVector);

void flex_load_acceleration (uint8_t axis, uint16_t accelerationType, uint32_t acceleration, uint8_t inputVector);

void flex_load_target_pos (uint8_t axis, uint32_t position);

void flex_config_inhibit_output (uint8_t resource, uint16_t enable, uint16_t polarity, uint16_t driveMode);

void flex_read_velocity_rtn (uint8_t axis, int32_t *velocity);

void flex_read_rpm (uint8_t axis, int64_t *rpm);

void flex_read_position_rtn (uint8_t axis, int32_t *position);

void flex_set_op_mode (uint8_t resource, uint16_t operationMode);

void flex_start (uint8_t resource, uint16_t map);

void flex_stop_motion (uint8_t resource, uint16_t stopType, uint16_t map);

/**
 * Reset the axis position to the specified position.
 *
 * @param axis           Axis to reset
 * @param position1      value for axis and primary feedback resource
 * @param position2      value for secondary feedback resource
 * @param inputVector    source of the data for this function
 */
void flex_reset_pos (uint8_t axis, int32_t position1, int32_t position2, uint8_t inputVector);

/**
 * Read I/O port status.
 *
 * @param port           NIMC_IO_PORTn, NIMC_DIGITAL_OUTPUT_PORTn
 * @param portData       bitmasked state of 8 I/O port; upper 8 bits should be unused
 */
void flex_read_port_rtn (int8_t port, uint16_t *portData);

/**
 * Set I/O port direction.
 *
 * @param port           NIMC_IO_PORTn, NIMC_DIGITAL_OUTPUT_PORTn
 * @param directionMap   0 - output, 1 - input (default)
 */
void flex_set_port_direction (uint8_t port, uint16_t directionMap);

/**
 * Set I/O port.
 *
 * @param port           NIMC_IO_PORTn, NIMC_DIGITAL_OUTPUT_PORTn
 * @param mustOn         bitmask of outputs which must turn on
 * @param mustOff        bitmask of outputs which must turn off
 */
void flex_set_port (uint8_t port, uint8_t mustOn, uint8_t mustOff); 

/**
 * Set ADC range.
 *
 * @param ADC    adc channel to set
 * @param range  ADC range (0=-5..5V,..)
 */
void flex_set_adc_range (uint8_t ADC, uint16_t range);

/**
 * Read ADCs.
 *
 * @param ADC      ADC channel to read
 * @param ADCvalue converged ADC value
 */
void flex_read_adc16_rtn (uint8_t ADC, int32_t *ADCValue);

/**
 * Enable/disable ADC inputs.
 *
 * @param ADCMap   bitmap of ADCs to enable.
 */
void flex_enable_adcs (uint16_t ADCMap);

/**
 * @param path /proc entry to PCI device - see lspci for ID, you will then need something like /sys/bus/pci/devices/0000:01:01.0
 *
 * @return < 0 on error
 */
int initMotion (const char *path);

#ifdef __cplusplus
}
#endif
