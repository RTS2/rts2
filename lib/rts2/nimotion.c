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
 * lead to figuring the match.
 */
#include "nimotion.h"

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// mapped memory location
uint16_t *mem;

#define DEBUG_LOG

#ifdef DEBUG_LOG
FILE *fLog;
#endif

// enables printout of low level debug
#define DEBUG

int writeWord (uint16_t pd)
{
	while (1)
	{
		uint16_t a = mem[4];
#ifdef DEBUG
		printf ("mem %04x\r", a);
#endif
		if (a & NIMC_READY_TO_RECEIVE)
			break;
		usleep (1000);
	}
		
	mem[0] = pd;
#ifdef DEBUG
	printf ("wrote %04x (%d)\n", pd, pd);
	printf ("mcs %04x csr %04x\n", mem[2], mem[4]);
#endif
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
uint16_t readData ()
{
	// wait for RDB
	while ((mem[4] & NIMC_DATA_IN_RDB) == 0x00)
	{
#ifdef DEBUG
		printf ("wait %x\r", mem[4]);
#endif
		if (mem[4] & NIMC_PACKET_ERROR)
		{
#ifdef DEBUG
			printf ("\nerror %x\n", mem[4]);
#endif
			//exit (5);
		}
		usleep (1000);
	}
	uint16_t r = mem[0];
#ifdef DEBUG
	printf ("readData %04x (%d)\n", r, r);
#endif

#ifdef DEBUG_LOG
	fprintf (fLog, "readData %04x (%d)\n", r, r);
	fflush(fLog);
#endif
	return r;
}

void readPacket (int16_t *c, uint8_t size, uint16_t *data)
{
	// read header..
	int16_t h = readData ();
	int16_t s = h >> 8;
	if (s - 2 != size)
	{
#ifdef DEBUG
		printf ("invalid reply length - expected %d, received %d\n", size, s - 2);
		return;
#endif
	}

	*c = readData ();
	// extract size
	for (s = h >> 8; s > 2; s--)
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
#ifdef DEBUG
		printf ("wait for command %d csr %04x\r", command, mem[4]);
#endif

#ifdef DEBUG_LOG
		fprintf (fLog, "wait for command %d csr %04x\r", command, mem[4]);
		fflush(fLog);
#endif
		usleep (1000);
	}
#ifdef DEBUG
	printf ("wait ends csr %04x                                         \n", mem[4]);
#endif

#ifdef DEBUG_LOG
	fprintf (fLog, "wait ends csr %04x                                         \n", mem[4]);
	fflush(fLog);
#endif
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
#ifdef DEBUG
	printf ("reading error\n");
#endif
	int16_t h = readData ();
	int16_t len = (h >> 8) & 0xff;
	*command = readData ();
	if (len > 2)
		*resource = readData ();
	else
		*resource = 0xff;
	if (len > 3)
		*error = -70000 - ((int32_t) readData ());
	else
		*error = 0xabcdef00;
	
	len -= 4;

	while (len > 0 && mem[4] & NIMC_DATA_IN_RDB)
	{
		sleep (1);
		readData ();
		len--;
	}
#ifdef DEBUG
	fprintf (stderr, "error reply len %d command %04x res %04x error %d\n", len, *command, *resource, *error);
#endif
}

