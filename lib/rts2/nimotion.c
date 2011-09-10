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

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// mapped memory location
uint16_t *mem;

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

void readPacket (int16_t *c, uint8_t size, uint16_t *data)
{
	// read header..
	int16_t h = readData ();
	int16_t s;
	*c = readData ();
	// extract size
	for (s = h >> 8; s > 0; s--)
	{
		*data = readData ();
		data++;
	}
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
	fprintf (stderr, "command %04x res %04x error %d\n", *command, *resource, *error);
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
	int16_t c;
	readPacket (&c, 2, (uint16_t *) position);
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

void initMotion ()
{
	int16_t c, r;
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
}
