#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint16_t *mem;

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

int writeWord (uint16_t pd)
{
	while (1)
	{
		uint16_t a = mem[4];
		printf ("mem %04x\r", a);
		if (a & NIMC_READY_TO_RECEIVE)
			break;
		usleep (1000);
	}
		
	mem[0] = pd;
	printf ("wrote %04x (%d)\n", pd, pd);
	printf ("mcs %04x csr %04x\n", mem[2], mem[4]);
	return 0;
}

/**
 * Read data from receive buffer. Waits till data arrives, and read them out.
 * Pleae note that this is low level function. You probably would like to see
 * readPacket, which reads whole packet from the registers.
 *
 * @return RDB (read data) value.
 *
 * @see readPacket
 */
int16_t readData ()
{
	// wait for RDB
	while ((mem[4] & NIMC_DATA_IN_RDB) == 0x00)
	{
		printf ("wait %x\r", mem[4]);
		if (mem[4] & NIMC_PACKET_ERROR)
		{
			printf ("\nerror %x\n", mem[4]);
			//exit (5);
		}
		usleep (1000);
	}
	int16_t r = mem[0];
	printf ("readData %04x (%d)\n", r, r);
	return r;
}

void writePacketWithIOVector (uint8_t resource, uint16_t command, uint8_t size, uint16_t *data, uint8_t inputVector)
{
	int i;
	writeWord (resource | ((size + 3) << 8));
	writeWord (command);
	for(i = 0;i < size; i++)
		writeWord (data[i]);
	writeWord ((((uint16_t) inputVector) << 8) | 0x0a);

	// wait for signaled end of packet..
	while (mem[4] & NIMC_COMMAND_IN_PROGRESS)
	{
		printf ("wait for command %d csr %04x\r", command, mem[4]);
		usleep (1000);
	}
	printf ("wait ends csr %04x                                         \n", mem[4]);
}

void writePacket (uint8_t resource, uint16_t command, uint8_t size, uint16_t *data)
{
	writePacketWithIOVector (resource, command, size, data, 0xff);
}

/** read error code..
 */
void flex_read_error_msg_rtn (int16_t *command, int16_t *resource, int32_t *error)
{
	writePacket (NIMC_NOAXIS, 2, 0, NULL);
	printf ("reading error\n");
	*command = readData ();
	*command = readData ();
	*resource = readData ();
	*error = -70000 - ((int32_t) readData ());
	do
	{
		sleep (1);
		readData ();
	}
	while (mem[4] & NIMC_DATA_IN_RDB);
	fprintf (stderr, "command %04x res %04x error %d\n", *command, *resource, *error, *error);
}

void checkStatus ()
{
	if (mem[4] & NIMC_MODAL_ERROR_MSG)
	{
		fprintf (stderr, "modal error\n");
		int16_t c, r;
		int32_t e;
		flex_read_error_msg_rtn (&c, &r, &e);
	}
	if (mem[4] & NIMC_PACKET_ERROR)
	{
		fprintf (stderr, "packet error\n");
	}
}

/**
 * Read command status register.
 */
uint16_t flex_read_csr_rtn ()
{
	return mem[4];
}

void flex_clear_pu_status ()
{
	writePacket (NIMC_NOAXIS, 258, 0, NULL);
	checkStatus ();
}

void flex_config_axis (uint8_t axis, uint8_t p_feedback, uint8_t s_feedback, uint8_t p_out, uint8_t s_out)
{
	uint16_t data[2];
	data[0] = (((uint16_t) p_feedback) << 8) | s_feedback;
	data[1] = (((uint16_t) p_out) << 8) | s_out;
	writePacket (axis, 281, 2, data);
	checkStatus ();
}

void flex_enable_axis (uint16_t axisMap, uint16_t PIDrate)
{
	uint16_t data[2];
	data[0] = axisMap;
	data[1] = PIDrate;
	writePacket (NIMC_NOAXIS, 436, 2, data);
	checkStatus ();
}

void flex_configure_stepper_output (uint8_t resource, uint8_t driveMode, uint8_t polarity, uint8_t outputMode)
{
	uint16_t data = (driveMode) | (polarity << 2) | (outputMode << 4);
	writePacket (resource, 490, 1, &data);
	checkStatus ();
}

void flex_load_counts_steps_rev (uint8_t resource, int16_t type, uint32_t val)
{
	uint16_t data[3];
	data[0] = type;
	data[1] = val >> 16;
	data[2] = val & 0xffff;
	writePacket (resource, 406, 3, data);
	checkStatus ();
}