void checkStatus ()
{
	if (mem[4] & NIMC_MODAL_ERROR_MSG)
	{
#ifdef DEBUG
		fprintf (stderr, "modal error\n");
#endif
		int16_t c, r;
		int32_t e;
		flex_read_error_msg_rtn (&c, &r, &e);
	}
	if (mem[4] & NIMC_PACKET_ERROR)
	{
#ifdef DEBUG
		fprintf (stderr, "packet error\n");
#endif
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

void flex_load_base_vel (uint8_t axis, uint16_t baseVelocity)
{
	writePacket (axis, 28, 1, &baseVelocity);
	checkStatus ();
}

void flex_load_velocity (uint8_t axis, int32_t velocity, uint8_t inputVector)
{
	uint16_t data[2];
	data[0] = ((uint32_t) velocity) >> 16;
	data[1] = velocity & 0xffff;
	writePacketWithIOVector (axis, 391, 2, data, inputVector);
	checkStatus ();
}

void flex_load_rpm (uint8_t axis, int64_t rpm, uint8_t inputVector)
{
	uint16_t data[4];
	data[0] = rpm & 0xffff;
	data[1] = (rpm >> 16) & 0xffff;
	data[2] = (rpm >> 32) & 0xffff;
	data[3] = (rpm >> 48) & 0xffff;
	writePacketWithIOVector (axis, 371, 4, data, inputVector);
	checkStatus ();
}

void flex_load_acceleration (uint8_t axis, uint16_t accelerationType, uint32_t acceleration, uint8_t inputVector)
{
	uint16_t data[3];
	data[0] = accelerationType;
	data[1] = acceleration >> 16;
	data[2] = acceleration & 0xffff;
	writePacketWithIOVector (axis, 379, 3, data, inputVector);
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

void flex_set_inhibit_output_momo (uint16_t mustOn, uint16_t mustOff)
{
	uint16_t data[2];
	data[0] = mustOn;
	data[1] = mustOff;
	writePacket (NIMC_NOAXIS, 443, 2, data);
	checkStatus ();
}

// swap 2 bytes..
void btol32 (uint32_t *b)
{
	uint32_t r = *b & 0xffff;
	r = r << 16;
	r |= (*b >> 16);
	*b = r;

#ifdef DEBUG_LOG
	unsigned char *p = (unsigned char*) b;
	fprintf (fLog, "btol %x %x %x %x %d\n", p[0], p[1], p[2], p[3], r);
	fflush (fLog);
#endif
}

void flex_read_velocity_rtn (uint8_t axis, int32_t *velocity)
{
	writePacket (axis, 392, 0, NULL);
	int16_t c;
	readPacket (&c, 2, (uint16_t *) velocity);
	btol32 ((uint32_t *) velocity);
}

void flex_read_rpm (uint8_t axis, int64_t *rpm)
{
	writePacket (axis, 374, 0, NULL);
	int16_t c;
	readPacket (&c, 4, (uint16_t *) rpm);
}

void flex_read_position_rtn (uint8_t axis, int32_t *position)
{
#ifdef DEBUG_LOG
	fprintf(fLog, "read position, axis=%d\n", axis);
	fflush(stdout);
#endif
	writePacket (axis, 41, 0, NULL);
	int16_t c;
	readPacket (&c, 2, (uint16_t *) position);
	btol32 ((uint32_t *) position);

#ifdef DEBUG_LOG
	fprintf(fLog, "read position, axis=%d\n", *position);
	fflush(stdout);
#endif
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

void flex_reset_pos (uint8_t axis, int32_t position1, int32_t position2, uint8_t inputVector)
{
	uint16_t data[4];
	data[0] = position1 >> 16;
	data[1] = position1 & 0xffff;
	data[2] = position2 >> 16;
	data[3] = position2 & 0xffff;
	writePacketWithIOVector (axis, 42, 4, data, inputVector);
	checkStatus ();
}

void flex_read_port_rtn (int8_t port, uint16_t *portData)
{
	writePacket (port, 319, 0, NULL);
	int16_t c;
	readPacket (&c, 1, portData);
}

void flex_set_port_direction (uint8_t port, uint16_t directionMap)
{
	writePacket (port, 398, 1, &directionMap);
	checkStatus ();
}

void flex_set_port (uint8_t port, uint8_t mustOn, uint8_t mustOff)
{
	uint16_t data;
	data = (mustOn << 8) | mustOff;

	writePacket (port, 318, 1, &data);
	checkStatus ();
}

void flex_set_adc_range (uint8_t ADC, uint16_t range)
{
	writePacket (ADC, 401, 1, &range);
	checkStatus ();
}

void flex_read_adc16_rtn (uint8_t ADC, int32_t *ADCValue)
{
	writePacket (ADC, 480, 0, NULL);
	int16_t c;
	readPacket (&c, 2, (uint16_t *) ADCValue);
	btol32 ((uint32_t *) ADCValue);
}

void flex_enable_adcs (uint16_t ADCMap)
{
	writePacket (1, 321, 1, &ADCMap);
	checkStatus ();
}

#ifdef DEBUG_LOG
void openLog()
{
	fLog = fopen("/tmp/nimotion.log", "w");
}
#endif

int initMotion (const char *path)
{
#ifdef DEBUG_LOG
	printf ("Opening log\n");
	openLog ();
	
	fprintf (fLog, "initMotion %s\n", path);
#endif

	int16_t c, r;
	int32_t e;

	char rp[strlen (path) + 11];

	sprintf (rp, "%s/resource0", path);

	int fd0 = open (rp, O_RDWR);
	if (fd0 < 0)
	{
		perror ("cannot open PCI resource file");
		return -1;
	}
	uint8_t *mem0 = mmap (NULL, 0x400, PROT_READ | PROT_WRITE, MAP_SHARED, fd0, 0);
	if (mem0 == MAP_FAILED)
	{
		perror ("cannot map PCI resource file");
		return -2;
	}

	char cpath[strlen (path) + 7];

	sprintf (cpath, "%s/config", path);

	// found bar1 offset
	int fconfig = open (cpath, O_RDONLY);
	if (fconfig < 0)
	{
		perror ("cannot open config file");
		return -3;
	}

	uint32_t bar1 = 0;
	if (lseek (fconfig, 20, SEEK_SET) < 0)
	{
		perror ("cannot seek to byte 24 in config file");
		return -4;
	}

	unsigned char barp[4];

	if (read (fconfig, barp, 4) < 4)
	{
		perror ("cannot read offsets 20-24 from config file (BAR1 location)");
		return -5;
	}

	int i;

	for (i = 0; i < 4; i++)
	{
		bar1 |= ((uint32_t) (barp[i])) << (i * 8);
	}

	printf ("(gsfc) bar1 is 0x%08x\n", bar1);


	*((int32_t *) (mem0 + 0xc0)) = bar1 | 0x80;

	sprintf (rp, "%s/resource1", path);
	int fd1 = open (rp, O_RDWR);
	if (fd1 < 0)
	{
		perror ("cannot open PCI resource1 file");
		return -6;
	}

	mem = mmap (NULL, 0x400, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
	if (mem == MAP_FAILED)
	{
		perror ("cannot map PCI resource1 file");
		return -7;
	}

#ifdef DEBUG
	printf ("address %x %x %x %x\n", mem[2], mem[4], mem[0x300], mem[0x180]);
#endif

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

	return 0;
}
