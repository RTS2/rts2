/*
 *
 * Copyright (C) 2010 Markus Wildi, version for RTS2
 * Copyright (C) 2009 Lukas Zimmermann, Basel, Switzerland
 *
 *
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Or visit http://www.gnu.org/licenses/gpl.html.

 *
 */

#ifndef __ssd650v_comm_h__
#define __ssd650v_comm_h__


// default serial port device and it's default settings
#define DEFAULT_SERPORT_DEV      "/dev/ssd650v"
#define DEFAULT_BITRATE          19200
#define DEFAULT_DATABITS         7
#define DEFAULT_PARITY           PARITY_EVEN
#define DEFAULT_STOPBITS         1



static int ser_dev;
static int ser_open = 0;
static int inverter_seq_status = -1;
static int mot_cmd_status = -1;
static struct timeval time_last_stat;   /* time of last status enquiry sent to
                                         * SSD650V */
// INDI replacements
#define ISS_ON 0
#define ISS_OFF 1 
#define IPS_IDLE 1 


static int motor_fast_stop_state= 0 ;
// MotorStartSP.sp[0].s
// MotorStartSP.sp[1].s
// MotorStartSP.sp[2].s
// MotorStartSP.sp[3].s
static int motor_start_state= 0 ;
// MotorCoastSP.sp[0].s
// MotorCoastSP.sp[1].s
static int motor_coast_state= 0 ;
// SSD_MotorSeqLP.s
static int ssd_motor_seq= 0 ;

int ssd_start_comm( char *device_ssd650v);

int ssd_stop_comm() ;

char * SSD_qry_identity(int sd);

char * SSD_qry_major_state(int sd);

int SSD_qry_comms_status(int sd);

int SSD_qry_last_error(int sd);

float SSD_qry_setpoint(int sd);

int SSD_qry_hexword(int sd, int tag);

float SSD_qry_real(int sd, int tag);

int SSD_set_tag(int sd, int tag, char * data);

#endif   // #ifndef __ssd650v_comm_h__