void flex_load_velocity (uint8_t axis, int32_t velocity, uint8_t inputVector)
{
	uint16_t data[2];
	data[0] = ((uint32_t) velocity) >> 16;
	data[1] = velocity & 0xffff;
	writePacketWithIOVector (axis, 391, 2, data, 0xff);
	checkStatus ();
}

void flex_load_target_pos (uint8_t axis, uint32_t position)
{
	uint16_t data[2];
	data[0] = position >> 16;
	data[1] = position & 0xffff;
	writePacket (axis, 39, 2, data);
	checkStatus ();
}

void flex_config_inhibit_output (uint8_t resource, uint16_t enable, uint16_t polarity,uint16_t driveMode)
{
	uint16_t data;
	data = enable | (polarity << 2) | (driveMode << 4);
	writePacket (resource, 491, 1, &data);
	checkStatus ();
}

void flex_read_position_rtn (uint8_t axis, int32_t *position)
{
	writePacket (axis, 41, 0, NULL);
	//readPacket (2, position);
}

void flex_set_op_mode (uint8_t resource, uint16_t operationMode)
{
	writePacket (resource, 35, 1, &operationMode);
	checkStatus ();
}

void flex_start (uint8_t resource, uint16_t map)
{
	writePacket (resource, 21, 1, &map);
	checkStatus ();
}

void flex_stop_motion (uint8_t resource, uint16_t stopType, uint16_t map)
{
	uint16_t data[2] = {stopType, map};
	writePacket (resource, 384, 2, data);
	checkStatus ();
}

int main (int argc, char **argv)
{
	int16_t r, c;
	int32_t e;

	int fd0 = open ("/sys/bus/pci/devices/0000:01:01.0/resource0", O_RDWR);
	if (fd0 < 0)
	{
		perror ("cannot open PCI resource file");
		exit (1);
	}
	uint8_t *mem0 = mmap (NULL, 0x400, PROT_READ | PROT_WRITE, MAP_SHARED, fd0, 0);
	if (mem0 == MAP_FAILED)
	{
		perror ("cannot map PCI resource file");
		exit (2);
	}

	*((int32_t *) (mem0 + 0xc0)) = 0xff8fe000 | 0x80;

	int fd1 = open ("/sys/bus/pci/devices/0000:01:01.0/resource1", O_RDWR);
	if (fd1 < 0)
	{
		perror ("cannot open PCI resource1 file");
		exit (3);
	}

	mem = mmap (NULL, 0x400, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
	if (mem == MAP_FAILED)
	{
		perror ("cannot map PCI resource1 file");
		exit (4);
	}
	
	printf ("address %x %x %x %x\n", mem[2], mem[4], mem[0x300], mem[0x180]);

	writeWord (0x000a);

	while (mem[4] & (NIMC_PACKET_ERROR | NIMC_COMMAND_IN_PROGRESS | NIMC_DATA_IN_RDB))
	{
		writeWord (0x000a);
		// flush read buffer
		if (mem[4] & NIMC_DATA_IN_RDB)
			readData ();
	}

	while (mem[4] & NIMC_MODAL_ERROR_MSG)
	{
		flex_read_error_msg_rtn (&c, &r, &e);
	}

	printf ("clearing status\n");

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

	flex_set_op_mode (NIMC_AXIS1, NIMC_ABSOLUTE_POSITION);

	flex_load_target_pos (NIMC_AXIS1, 20000);

	flex_start (NIMC_NOAXIS, 0x02);

	sleep (2);

	flex_stop_motion (NIMC_NOAXIS, NIMC_DECEL_STOP, 0x02);

	sleep (2);

	flex_load_target_pos (NIMC_AXIS1, 4000);

	flex_start (NIMC_NOAXIS, 0x02);

	sleep (5);

	flex_load_target_pos (NIMC_AXIS1, 20000);

	flex_start (NIMC_NOAXIS, 0x02); 

	flex_set_op_mode (NIMC_AXIS1, NIMC_VELOCITY);

	flex_stop_motion (NIMC_NOAXIS, NIMC_DECEL_STOP, 0x02);

	sleep (2);

	flex_load_velocity (NIMC_AXIS1, 200, 0xff);

	flex_start (NIMC_NOAXIS, 0x02);

	sleep (5);

	flex_stop_motion (NIMC_NOAXIS, NIMC_DECEL_STOP, 0x02);

	sleep (2);

	flex_load_velocity (NIMC_AXIS1, -200, 0xff);

	flex_start (NIMC_NOAXIS, 0x02);

	sleep (5);
	
	checkStatus ();
}
